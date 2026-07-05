#pragma once

#include <QObject>
#include <QPixmap>
#include <QQuickImageProvider>

namespace fang {

/// QIcon::fromTheme against the configured XDG icon theme, with fallbacks.
/// Provides pixmaps to QML through the "fang-icon" image provider
/// (image://fang-icon/<icon-name>).
class IconResolver {
public:
    /// Applies appearance.icon_theme ("auto" keeps the system default).
    static void setIconTheme(const QString& themeName);

    /// Resolves an icon name to a pixmap: user overrides from
    /// ~/.config/fang/icons/<name>.svg|png first, then the icon theme with a
    /// fallback chain, finally a painted generic folder/file glyph so
    /// icon-less systems still get something visible.
    static QPixmap resolve(const QString& iconName, int size);

private:
    /// $XDG_CONFIG_HOME/fang/icons/<name>.svg (or .png), empty if absent.
    static QString userIconFile(const QString& iconName);
    static QPixmap paintFallback(const QString& iconName, int size);
};

/// image://fang-icon/<name> — themed icon lookup for QML.
class IconImageProvider : public QQuickImageProvider {
public:
    IconImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
    }

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
};

} // namespace fang
