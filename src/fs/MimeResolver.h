#pragma once

#include <QMimeDatabase>
#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace fang {

/// QMimeDatabase wrapper: content+name detection, human descriptions,
/// default-app lookup and launching via xdg/gio with .desktop parsing as
/// fallback (docs/03 §12). Exposed to QML as a singleton for the open-with
/// UI; also used internally by controllers.
class MimeResolver : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    /// No default argument: keeps the type non-default-constructible so the
    /// QML engine uses create() instead of building a second instance.
    explicit MimeResolver(QObject* parent);

    static MimeResolver* instance();
    static MimeResolver* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    /// Content+name detection (may read the file).
    Q_INVOKABLE QString mimeTypeFor(const QString& path) const;
    Q_INVOKABLE QString commentFor(const QString& mimeType) const;
    Q_INVOKABLE QString iconNameFor(const QString& mimeType) const;

    /// Launch the default application, detached. Returns false when nothing
    /// could be launched.
    Q_INVOKABLE bool openDefault(const QList<QUrl>& urls);
    /// Launch a specific application (.desktop id, e.g. "org.gnome.gedit.desktop").
    Q_INVOKABLE bool openWith(const QList<QUrl>& urls, const QString& desktopFileId);
    /// Apps able to open `mimeType`: list of { id, name, iconName, exec, isDefault }.
    Q_INVOKABLE QVariantList appsFor(const QString& mimeType) const;
    /// Default .desktop id for a mime type ("" if unknown).
    Q_INVOKABLE QString defaultAppId(const QString& mimeType) const;
    /// Persist a new default application for a mime type.
    Q_INVOKABLE bool setDefaultApp(const QString& mimeType, const QString& desktopFileId);

    struct DesktopApp {
        QString id;      // desktop file id
        QString name;
        QString iconName;
        QString exec;    // raw Exec line
        bool terminal = false;
        bool valid = false;
    };

    /// Parses a .desktop file by id across $XDG_DATA_DIRS/applications.
    DesktopApp desktopAppById(const QString& desktopFileId) const;
    /// Expands Exec field codes (%f %F %u %U, strips %i %c %k) into argv.
    static QStringList expandExecLine(const QString& exec, const QList<QUrl>& urls);

signals:
    void error(const QString& title, const QString& detail);

private:
    QStringList applicationDirs() const;
    QStringList appIdsForMime(const QString& mimeType) const;
    bool launchDetached(const QStringList& argv, const QString& workingDir = {});

    QMimeDatabase m_db;
    static MimeResolver* s_instance;
};

} // namespace fang
