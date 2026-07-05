#include "Application.h"

#include "app/PaneController.h"
#include "app/TabController.h"
#include "app/WindowController.h"
#include "config/AppearancePortal.h"
#include "config/ConfigStore.h"
#include "config/Settings.h"
#include "config/Theme.h"
#include "fs/DirectoryModel.h"
#include "fs/IconResolver.h"
#include "ops/FileOperationEngine.h"
#include "places/Places.h"
#include "preview/Previews.h"
#include "preview/ThumbnailGenerator.h"
#include "search/SearchEngine.h"
#include "util/HumanFormat.h"
#include "util/Logging.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QQmlEngine>
#include <QSaveFile>

#include <toml++/toml.hpp>

#include <sstream>

namespace fang {

Application* Application::s_instance = nullptr;

Application::Application(QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    // Config first: theme/appearance and every controller default reads it.
    m_config = new ConfigStore(this);
    connect(m_config, &ConfigStore::error, this, &Application::error);
    m_config->load();
    m_settings = new Settings(m_config, this);

    m_theme = new Theme(this);
    m_theme->setUserThemesDir(ConfigStore::configDir() + QStringLiteral("/themes"));
    applyAppearance();
    connect(m_theme, &Theme::changed, this, &Application::themeChanged);
    connect(m_settings, &Settings::appearanceChanged, this, &Application::applyAppearance);

    applyPreviewSettings();
    connect(m_settings, &Settings::previewsChanged, this, &Application::applyPreviewSettings);

    m_places = new Places(this);
    connect(m_places, &Places::error, this, &Application::error);

    m_engine = new FileOperationEngine(this);
    m_previews = new Previews(this);
    m_search = new SearchEngine(this);

    if (QCoreApplication::instance())
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this,
                &Application::saveSession);
}

void Application::applyAppearance()
{
    const bool follow = m_settings->followSystem();
    if (follow && !m_portal) {
        m_portal = new AppearancePortal(this);
        connect(m_portal, &AppearancePortal::colorSchemeChanged, this,
                [this](bool) { applyAppearance(); });
    } else if (!follow && m_portal) {
        delete m_portal;
        m_portal = nullptr;
    }

    QString themeName = m_settings->theme();
    if (follow && m_portal && m_portal->available())
        themeName = m_portal->prefersDark() ? m_settings->darkTheme() : m_settings->lightTheme();

    m_theme->setFollowSystem(follow);
    // Only override typography when the keys were set explicitly; a theme's
    // own font metrics win otherwise.
    const QString family = m_settings->fontFamily();
    const bool sizeSet =
        m_config->value(QStringLiteral("appearance"), QStringLiteral("font_size")).isValid();
    m_theme->setFontOverride(family == QLatin1String("auto") ? QString() : family,
                             sizeSet ? m_settings->fontSize() : 0);
    IconResolver::setIconTheme(m_settings->iconTheme());
    m_theme->loadByName(themeName);
}

void Application::applyPreviewSettings()
{
    const bool enabled = m_settings->previewsEnabled() && m_settings->previewsImages();
    // Disabling previews collapses the size cap to zero: nothing generates.
    ThumbnailGenerator::setMaxFileSizeBytes(
        enabled ? qint64(m_settings->maxFileSizeMb()) * 1024 * 1024 : 0);
    ThumbnailGenerator::setUseSharedCache(m_settings->useSharedCache());
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
    if (!paths.isEmpty()) {
        for (const QString& p : paths)
            win->addTab(p);
        win->setActiveTabIndex(0);
    } else if (!m_settings->restoreSession() || !restoreSessionInto(win)) {
        win->addTab(QString());
    }
    return win;
}

// ------------------------------------------------------------- session

namespace {

QString sessionFilePath()
{
    return ConfigStore::stateDir() + QLatin1String("/session.toml");
}

toml::table paneToToml(const PaneController* pane)
{
    toml::table t;
    t.insert("location", pane->location().toStdString());
    t.insert("view_mode", PaneController::viewModeToString(pane->viewMode()).toStdString());
    t.insert("sort_role", PaneController::sortRoleToString(pane->sortRole()).toStdString());
    t.insert("sort_descending", pane->sortDescending());
    t.insert("show_hidden", pane->showHidden());
    t.insert("icon_size", int64_t(pane->iconSize()));
    toml::array cols;
    const QStringList columns = pane->columns();
    for (const QString& c : columns)
        cols.push_back(c.toStdString());
    t.insert("columns", std::move(cols));
    return t;
}

void paneFromToml(PaneController* pane, const toml::table& t)
{
    if (auto v = t["location"].value<std::string>())
        pane->open(QString::fromStdString(*v));
    if (auto v = t["view_mode"].value<std::string>())
        pane->setViewMode(PaneController::viewModeFromString(QString::fromStdString(*v)));
    const QString sortRole =
        QString::fromStdString(t["sort_role"].value_or<std::string>("name"));
    pane->setSort(PaneController::sortRoleFromString(sortRole),
                  t["sort_descending"].value_or<bool>(false));
    pane->setShowHidden(t["show_hidden"].value_or<bool>(false));
    pane->setIconSize(int(t["icon_size"].value_or<int64_t>(48)));
    if (const toml::array* cols = t["columns"].as_array()) {
        QStringList columns;
        for (const toml::node& c : *cols)
            if (auto s = c.value<std::string>())
                columns << QString::fromStdString(*s);
        if (!columns.isEmpty())
            pane->setColumns(columns);
    }
}

} // namespace

