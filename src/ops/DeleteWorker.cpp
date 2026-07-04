#include "DeleteWorker.h"

#include "ops/TrashManager.h"
#include "util/Logging.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QVariantMap>

namespace fang {

namespace {
constexpr int kProgressIntervalMs = 100;

QVariantMap itemToMap(const TrashItem& item)
{
    QVariantMap map;
    map.insert(QStringLiteral("name"), item.name);
    map.insert(QStringLiteral("originalPath"), item.originalPath);
    map.insert(QStringLiteral("filesPath"), item.filesPath);
    map.insert(QStringLiteral("infoPath"), item.infoPath);
    return map;
}

TrashItem mapToItem(const QVariantMap& map)
{
    TrashItem item;
    item.name = map.value(QStringLiteral("name")).toString();
    item.originalPath = map.value(QStringLiteral("originalPath")).toString();
    item.filesPath = map.value(QStringLiteral("filesPath")).toString();
    item.infoPath = map.value(QStringLiteral("infoPath")).toString();
    return item;
}
} // namespace

DeleteWorker::DeleteWorker(Mode mode, const QStringList& paths, const QVariantList& items,
                           std::shared_ptr<OpControl> control, bool preciseList)
    : OpWorker(std::move(control))
    , m_mode(mode)
    , m_paths(paths)
    , m_items(items)
    , m_preciseList(preciseList)
{
}

void DeleteWorker::run()
{
    m_progressClock.start();
    emit stateHint(int(OpState::Running));
    switch (m_mode) {
    case Mode::Remove:
        runRemove();
        break;
    case Mode::Trash:
        runTrash();
        break;
    case Mode::Restore:
        runRestore();
        break;
    }
}

// ----------------------------------------------------------------- remove

void DeleteWorker::runRemove()
{
    if (m_preciseList) {
        m_filesTotal = m_paths.size();
        reportProgress(m_paths.value(0), QString(), true);
        for (const QString& path : m_paths) {
            if (!keepGoing())
                break;
            const QFileInfo fi(path);
            bool ok;
            if (fi.isDir() && !fi.isSymLink())
                ok = QDir().rmdir(path); // only ever empty when the list is complete
            else
                ok = QFile::remove(path) || (!fi.exists() && !fi.isSymLink());
            if (ok)
                ++m_successes;
            else
                addError(tr("Could not delete %1").arg(path));
            ++m_filesDone;
            reportProgress(path);
        }
        reportProgress(QString(), QString(), true);
        emit done(m_errors.isEmpty() || m_successes > 0, errorSummary(), {});
        return;
    }

    QStringList accepted;
    for (const QString& path : m_paths) {
        const QString norm = QDir::cleanPath(path);
        // Never delete / or a mount root (docs/03 §6 safety rules).
        if (norm == QLatin1String("/") || QStorageInfo(norm).rootPath() == norm) {
            addError(tr("Refusing to delete %1").arg(norm));
            continue;
        }
        accepted << norm;
    }

    for (const QString& path : accepted) {
        qint64 bytes = 0;
        const QFileInfo fi(path);
        ++m_filesTotal;
        if (fi.isDir() && !fi.isSymLink()) {
            QDirIterator it(path,
                            QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                ++m_filesTotal;
            }
        }
        Q_UNUSED(bytes)
    }
    reportProgress(accepted.value(0), QString(), true);

    for (const QString& path : accepted) {
        if (!keepGoing())
            break;
        if (removeRecursive(path))
            ++m_successes;
    }
    reportProgress(QString(), QString(), true);
    emit done(m_errors.isEmpty() || m_successes > 0, errorSummary(), {});
}

bool DeleteWorker::removeRecursive(const QString& path)
{
    if (!keepGoing())
        return false;
    const QFileInfo fi(path);
    reportProgress(path);

    if (fi.isDir() && !fi.isSymLink()) {
        bool clean = true;
        const QStringList children = QDir(path).entryList(
            QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        for (const QString& name : children) {
            if (!keepGoing())
                return false;
            clean &= removeRecursive(path + QLatin1Char('/') + name);
        }
        ++m_filesDone;
        if (!clean)
            return false;
        if (!QDir().rmdir(path)) {
            addError(tr("Could not delete %1").arg(path));
            return false;
        }
        return true;
    }

    ++m_filesDone;
    if (!QFile::remove(path) && (fi.exists() || fi.isSymLink())) {
        addError(tr("Could not delete %1").arg(path));
        return false;
    }
    reportProgress(path);
    return true;
}

// ------------------------------------------------------------------ trash

void DeleteWorker::runTrash()
{
    m_filesTotal = m_paths.size();
    reportProgress(m_paths.value(0), QString(), true);

    QVariantList trashedItems;
    for (const QString& path : m_paths) {
        if (!keepGoing())
            break;
        reportProgress(path);
        TrashItem item;
        QString error;
        if (TrashManager::trashFile(path, &item, &error)) {
            trashedItems.append(itemToMap(item));
            ++m_successes;
        } else {
            addError(error);
        }
        ++m_filesDone;
        reportProgress(path);
    }

    QVariantMap undo;
    undo.insert(QStringLiteral("items"), trashedItems);
    reportProgress(QString(), QString(), true);
    emit done(m_errors.isEmpty() || m_successes > 0, errorSummary(), undo);
}

// ---------------------------------------------------------------- restore

void DeleteWorker::runRestore()
{
    m_filesTotal = m_items.size();
    reportProgress(QString(), QString(), true);

    QStringList restoredPaths;
    for (const QVariant& v : m_items) {
        if (!keepGoing())
            break;
        TrashItem item = mapToItem(v.toMap());
        reportProgress(item.filesPath, item.originalPath);

        bool overwrite = false;
        if (!item.originalPath.isEmpty()
            && (QFileInfo::exists(item.originalPath)
                || QFileInfo(item.originalPath).isSymLink())) {
            const ConflictMode mode = askConflict(item.filesPath, item.originalPath);
            if (m_ctl->cancelled.load())
                break;
            switch (mode) {
            case ConflictMode::Skip:
                ++m_filesDone;
                continue;
            case ConflictMode::Rename:
            case ConflictMode::KeepBoth: {
                // Restore beside the existing file under a fresh name: retarget
                // the item and do the move ourselves via a tweaked copy.
                QFileInfo ofi(item.originalPath);
                QString unique = item.originalPath;
                for (int i = 2; QFileInfo::exists(unique) || QFileInfo(unique).isSymLink(); ++i) {
                    const QString suffix = ofi.suffix();
                    const QString base =
                        suffix.isEmpty() ? ofi.fileName() : ofi.completeBaseName();
                    const QString name = suffix.isEmpty()
                        ? QStringLiteral("%1 (%2)").arg(base).arg(i)
                        : QStringLiteral("%1 (%2).%3").arg(base).arg(i).arg(suffix);
                    unique = ofi.absolutePath() + QLatin1Char('/') + name;
                }
                item.originalPath = unique;
                break;
            }
            case ConflictMode::Overwrite:
            case ConflictMode::Merge:
                overwrite = true;
                break;
            }
        }

        QString error;
        if (TrashManager::restore(item, overwrite, &error)) {
            restoredPaths << item.originalPath;
            ++m_successes;
        } else {
            addError(error);
        }
        ++m_filesDone;
        reportProgress(item.filesPath, item.originalPath);
    }

    QVariantMap undo;
    undo.insert(QStringLiteral("restoredPaths"), restoredPaths);
    reportProgress(QString(), QString(), true);
    emit done(m_errors.isEmpty() || m_successes > 0, errorSummary(), undo);
}

void DeleteWorker::reportProgress(const QString& src, const QString& dest, bool force)
{
    if (!force && m_progressClock.isValid() && m_progressClock.elapsed() < kProgressIntervalMs)
        return;
    m_progressClock.restart();
    emit progressed(0, 0, m_filesDone, m_filesTotal, src, dest);
}

} // namespace fang
