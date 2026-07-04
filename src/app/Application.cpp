#include "Application.h"

#include "app/WindowController.h"
#include "config/Theme.h"
#include "ops/FileOperationEngine.h"
#include "places/Places.h"
#include "preview/Previews.h"
#include "util/HumanFormat.h"
#include "util/Logging.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QQmlEngine>

namespace fang {

Application* Application::s_instance = nullptr;

Application::Application(QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    m_theme = new Theme(this);
    m_theme->loadByName(QStringLiteral("dark"));
    connect(m_theme, &Theme::changed, this, &Application::themeChanged);

    m_places = new Places(this);
    connect(m_places, &Places::error, this, &Application::error);

    m_engine = new FileOperationEngine(this);
    m_previews = new Previews(this);
}

Clipboard* Application::clipboard() const
{
    return m_engine->clipboard();
}

UndoStack* Application::undo() const
{
    return m_engine->undoStack();
}

void Application::notify(const QString& title, const QString& detail)
{
    emit error(title, detail);
}

Application::~Application()
{
    s_instance = nullptr;
}

Application* Application::instance()
{
    return s_instance;
}

Application* Application::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    Q_ASSERT(s_instance); // constructed in main() before the engine loads
    QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

QString Application::appVersion() const
{
    return QStringLiteral(FANG_VERSION);
}

void Application::setOverrideError(const QString& text)
{
    if (m_overrideError == text)
        return;
    m_overrideError = text;
    emit overrideErrorChanged();
    if (!text.isEmpty())
        emit error(tr("UI override failed to load"), text);
}

void Application::setStartupPaths(const QStringList& paths)
{
    m_startupPaths = paths;
}

QStringList Application::takeStartupPaths()
{
    QStringList out;
    m_startupPaths.swap(out);
    return out;
}

QString Application::formatSize(qint64 bytes) const
{
    return HumanFormat::size(bytes);
}

WindowController* Application::newWindow()
{
    auto* win = new WindowController(this);
    m_windows.append(win);
    connect(win, &QObject::destroyed, this, [this, win] { m_windows.removeAll(win); });

    const QStringList paths = takeStartupPaths();
    if (paths.isEmpty()) {
        win->addTab(QString());
    } else {
        for (const QString& p : paths)
            win->addTab(p);
        win->setActiveTabIndex(0);
    }
    return win;
}

void Application::openPath(const QString& path)
{
    // Multi-process: every new window is a new independent process.
    const QString exe = QCoreApplication::applicationFilePath();
    if (!QProcess::startDetached(exe, {path}))
        emit error(tr("Could not open new window"), exe);
}

void Application::quit()
{
    QCoreApplication::quit();
}

void Application::reloadTheme()
{
    m_theme->loadByName(m_theme->name());
}

void Application::reloadActions()
{
    // Wired to ActionRegistry once available (stage 8).
}

void Application::reloadConfig()
{
    emit configReloaded();
}

} // namespace fang
