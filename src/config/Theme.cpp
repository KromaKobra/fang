#include "Theme.h"

#include "util/Logging.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <toml++/toml.hpp>

namespace fang {

namespace {

QColor tomlColor(const toml::table* tbl, const char* key, const QColor& fallback)
{
    if (tbl) {
        if (auto v = (*tbl)[key].value<std::string>())
            return QColor(QString::fromStdString(*v));
    }
    return fallback;
}

template <typename T>
T tomlNumber(const toml::table* tbl, const char* key, T fallback)
{
    if (tbl) {
        if (auto v = (*tbl)[key].value<double>())
            return static_cast<T>(*v);
    }
    return fallback;
}

QString tomlString(const toml::table* tbl, const char* key, const QString& fallback)
{
    if (tbl) {
        if (auto v = (*tbl)[key].value<std::string>())
            return QString::fromStdString(*v);
    }
    return fallback;
}

} // namespace

Theme::Theme(QObject* parent)
    : QObject(parent)
{
    m_userThemesDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/fang/themes");
}

void Theme::setUserThemesDir(const QString& dir)
{
    m_userThemesDir = dir;
}

bool Theme::loadByName(const QString& name)
{
    const QStringList candidates = {
        m_userThemesDir + QLatin1Char('/') + name + QStringLiteral(".toml"),
        QStringLiteral(":/Fang/themes/") + name + QStringLiteral(".toml"),
    };
    for (const QString& path : candidates) {
        QFile f(path);
        if (!f.exists())
            continue;
        if (!f.open(QIODevice::ReadOnly)) {
            qCWarning(fangConfig) << "cannot open theme" << path << f.errorString();
            continue;
        }
        if (loadFromData(f.readAll(), name)) {
            qCDebug(fangConfig) << "loaded theme" << name << "from" << path;
            return true;
        }
    }
    qCWarning(fangConfig) << "theme not found or unparsable:" << name;
    return false;
}

bool Theme::loadFromData(const QByteArray& data, const QString& fallbackName)
{
    toml::table root;
    try {
        root = toml::parse(std::string_view(data.constData(), size_t(data.size())));
    } catch (const toml::parse_error& err) {
        qCWarning(fangConfig) << "theme parse error:" << err.description().data();
        return false;
    }

    const toml::table* colors = root["colors"].as_table();
    const toml::table* metrics = root["metrics"].as_table();
    const toml::table* typography = root["typography"].as_table();

    Colors c;
    c.background = tomlColor(colors, "background", c.background);
    c.backgroundAlt = tomlColor(colors, "background_alt", c.backgroundAlt);
    c.surface = tomlColor(colors, "surface", c.surface);
    c.surfaceHover = tomlColor(colors, "surface_hover", c.surfaceHover);
    c.surfaceActive = tomlColor(colors, "surface_active", c.surfaceActive);
    c.foreground = tomlColor(colors, "foreground", c.foreground);
    c.foregroundMuted = tomlColor(colors, "foreground_muted", c.foregroundMuted);
    c.accent = tomlColor(colors, "accent", c.accent);
    c.accentForeground = tomlColor(colors, "accent_foreground", c.accentForeground);
    c.border = tomlColor(colors, "border", c.border);
    c.selectionBackground = tomlColor(colors, "selection_background", c.selectionBackground);
    c.selectionForeground = tomlColor(colors, "selection_foreground", c.selectionForeground);
    c.error = tomlColor(colors, "error", c.error);
    c.warning = tomlColor(colors, "warning", c.warning);
    c.success = tomlColor(colors, "success", c.success);
    c.shadow = tomlColor(colors, "shadow", c.shadow);
    c.scrollbar = tomlColor(colors, "scrollbar", c.scrollbar);
    c.scrollbarHover = tomlColor(colors, "scrollbar_hover", c.scrollbarHover);
    m_colors = c;

    Metrics m;
    m.radiusSmall = tomlNumber(metrics, "radius_small", m.radiusSmall);
    m.radiusMedium = tomlNumber(metrics, "radius_medium", m.radiusMedium);
    m.radiusLarge = tomlNumber(metrics, "radius_large", m.radiusLarge);
    m.spacingSmall = tomlNumber(metrics, "spacing_small", m.spacingSmall);
    m.spacingMedium = tomlNumber(metrics, "spacing_medium", m.spacingMedium);
    m.spacingLarge = tomlNumber(metrics, "spacing_large", m.spacingLarge);
    m.rowHeight = tomlNumber(metrics, "row_height", m.rowHeight);
    m.toolbarHeight = tomlNumber(metrics, "toolbar_height", m.toolbarHeight);
    m.sidebarWidth = tomlNumber(metrics, "sidebar_width", m.sidebarWidth);
    m.iconSizeSmall = tomlNumber(metrics, "icon_size_small", m.iconSizeSmall);
    m.iconSizeMedium = tomlNumber(metrics, "icon_size_medium", m.iconSizeMedium);
    m.iconSizeLarge = tomlNumber(metrics, "icon_size_large", m.iconSizeLarge);
    m_metrics = m;

    Typography t;
    t.fontFamily = tomlString(typography, "font_family", t.fontFamily);
    t.fontFamilyMono = tomlString(typography, "font_family_mono", t.fontFamilyMono);
    t.fontSizeSmall = tomlNumber(typography, "font_size_small", t.fontSizeSmall);
    t.fontSizeNormal = tomlNumber(typography, "font_size_normal", t.fontSizeNormal);
    t.fontSizeLarge = tomlNumber(typography, "font_size_large", t.fontSizeLarge);
    m_typography = t;

    m_name = tomlString(&root, "name", fallbackName);
    m_isDark = root["is_dark"].value<bool>().value_or(m_isDark);

    emit changed();
    return true;
}

void Theme::setFollowSystem(bool follow)
{
    if (m_followSystem == follow)
        return;
    m_followSystem = follow;
    emit changed();
}

void Theme::setFontOverride(const QString& family, int pointSize)
{
    m_fontFamilyOverride = family;
    m_fontSizeOverride = pointSize;
    emit changed();
}

QString Theme::fontFamily() const
{
    if (!m_fontFamilyOverride.isEmpty() && m_fontFamilyOverride != QLatin1String("auto"))
        return m_fontFamilyOverride;
    return m_typography.fontFamily;
}

int Theme::fontSizeNormal() const
{
    return m_fontSizeOverride > 0 ? m_fontSizeOverride : m_typography.fontSizeNormal;
}

int Theme::fontSizeSmall() const
{
    return m_fontSizeOverride > 0 ? m_fontSizeOverride - 1 : m_typography.fontSizeSmall;
}

int Theme::fontSizeLarge() const
{
    return m_fontSizeOverride > 0 ? m_fontSizeOverride + 3 : m_typography.fontSizeLarge;
}

} // namespace fang
