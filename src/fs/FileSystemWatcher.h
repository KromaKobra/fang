#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QTimer>

namespace fang {

/// Wraps QFileSystemWatcher: coalesces event bursts (debounce ~100 ms),
/// diffs each watched directory against a snapshot, and emits
/// added/removed/changed path deltas. Re-arms watches when a directory is
/// replaced. Only watches explicitly-registered scopes (current dir +
/// expanded tree nodes) to stay under the inotify watch limit.
class FileSystemWatcher : public QObject {
    Q_OBJECT

public:
    explicit FileSystemWatcher(QObject* parent = nullptr);

    void setDebounceMs(int ms) { m_debounceMs = ms; }

    void watchDirectory(const QString& absPath);
    void unwatchDirectory(const QString& absPath);
    void clear();
    QStringList watchedDirectories() const;

signals:
    /// All paths are absolute children of `dir`.
    void added(const QString& dir, const QStringList& paths);
    void removed(const QString& dir, const QStringList& paths);
    void changed(const QString& dir, const QStringList& paths);
    /// The watched directory itself disappeared.
    void directoryGone(const QString& dir);

private:
    struct EntryStat {
        qint64 mtimeMs = 0;
        qint64 size = 0;
        bool operator==(const EntryStat&) const = default;
    };
    using Snapshot = QHash<QString, EntryStat>; // name -> stat

    static Snapshot takeSnapshot(const QString& dir);
    void onDirectoryEvent(const QString& path);
    void onFileEvent(const QString& path);
    void flush();
    void armFileWatches(const QString& dir);

    // Above this many entries we skip per-file watches (content-change
    // detection) and rely on directory-level events only.
    static constexpr int kMaxFileWatchesPerDir = 512;

    QFileSystemWatcher m_watcher;
    QHash<QString, Snapshot> m_snapshots; // dir -> snapshot
    QSet<QString> m_dirty;
    QTimer m_debounce;
    int m_debounceMs = 100;
};

} // namespace fang
