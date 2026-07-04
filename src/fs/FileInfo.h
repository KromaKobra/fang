#pragma once

#include <QDateTime>
#include <QString>

namespace fang {

/// Value type: one directory entry with cached stat/mime/icon data.
/// Built off the GUI thread by scan workers (docs/01 threading model).
class FileInfo {
public:
    FileInfo() = default;

    /// Stats `path` (lstat semantics for symlink detection). `withMime`
    /// controls whether mime/type/icon are resolved (extension-based match;
    /// cheap enough for listings).
    static FileInfo fromPath(const QString& path, bool withMime = true);

    QString name;
    QString path; // absolute
    QString extension;
    bool isDir = false;
    bool isSymlink = false;
    bool isHidden = false;
    bool exists = false;
    qint64 size = -1; // bytes; -1 for dirs unless computed
    QDateTime modified;
    QDateTime created;
    QDateTime accessed;
    QString mimeType;
    QString mimeComment;
    QString permissions; // "rwxr-xr-x"
    QString owner;
    QString group;
    QString iconName;

    QString url() const; // file:// URL
    bool isImage() const;
};

} // namespace fang
