#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

namespace fang {

/// One entry in a trash directory (parsed .trashinfo + files/ path).
struct TrashItem {
    QString name;         ///< entry name inside files/ (and info/<name>.trashinfo)
    QString originalPath; ///< empty when unknown/corrupt
    QDateTime deletionDate;
    QString filesPath; ///< absolute path of the trashed file/dir
    QString infoPath;  ///< absolute path of the .trashinfo (may be empty)
    bool corrupt = false;
};

/// freedesktop Trash spec v1.0 (docs/03 §7). Stateless: all statics; XDG env
/// is read at call time so tests can redirect it per-case.
class TrashManager {
public:
    /// $XDG_DATA_HOME/Trash (the home trash), not necessarily existing yet.
    static QString homeTrashDir();

    /// Trash directory responsible for `path`: the home trash when on the
    /// same device, else $topdir/.Trash/$uid (validated sticky, non-symlink)
    /// or $topdir/.Trash-$uid. Empty + error set when none can be created.
    static QString trashDirFor(const QString& path, QString* error = nullptr);

    /// Moves `path` into the trash. Reserves a unique name by O_EXCL-creating
    /// info/<name>.trashinfo first (collisions get name.2, name.3, ...).
    static bool trashFile(const QString& path, TrashItem* itemOut, QString* error = nullptr);

    /// Parses info/*.trashinfo in `trashDir` (default: home trash). Corrupt
    /// info files yield flagged items; files/ entries without info become
    /// "unknown origin" items. Never throws/crashes.
    static QList<TrashItem> list(const QString& trashDir = QString());

    /// Moves the item back to its original path; refuses when the origin
    /// exists unless `overwrite`.
    static bool restore(const TrashItem& item, bool overwrite, QString* error = nullptr);

    /// Permanently deletes one trashed item (files/ entry + info file).
    static bool removeItem(const TrashItem& item, QString* error = nullptr);

    /// Empties a whole trash directory (default: home trash).
    static bool emptyAll(const QString& trashDir = QString(), QString* error = nullptr);

    /// Number of entries in files/ (trash badge).
    static int itemCount(const QString& trashDir = QString());
};

} // namespace fang
