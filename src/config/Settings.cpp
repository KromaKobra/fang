#include "Settings.h"

#include "config/ConfigStore.h"

#include <QDir>

namespace fang {

namespace {
const auto kGeneral = QStringLiteral("general");
const auto kView = QStringLiteral("view");
const auto kBehavior = QStringLiteral("behavior");
const auto kPreviews = QStringLiteral("previews");
const auto kSearch = QStringLiteral("search");
const auto kAppearance = QStringLiteral("appearance");
} // namespace

Settings::Settings(ConfigStore* store, QObject* parent)
    : QObject(parent)
    , m_store(store)
{
    connect(m_store, &ConfigStore::groupChanged, this, [this](const QString& group) {
        if (group == kGeneral)
            emit generalChanged();
        else if (group == kView)
            emit viewChanged();
        else if (group == kBehavior)
            emit behaviorChanged();
        else if (group == kPreviews)
            emit previewsChanged();
        else if (group == kSearch)
            emit searchChanged();
        else if (group == kAppearance)
            emit appearanceChanged();
        else if (group == QLatin1String("shortcuts"))
            emit shortcutsChanged();
        emit changed();
    });
}

// -------------------------------------------------------------- [general]

QString Settings::defaultLocation() const
{
    QString v = m_store->value(kGeneral, QStringLiteral("default_location"),
                               QStringLiteral("~")).toString();
    if (v == QLatin1String("~") || v.startsWith(QLatin1String("~/")))
        v = QDir::homePath() + v.mid(1);
    return v;
}
void Settings::setDefaultLocation(const QString& v)
{
    m_store->setValue(kGeneral, QStringLiteral("default_location"), v);
}

bool Settings::restoreSession() const
{
    return m_store->value(kGeneral, QStringLiteral("restore_session"), true).toBool();
}
void Settings::setRestoreSession(bool v)
{
    m_store->setValue(kGeneral, QStringLiteral("restore_session"), v);
}

bool Settings::confirmDelete() const
{
    return m_store->value(kGeneral, QStringLiteral("confirm_delete"), true).toBool();
}
void Settings::setConfirmDelete(bool v)
{
    m_store->setValue(kGeneral, QStringLiteral("confirm_delete"), v);
}

bool Settings::confirmTrash() const
{
    return m_store->value(kGeneral, QStringLiteral("confirm_trash"), false).toBool();
}
void Settings::setConfirmTrash(bool v)
{
    m_store->setValue(kGeneral, QStringLiteral("confirm_trash"), v);
}

QString Settings::terminalCommand() const
{
    return m_store->value(kGeneral, QStringLiteral("terminal_command"),
                          QStringLiteral("auto")).toString();
}
void Settings::setTerminalCommand(const QString& v)
{
    m_store->setValue(kGeneral, QStringLiteral("terminal_command"), v);
}

// ----------------------------------------------------------------- [view]

QString Settings::defaultMode() const
{
    return m_store->value(kView, QStringLiteral("default_mode"),
                          QStringLiteral("details")).toString();
}
void Settings::setDefaultMode(const QString& v)
{
    m_store->setValue(kView, QStringLiteral("default_mode"), v);
}

bool Settings::showHidden() const
{
    return m_store->value(kView, QStringLiteral("show_hidden"), false).toBool();
}
void Settings::setShowHidden(bool v)
{
    m_store->setValue(kView, QStringLiteral("show_hidden"), v);
}

bool Settings::foldersFirst() const
{
    return m_store->value(kView, QStringLiteral("folders_first"), true).toBool();
}
void Settings::setFoldersFirst(bool v)
{
    m_store->setValue(kView, QStringLiteral("folders_first"), v);
}

QString Settings::sortRole() const
{
    return m_store->value(kView, QStringLiteral("sort_role"), QStringLiteral("name")).toString();
}
void Settings::setSortRole(const QString& v)
{
    m_store->setValue(kView, QStringLiteral("sort_role"), v);
}

bool Settings::sortDescending() const
{
    return m_store->value(kView, QStringLiteral("sort_descending"), false).toBool();
}
void Settings::setSortDescending(bool v)
{
    m_store->setValue(kView, QStringLiteral("sort_descending"), v);
}

bool Settings::caseSensitiveSort() const
{
    return m_store->value(kView, QStringLiteral("case_sensitive_sort"), false).toBool();
}
void Settings::setCaseSensitiveSort(bool v)
{
    m_store->setValue(kView, QStringLiteral("case_sensitive_sort"), v);
}

int Settings::iconSize() const
{
    return m_store->value(kView, QStringLiteral("icon_size"), 48).toInt();
}
void Settings::setIconSize(int v)
{
    m_store->setValue(kView, QStringLiteral("icon_size"), v);
}

QStringList Settings::columns() const
{
    const QVariant v = m_store->value(kView, QStringLiteral("columns"));
    if (!v.isValid())
        return {QStringLiteral("name"), QStringLiteral("size"), QStringLiteral("type"),
                QStringLiteral("modified")};
    QStringList out;
    const QVariantList list = v.toList();
    for (const QVariant& e : list)
        out << e.toString();
    return out;
}
void Settings::setColumns(const QStringList& v)
{
    m_store->setValue(kView, QStringLiteral("columns"), v);
}

QVariantMap Settings::columnWidths() const
{
    const QVariant v = m_store->value(kView, QStringLiteral("column_widths"));
    if (v.typeId() == QMetaType::QVariantMap)
        return v.toMap();
    return {{QStringLiteral("name"), 320}, {QStringLiteral("size"), 90},
            {QStringLiteral("type"), 160}, {QStringLiteral("modified"), 160}};
}
void Settings::setColumnWidths(const QVariantMap& v)
{
    m_store->setValue(kView, QStringLiteral("column_widths"), v);
}

bool Settings::previewPane() const
{
    return m_store->value(kView, QStringLiteral("preview_pane"), false).toBool();
}
void Settings::setPreviewPane(bool v)
{
    m_store->setValue(kView, QStringLiteral("preview_pane"), v);
}

int Settings::previewPaneWidth() const
{
    return m_store->value(kView, QStringLiteral("preview_pane_width"), 280).toInt();
}
void Settings::setPreviewPaneWidth(int v)
{
    m_store->setValue(kView, QStringLiteral("preview_pane_width"), v);
}

// ------------------------------------------------------------- [behavior]

QString Settings::openWith() const
{
    return m_store->value(kBehavior, QStringLiteral("open_with"),
                          QStringLiteral("double")).toString();
}
void Settings::setOpenWith(const QString& v)
{
    m_store->setValue(kBehavior, QStringLiteral("open_with"), v);
}

QString Settings::selectWith() const
{
    return m_store->value(kBehavior, QStringLiteral("select_with"),
                          QStringLiteral("single")).toString();
}
void Settings::setSelectWith(const QString& v)
{
    m_store->setValue(kBehavior, QStringLiteral("select_with"), v);
}

QString Settings::typeAhead() const
{
    return m_store->value(kBehavior, QStringLiteral("type_ahead"),
                          QStringLiteral("filter")).toString();
}
void Settings::setTypeAhead(const QString& v)
{
    m_store->setValue(kBehavior, QStringLiteral("type_ahead"), v);
}

QString Settings::dragDefault() const
{
    return m_store->value(kBehavior, QStringLiteral("drag_default"),
                          QStringLiteral("auto")).toString();
}
void Settings::setDragDefault(const QString& v)
{
    m_store->setValue(kBehavior, QStringLiteral("drag_default"), v);
}

QString Settings::newTabPosition() const
{
    return m_store->value(kBehavior, QStringLiteral("new_tab_position"),
                          QStringLiteral("after")).toString();
}
void Settings::setNewTabPosition(const QString& v)
{
    m_store->setValue(kBehavior, QStringLiteral("new_tab_position"), v);
}

bool Settings::closeConfirmMultipleTabs() const
{
    return m_store->value(kBehavior, QStringLiteral("close_confirm_multiple_tabs"), true)
        .toBool();
}
void Settings::setCloseConfirmMultipleTabs(bool v)
{
    m_store->setValue(kBehavior, QStringLiteral("close_confirm_multiple_tabs"), v);
}

// ------------------------------------------------------------- [previews]

bool Settings::previewsEnabled() const
{
    return m_store->value(kPreviews, QStringLiteral("enabled"), true).toBool();
}
void Settings::setPreviewsEnabled(bool v)
{
    m_store->setValue(kPreviews, QStringLiteral("enabled"), v);
}

bool Settings::previewsImages() const
{
    return m_store->value(kPreviews, QStringLiteral("images"), true).toBool();
}
void Settings::setPreviewsImages(bool v)
{
    m_store->setValue(kPreviews, QStringLiteral("images"), v);
}

int Settings::maxFileSizeMb() const
{
    return m_store->value(kPreviews, QStringLiteral("max_file_size_mb"), 64).toInt();
}
void Settings::setMaxFileSizeMb(int v)
{
    m_store->setValue(kPreviews, QStringLiteral("max_file_size_mb"), v);
}

int Settings::textPeekKb() const
{
    return m_store->value(kPreviews, QStringLiteral("text_peek_kb"), 128).toInt();
}
void Settings::setTextPeekKb(int v)
{
    m_store->setValue(kPreviews, QStringLiteral("text_peek_kb"), v);
}

bool Settings::useSharedCache() const
{
    return m_store->value(kPreviews, QStringLiteral("use_shared_cache"), true).toBool();
}
void Settings::setUseSharedCache(bool v)
{
    m_store->setValue(kPreviews, QStringLiteral("use_shared_cache"), v);
}

// --------------------------------------------------------------- [search]

bool Settings::contentSearchDefault() const
{
    return m_store->value(kSearch, QStringLiteral("content_search_default"), false).toBool();
}
void Settings::setContentSearchDefault(bool v)
{
    m_store->setValue(kSearch, QStringLiteral("content_search_default"), v);
}

bool Settings::searchFollowSymlinks() const
{
    return m_store->value(kSearch, QStringLiteral("follow_symlinks"), false).toBool();
}
void Settings::setSearchFollowSymlinks(bool v)
{
    m_store->setValue(kSearch, QStringLiteral("follow_symlinks"), v);
}

int Settings::searchMaxDepth() const
{
    return m_store->value(kSearch, QStringLiteral("max_depth"), 0).toInt();
}
void Settings::setSearchMaxDepth(int v)
{
    m_store->setValue(kSearch, QStringLiteral("max_depth"), v);
}

bool Settings::searchIgnoreHidden() const
{
    return m_store->value(kSearch, QStringLiteral("ignore_hidden"), true).toBool();
}
void Settings::setSearchIgnoreHidden(bool v)
{
    m_store->setValue(kSearch, QStringLiteral("ignore_hidden"), v);
}

QStringList Settings::searchIgnorePaths() const
{
    const QVariant v = m_store->value(kSearch, QStringLiteral("ignore_paths"));
    if (!v.isValid())
        return {QStringLiteral("/proc"), QStringLiteral("/sys"), QStringLiteral("/dev"),
                QStringLiteral("/run")};
    QStringList out;
    const QVariantList list = v.toList();
    for (const QVariant& e : list)
        out << e.toString();
    return out;
}
void Settings::setSearchIgnorePaths(const QStringList& v)
{
    m_store->setValue(kSearch, QStringLiteral("ignore_paths"), v);
}

// ----------------------------------------------------------- [appearance]

QString Settings::theme() const
{
    return m_store->value(kAppearance, QStringLiteral("theme"), QStringLiteral("dark"))
        .toString();
}
void Settings::setTheme(const QString& v)
{
    m_store->setValue(kAppearance, QStringLiteral("theme"), v);
}

bool Settings::followSystem() const
{
    return m_store->value(kAppearance, QStringLiteral("follow_system"), false).toBool();
}
void Settings::setFollowSystem(bool v)
{
    m_store->setValue(kAppearance, QStringLiteral("follow_system"), v);
}

QString Settings::lightTheme() const
{
    return m_store->value(kAppearance, QStringLiteral("light_theme"), QStringLiteral("light"))
        .toString();
}
void Settings::setLightTheme(const QString& v)
{
    m_store->setValue(kAppearance, QStringLiteral("light_theme"), v);
}

QString Settings::darkTheme() const
{
    return m_store->value(kAppearance, QStringLiteral("dark_theme"), QStringLiteral("dark"))
        .toString();
}
void Settings::setDarkTheme(const QString& v)
{
    m_store->setValue(kAppearance, QStringLiteral("dark_theme"), v);
}

QString Settings::iconTheme() const
{
    return m_store->value(kAppearance, QStringLiteral("icon_theme"), QStringLiteral("auto"))
        .toString();
}
void Settings::setIconTheme(const QString& v)
{
    m_store->setValue(kAppearance, QStringLiteral("icon_theme"), v);
}

QString Settings::fontFamily() const
{
    return m_store->value(kAppearance, QStringLiteral("font_family"), QStringLiteral("auto"))
        .toString();
}
void Settings::setFontFamily(const QString& v)
{
    m_store->setValue(kAppearance, QStringLiteral("font_family"), v);
}

int Settings::fontSize() const
{
    return m_store->value(kAppearance, QStringLiteral("font_size"), 10).toInt();
}
void Settings::setFontSize(int v)
{
    m_store->setValue(kAppearance, QStringLiteral("font_size"), v);
}

// -------------------------------------------------------------- shortcuts

QString Settings::shortcut(const QString& name, const QString& fallback) const
{
    const QVariantMap overrides = m_store->shortcutOverrides();
    const QString v = overrides.value(name).toString();
    return v.isEmpty() ? fallback : v;
}

void Settings::notifyAllChanged()
{
    emit generalChanged();
    emit viewChanged();
    emit behaviorChanged();
    emit previewsChanged();
    emit searchChanged();
    emit appearanceChanged();
    emit shortcutsChanged();
    emit changed();
}

} // namespace fang
