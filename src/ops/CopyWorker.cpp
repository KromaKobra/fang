#include "CopyWorker.h"

#include "util/Logging.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QVariantMap>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>

namespace fang {

namespace {
constexpr qint64 kChunkSize = 1024 * 1024;
constexpr int kProgressIntervalMs = 100;

// atime/mtime via utimensat: works for dirs too (QFile::setFileTime needs an
// openable file handle).
void preserveTimes(const QString& src, const QString& dest)
{
    struct stat st{};
    if (::stat(QFile::encodeName(src).constData(), &st) != 0)
        return;
    struct timespec times[2] = {st.st_atim, st.st_mtim};
    ::utimensat(AT_FDCWD, QFile::encodeName(dest).constData(), times, 0);
}
} // namespace

CopyWorker::CopyWorker(const QStringList& sources, const QString& destDir,
                       std::shared_ptr<OpControl> control, bool moveMode)
    : OpWorker(std::move(control))
    , m_sources(sources)
    , m_destDir(QDir::cleanPath(destDir))
    , m_moveMode(moveMode)
{
}

void CopyWorker::run()
{
    for (const QString& src : m_sources) {
        const QString norm = QDir::cleanPath(src);
        if (m_destDir == norm || m_destDir.startsWith(norm + QLatin1Char('/'))) {
            emit done(false, tr("The destination is inside %1").arg(norm), {});
            return;
        }
    }
    const QFileInfo destInfo(m_destDir);
    if (!destInfo.isDir() || !destInfo.isWritable()) {
        emit done(false, tr("Destination folder is not writable: %1").arg(m_destDir), {});
        return;
    }

    for (const QString& src : m_sources)
        enumerate(src, &m_bytesTotal, &m_filesTotal);
    m_progressClock.start();
    emit stateHint(int(OpState::Running));
    reportProgress(m_sources.value(0), m_destDir, true);

    for (const QString& src : m_sources) {
        if (!keepGoing())
            break;
        const QFileInfo sfi(src);
        if (!sfi.exists() && !sfi.isSymLink()) {
            addError(tr("No such file: %1").arg(src));
            continue;
        }

        QString dest = m_destDir + QLatin1Char('/') + sfi.fileName();
        bool merge = false;
        if (QFileInfo::exists(dest) || QFileInfo(dest).isSymLink()) {
            const ConflictMode mode = askConflict(src, dest);
            if (m_ctl->cancelled.load())
                break;
            switch (mode) {
            case ConflictMode::Skip:
                countSubtreeAsDone(src);
                continue;
            case ConflictMode::Overwrite:
                removeExisting(dest);
                break;
            case ConflictMode::Merge:
                if (sfi.isDir() && !sfi.isSymLink() && QFileInfo(dest).isDir())
                    merge = true;
                else
                    removeExisting(dest);
                break;
            case ConflictMode::Rename:
            case ConflictMode::KeepBoth:
                dest = uniqueName(dest);
                break;
            }
        }

        const bool clean = m_moveMode ? moveEntry(src, dest, merge)
                                      : copyEntry(src, dest, merge);
        if (clean)
            ++m_successes;
    }

    QVariantMap undo;
    if (m_moveMode) {
        undo.insert(QStringLiteral("pairs"), m_movePairs);
    } else {
        undo.insert(QStringLiteral("sources"), m_sources);
        undo.insert(QStringLiteral("destDir"), m_destDir);
        undo.insert(QStringLiteral("createdPaths"), m_createdPaths);
    }
    reportProgress(QString(), QString(), true);
    // Per docs/03 §6 default policy: collect per-file errors and continue;
    // only a batch with zero successes fails outright.
    const bool ok = m_errors.isEmpty() || m_successes > 0 || !m_movePairs.isEmpty()
        || !m_createdPaths.isEmpty();
    emit done(ok, errorSummary(), undo);
}

// ------------------------------------------------------------------- copy

bool CopyWorker::copyEntry(const QString& src, const QString& dest, bool mergeConflicts)
{
    if (!keepGoing())
        return false;

    const QFileInfo sfi(src);
    reportProgress(src, dest);

    if (sfi.isSymLink()) {
        if (mergeConflicts && (QFileInfo::exists(dest) || QFileInfo(dest).isSymLink())) {
            const ConflictMode mode = askConflict(src, dest);
            if (mode == ConflictMode::Skip || m_ctl->cancelled.load()) {
                ++m_filesDone;
                return false;
            }
            if (mode == ConflictMode::Rename || mode == ConflictMode::KeepBoth)
                return copyEntry(src, uniqueName(dest), false);
            removeExisting(dest); // Overwrite / Merge
        }
        ++m_filesDone;
        if (!QFile::link(sfi.symLinkTarget(), dest)) {
            addError(tr("Could not create link %1").arg(dest));
            return false;
        }
        m_createdPaths << dest;
        reportProgress(src, dest);
        return true;
    }

    if (sfi.isDir()) {
        const QFileInfo dfi(dest);
        bool preExisting = dfi.exists();
        if (preExisting && !dfi.isDir()) {
            // Only reachable while merging; a file sits where the dir goes.
            const ConflictMode mode = askConflict(src, dest);
            if (mode == ConflictMode::Skip || m_ctl->cancelled.load()) {
                countSubtreeAsDone(src);
                return false;
            }
            if (mode == ConflictMode::Rename || mode == ConflictMode::KeepBoth)
                return copyEntry(src, uniqueName(dest), false);
            removeExisting(dest);
            preExisting = false;
        }
        if (!preExisting) {
            if (!QDir().mkdir(dest)) {
                addError(tr("Could not create folder %1").arg(dest));
                countSubtreeAsDone(src);
                return false;
            }
            m_createdPaths << dest;
        }
        ++m_filesDone;

        bool clean = true;
        const QStringList children = QDir(src).entryList(
            QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        for (const QString& name : children) {
            if (!keepGoing())
                return false;
            clean &= copyEntry(src + QLatin1Char('/') + name, dest + QLatin1Char('/') + name,
                               mergeConflicts);
        }
        applyMetadata(src, dest);
        return clean;
    }

    // Regular file.
    if (mergeConflicts && (QFileInfo::exists(dest) || QFileInfo(dest).isSymLink())) {
        const ConflictMode mode = askConflict(src, dest);
        if (mode == ConflictMode::Skip || m_ctl->cancelled.load()) {
            m_bytesDone += sfi.size();
            ++m_filesDone;
            return false;
        }
        if (mode == ConflictMode::Rename || mode == ConflictMode::KeepBoth)
            return copyEntry(src, uniqueName(dest), false);
        removeExisting(dest); // Overwrite (Merge is dirs-only)
    }
    return copyFileStream(src, dest);
}

bool CopyWorker::copyFileStream(const QString& src, const QString& dest)
{
    const qint64 fileSize = QFileInfo(src).size();
    qint64 copied = 0;
    const auto skipRest = [&] {
        m_bytesDone += fileSize - copied;
        ++m_filesDone;
    };

    QFile in(src);
    if (!in.open(QIODevice::ReadOnly)) {
        addError(tr("Could not read %1: %2").arg(src, in.errorString()));
        skipRest();
        return false;
    }
    QFile out(dest);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        addError(tr("Could not write %1: %2").arg(dest, out.errorString()));
        skipRest();
        return false;
    }

    QByteArray buffer(kChunkSize, Qt::Uninitialized);
    while (true) {
        const qint64 n = in.read(buffer.data(), buffer.size());
        if (n < 0) {
            addError(tr("Read error on %1: %2").arg(src, in.errorString()));
            out.close();
            out.remove();
            skipRest();
            return false;
        }
        if (n == 0)
            break;
        if (out.write(buffer.constData(), n) != n) {
            addError(tr("Write error on %1: %2").arg(dest, out.errorString()));
            out.close();
            out.remove();
            skipRest();
            return false;
        }
        copied += n;
        m_bytesDone += n;
        reportProgress(src, dest);
        if (!keepGoing()) {
            // Cancel mid-file never leaves a partial destination behind.
            out.close();
            out.remove();
            return false;
        }
    }
    in.close();
    if (!out.flush()) {
        addError(tr("Write error on %1: %2").arg(dest, out.errorString()));
        out.close();
        out.remove();
        skipRest();
        return false;
    }
    out.close();

    if (m_moveMode && QFileInfo(dest).size() != fileSize) {
        // Cross-device move verifies sizes before the source may be deleted.
        addError(tr("Verification failed for %1").arg(dest));
        QFile::remove(dest);
        skipRest();
        return false;
    }

    applyMetadata(src, dest);
    m_createdPaths << dest;
    ++m_filesDone;
    reportProgress(src, dest);
    return true;
}

// ------------------------------------------------------------------- move

bool CopyWorker::moveEntry(const QString& src, const QString& dest, bool merge)
{
    if (!keepGoing())
        return false;
    reportProgress(src, dest);

    if (merge) {
        // Both are dirs: per-child move, conflicts asked as encountered.
        bool clean = true;
        const QStringList children = QDir(src).entryList(
            QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        for (const QString& name : children) {
            if (!keepGoing())
                return false;
            const QString childSrc = src + QLatin1Char('/') + name;
            const QString childDest = dest + QLatin1Char('/') + name;
            const QFileInfo csfi(childSrc);
            const QFileInfo cdfi(childDest);
            if (cdfi.exists() || cdfi.isSymLink()) {
                if (csfi.isDir() && !csfi.isSymLink() && cdfi.isDir() && !cdfi.isSymLink()) {
                    clean &= moveEntry(childSrc, childDest, true);
                    continue;
                }
                const ConflictMode mode = askConflict(childSrc, childDest);
                if (m_ctl->cancelled.load())
                    return false;
                switch (mode) {
                case ConflictMode::Skip:
                    countSubtreeAsDone(childSrc);
                    clean = false;
                    continue;
                case ConflictMode::Rename:
                case ConflictMode::KeepBoth:
                    clean &= moveEntry(childSrc, uniqueName(childDest), false);
                    continue;
                case ConflictMode::Overwrite:
                case ConflictMode::Merge:
                    removeExisting(childDest);
                    break;
                }
            }
            clean &= moveEntry(childSrc, childDest, false);
        }
        ++m_filesDone;
        if (clean) {
            // The merged source dir is empty now; it was never "created" at
            // the destination, so it only disappears.
            if (!QDir().rmdir(src)) {
                addError(tr("Could not remove %1").arg(src));
                clean = false;
            }
        }
        return clean;
    }

    // Same-device rename fast path (whole entry at once).
    if (sameDevice(src, m_destDir)
        && ::rename(QFile::encodeName(src).constData(), QFile::encodeName(dest).constData())
            == 0) {
        countSubtreeAsDone(src);
        QVariantMap pair;
        pair.insert(QStringLiteral("from"), QDir::cleanPath(src));
        pair.insert(QStringLiteral("to"), dest);
        m_movePairs.append(pair);
        reportProgress(src, dest);
        return true;
    }

    // Cross-device (or rename failure): copy + verify + delete source, and
    // only delete when the whole subtree copied clean.
    const bool clean = copyEntry(src, dest, false);
    if (m_ctl->cancelled.load())
        return false;
    if (!clean) {
        addError(tr("%1 was not fully moved; the original was kept").arg(src));
        return false;
    }
    const QFileInfo sfi(src);
    bool removed;
    if (sfi.isDir() && !sfi.isSymLink())
        removed = QDir(src).removeRecursively();
    else
        removed = QFile::remove(src);
    if (!removed) {
        addError(tr("Could not remove the original %1").arg(src));
        return false;
    }
    QVariantMap pair;
    pair.insert(QStringLiteral("from"), QDir::cleanPath(src));
    pair.insert(QStringLiteral("to"), dest);
    m_movePairs.append(pair);
    return true;
}

// ---------------------------------------------------------------- helpers

void CopyWorker::enumerate(const QString& path, qint64* bytes, int* files) const
{
    const QFileInfo fi(path);
    if (!fi.exists() && !fi.isSymLink())
        return;
    ++*files;
    if (fi.isSymLink())
        return;
    if (fi.isDir()) {
        QDirIterator it(path, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo e = it.fileInfo();
            ++*files;
            if (e.isFile() && !e.isSymLink())
                *bytes += e.size();
        }
    } else if (fi.isFile()) {
        *bytes += fi.size();
    }
}

void CopyWorker::countSubtreeAsDone(const QString& path)
{
    qint64 bytes = 0;
    int files = 0;
    enumerate(path, &bytes, &files);
    m_bytesDone += bytes;
    m_filesDone += files;
    reportProgress(path, QString());
}

void CopyWorker::applyMetadata(const QString& src, const QString& dest)
{
    QFile::setPermissions(dest, QFile::permissions(src));
    preserveTimes(src, dest);
}

void CopyWorker::removeExisting(const QString& path)
{
    const QFileInfo fi(path);
    if (fi.isDir() && !fi.isSymLink())
        QDir(path).removeRecursively();
    else
        QFile::remove(path);
}

void CopyWorker::reportProgress(const QString& src, const QString& dest, bool force)
{
    if (!force && m_progressClock.isValid() && m_progressClock.elapsed() < kProgressIntervalMs)
        return;
    m_progressClock.restart();
    emit progressed(m_bytesDone, m_bytesTotal, m_filesDone, m_filesTotal, src, dest);
}

QString CopyWorker::uniqueName(const QString& destPath)
{
    const QFileInfo fi(destPath);
    const QString dir = fi.absolutePath();
    QString base = fi.completeBaseName();
    QString suffix = fi.suffix();
    if (fi.isDir() && !fi.isSymLink()) { // dirs keep their dots
        base = fi.fileName();
        suffix.clear();
    }
    for (int i = 2;; ++i) {
        const QString name = suffix.isEmpty()
            ? QStringLiteral("%1 (%2)").arg(base).arg(i)
            : QStringLiteral("%1 (%2).%3").arg(base).arg(i).arg(suffix);
        const QString candidate = dir + QLatin1Char('/') + name;
        if (!QFileInfo::exists(candidate) && !QFileInfo(candidate).isSymLink())
            return candidate;
    }
}

bool CopyWorker::sameDevice(const QString& a, const QString& b)
{
    struct stat sa{};
    struct stat sb{};
    if (::lstat(QFile::encodeName(a).constData(), &sa) != 0)
        return false;
    if (::stat(QFile::encodeName(b).constData(), &sb) != 0)
        return false;
    return sa.st_dev == sb.st_dev;
}

} // namespace fang
