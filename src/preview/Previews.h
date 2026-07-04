#pragma once

#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace fang {

/// Preview-pane data controller (docs/01): text peeks, per-file metadata
/// and async recursive dir sizes — QML never touches the filesystem
/// directly. Additive to API v1, exposed as Application.previews.
class Previews : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Previews is owned by the application")

public:
    explicit Previews(QObject* parent = nullptr);

    /// First `maxKb` KiB of a text file (previews.text_peek_kb); empty for
    /// binaries (null-byte heuristic) or unreadable files.
    Q_INVOKABLE QString readTextPeek(const QString& path, int maxKb) const;
    Q_INVOKABLE bool isProbablyText(const QString& path) const;

    /// Stat/mime metadata for one path: name, size, sizeText, isDir,
    /// isSymlink, isImage, mimeType, typeText, modified, accessed, created,
    /// permissions, owner, group, iconName, exists.
    Q_INVOKABLE QVariantMap fileDetails(const QString& path) const;

    /// Recursive dir size on a worker; result via dirSizeComputed.
    Q_INVOKABLE void computeDirSize(const QString& path);

signals:
    void dirSizeComputed(const QString& path, qlonglong bytes, int files);
};

} // namespace fang
