#include "FileSystemWatcher.h"

#include "util/Logging.h"

#include <QDir>
#include <QFileInfo>

namespace fang {

FileSystemWatcher::FileSystemWatcher(QObject* parent)
    : QObject(parent)
{
    m_debounce.setSingleShot(true);
    connect(&m_debounce, &QTimer::timeout, this, &FileSystemWatcher::flush);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
            &FileSystemWatcher::onDirectoryEvent);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &FileSystemWatcher::onFileEvent);
}

FileSystemWatcher::Snapshot FileSystemWatcher::takeSnapshot(const QString& dir)
{
    Snapshot snap;
    const QFileInfoList entries =
        QDir(dir).entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System
                                | QDir::NoDotAndDotDot);
    snap.reserve(entries.size());
    for (const QFileInfo& fi : entries)
        snap.insert(fi.fileName(), {fi.lastModified().toMSecsSinceEpoch(), fi.size()});
    return snap;
}

void FileSystemWatcher::watchDirectory(const QString& absPath)
{
    if (m_snapshots.contains(absPath))
        return;
    m_snapshots.insert(absPath, takeSnapshot(absPath));
    if (!m_watcher.addPath(absPath))
        qCDebug(fangFs) << "could not watch" << absPath;
    armFileWatches(absPath);
}

void FileSystemWatcher::unwatchDirectory(const QString& absPath)
{
    if (!m_snapshots.contains(absPath))
        return;
    m_snapshots.remove(absPath);
    m_dirty.remove(absPath);
    m_watcher.removePath(absPath);
    const QString prefix = absPath.endsWith(QLatin1Char('/')) ? absPath
                                                              : absPath + QLatin1Char('/');
    QStringList toRemove;
    for (const QString& f : m_watcher.files()) {
        if (f.startsWith(prefix) && !f.mid(prefix.size()).contains(QLatin1Char('/')))
            toRemove << f;
    }
    if (!toRemove.isEmpty())
        m_watcher.removePaths(toRemove);
}

void FileSystemWatcher::clear()
{
    m_snapshots.clear();
    m_dirty.clear();
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
}

QStringList FileSystemWatcher::watchedDirectories() const
{
    return QStringList(m_snapshots.keyBegin(), m_snapshots.keyEnd());
}

void FileSystemWatcher::armFileWatches(const QString& dir)
{
    // Directory-level inotify does not report pure content modifications of
    // children, so watch the files themselves while the count is sane.
    const Snapshot& snap = m_snapshots[dir];
    if (snap.size() > kMaxFileWatchesPerDir)
        return;
    QStringList files;
    const QString prefix = dir.endsWith(QLatin1Char('/')) ? dir : dir + QLatin1Char('/');
    for (auto it = snap.constBegin(); it != snap.constEnd(); ++it)
        files << prefix + it.key();
    if (!files.isEmpty())
        m_watcher.addPaths(files);
}

void FileSystemWatcher::onDirectoryEvent(const QString& path)
{
    m_dirty.insert(path);
    m_debounce.start(m_debounceMs);
}

void FileSystemWatcher::onFileEvent(const QString& path)
{
    const QString dir = QFileInfo(path).absolutePath();
    if (m_snapshots.contains(dir)) {
        m_dirty.insert(dir);
        m_debounce.start(m_debounceMs);
    }
}

void FileSystemWatcher::flush()
{
    const QSet<QString> dirty = std::exchange(m_dirty, {});
    for (const QString& dir : dirty) {
        if (!m_snapshots.contains(dir))
            continue;
        if (!QFileInfo::exists(dir)) {
            m_snapshots.remove(dir);
            m_watcher.removePath(dir);
            emit directoryGone(dir);
            continue;
        }
        // Re-arm: QFileSystemWatcher drops paths that were removed/replaced.
        if (!m_watcher.directories().contains(dir))
            m_watcher.addPath(dir);

        const Snapshot before = m_snapshots.value(dir);
        const Snapshot after = takeSnapshot(dir);
        m_snapshots[dir] = after;

        const QString prefix = dir.endsWith(QLatin1Char('/')) ? dir : dir + QLatin1Char('/');
        QStringList addedPaths, removedPaths, changedPaths;
        for (auto it = after.constBegin(); it != after.constEnd(); ++it) {
            const auto beforeIt = before.constFind(it.key());
            if (beforeIt == before.constEnd()) {
                addedPaths << prefix + it.key();
                if (after.size() <= kMaxFileWatchesPerDir)
                    m_watcher.addPath(prefix + it.key());
            } else if (!(*beforeIt == it.value())) {
                changedPaths << prefix + it.key();
            }
        }
        for (auto it = before.constBegin(); it != before.constEnd(); ++it) {
            if (!after.contains(it.key()))
                removedPaths << prefix + it.key();
        }

        if (!addedPaths.isEmpty())
            emit added(dir, addedPaths);
        if (!removedPaths.isEmpty())
            emit removed(dir, removedPaths);
        if (!changedPaths.isEmpty())
            emit changed(dir, changedPaths);
    }
}

} // namespace fang