void Application::saveSession()
{
    if (!m_settings->restoreSession())
        return;

    toml::table root;
    root.insert("schema_version", int64_t(1));
    toml::array windows;
    for (const WindowController* win : std::as_const(m_windows)) {
        toml::table w;
        w.insert("active_tab", int64_t(win->activeTabIndex()));
        if (win->geometry().isValid()) {
            toml::array geo;
            const QRect r = win->geometry();
            geo.push_back(int64_t(r.x()));
            geo.push_back(int64_t(r.y()));
            geo.push_back(int64_t(r.width()));
            geo.push_back(int64_t(r.height()));
            w.insert("geometry", std::move(geo));
        }
        toml::array tabs;
        const QList<TabController*> tabList = win->tabList();
        for (const TabController* tab : tabList) {
            toml::table t;
            t.insert("split", tab->splitView());
            t.insert("active_pane", int64_t(tab->activePane() == tab->rightPane() ? 1 : 0));
            toml::array panes;
            panes.push_back(paneToToml(tab->leftPane()));
            if (tab->splitView() && tab->rightPane())
                panes.push_back(paneToToml(tab->rightPane()));
            t.insert("pane", std::move(panes));
            tabs.push_back(std::move(t));
        }
        w.insert("tab", std::move(tabs));
        windows.push_back(std::move(w));
    }
    root.insert("window", std::move(windows));

    QDir().mkpath(ConfigStore::stateDir());
    QSaveFile out(sessionFilePath());
    if (!out.open(QIODevice::WriteOnly)) {
        qCWarning(fangConfig) << "could not write session" << out.fileName();
        return;
    }
    std::ostringstream ss;
    ss << root;
    const std::string text = ss.str();
    out.write(text.data(), qint64(text.size()));
    out.commit();
}

bool Application::restoreSessionInto(WindowController* win)
{
    QFile f(sessionFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QByteArray data = f.readAll();
    toml::table root;
    try {
        root = toml::parse(std::string_view(data.constData(), size_t(data.size())));
    } catch (const toml::parse_error&) {
        qCWarning(fangConfig) << "malformed session.toml, starting fresh";
        return false;
    }
    const toml::array* windows = root["window"].as_array();
    if (!windows || m_restoredWindows >= int(windows->size()))
        return false;

    const toml::table* w = windows->get(m_restoredWindows)->as_table();
    ++m_restoredWindows;
    if (!w)
        return false;
    const toml::array* tabs = (*w)["tab"].as_array();
    if (!tabs || tabs->empty())
        return false;

    if (const toml::array* geo = (*w)["geometry"].as_array(); geo && geo->size() == 4) {
        const QRect r(int(geo->get(0)->value_or<int64_t>(0)),
                      int(geo->get(1)->value_or<int64_t>(0)),
                      int(geo->get(2)->value_or<int64_t>(0)),
                      int(geo->get(3)->value_or<int64_t>(0)));
        if (r.isValid())
            win->setGeometry(r);
    }

    for (const toml::node& tabNode : *tabs) {
        const toml::table* t = tabNode.as_table();
        if (!t)
            continue;
        const toml::array* panes = (*t)["pane"].as_array();
        if (!panes || panes->empty())
            continue;

        TabController* tab = win->addTab(QString());
        if (const toml::table* left = panes->get(0)->as_table())
            paneFromToml(tab->leftPane(), *left);
        if ((*t)["split"].value_or<bool>(false)) {
            tab->setSplitView(true);
            if (panes->size() > 1 && tab->rightPane()) {
                if (const toml::table* right = panes->get(1)->as_table())
                    paneFromToml(tab->rightPane(), *right);
            }
            if ((*t)["active_pane"].value_or<int64_t>(0) == 1 && tab->rightPane())
                tab->setActivePane(tab->rightPane());
        }
    }
    if (win->tabCount() == 0)
        return false;
    win->setActiveTabIndex(
        qBound(0, int((*w)["active_tab"].value_or<int64_t>(0)), win->tabCount() - 1));
    return true;
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
    m_config->load();
    // Group signals re-run applyAppearance/applyPreviewSettings and every
    // QML binding on Settings.
    m_settings->notifyAllChanged();
    emit configReloaded();
}

} // namespace fang
