#pragma once

#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

Q_MOC_INCLUDE("config/Theme.h")
Q_MOC_INCLUDE("app/WindowController.h")
Q_MOC_INCLUDE("places/Places.h")
Q_MOC_INCLUDE("ops/Clipboard.h")
Q_MOC_INCLUDE("ops/UndoStack.h")

namespace fang {

class Clipboard;
class FileOperationEngine;
class Places;
class Theme;
class UndoStack;
class WindowController;

/// App-global QML singleton: version info, config/theme access, window
/// management, global services. See docs/03 §1 and docs/04.
class Application : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int apiVersion READ apiVersion CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(fang::Theme* theme READ theme NOTIFY themeChanged)
    Q_PROPERTY(fang::Places* places READ places CONSTANT)
    Q_PROPERTY(fang::Clipboard* clipboard READ clipboard CONSTANT)
    Q_PROPERTY(fang::UndoStack* undo READ undo CONSTANT)
    /// Non-empty when a user UI override failed to load and Fang fell back to
    /// the bundled UI (shown as a notification banner). Additive to API v1.
    Q_PROPERTY(QString overrideError READ overrideError NOTIFY overrideErrorChanged)

public:
    /// No default argument: a default-constructible type makes the QML
    /// engine construct its own singleton instance instead of calling
    /// create() (QQmlPrivate::singletonConstructionMode prefers the
    /// constructor), which would duplicate the app-owned instance.
    explicit Application(QObject* parent);
    ~Application() override;

    static Application* instance();
    /// Factory used by the declarative QML_SINGLETON registration.
    static Application* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    int apiVersion() const { return 1; }
    QString appVersion() const;
    Theme* theme() const { return m_theme; }
    Places* places() const { return m_places; }
    Clipboard* clipboard() const;
    UndoStack* undo() const;
    FileOperationEngine* engine() const { return m_engine; }

    /// Central user-facing notification entry point: forwards to error().
    void notify(const QString& title, const QString& detail);

    QString overrideError() const { return m_overrideError; }
    void setOverrideError(const QString& text);

    /// CLI paths (already resolved; --select mapped to parent dir) for the
    /// first window. Consumed by newWindow().
    void setStartupPaths(const QStringList& paths);
    QStringList takeStartupPaths();

    /// "18.4 MB" — byte formatting for QML (status bar, capacity bars).
    /// Additive to API v1.
    Q_INVOKABLE QString formatSize(qint64 bytes) const;

    Q_INVOKABLE fang::WindowController* newWindow();
    Q_INVOKABLE void openPath(const QString& path);
    Q_INVOKABLE void quit();
    Q_INVOKABLE void reloadTheme();
    Q_INVOKABLE void reloadActions();
    Q_INVOKABLE void reloadConfig();

signals:
    void themeChanged();
    void configReloaded();
    void error(const QString& title, const QString& detail);
    void overrideErrorChanged();

private:
    static Application* s_instance;

    Theme* m_theme = nullptr;
    Places* m_places = nullptr;
    FileOperationEngine* m_engine = nullptr;
    QString m_overrideError;
    QStringList m_startupPaths;
    QList<WindowController*> m_windows;
};

} // namespace fang
