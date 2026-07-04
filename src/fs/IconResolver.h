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

    /// Resolves an icon name to a pixmap, walking a fallback chain and
    /// finally painting a generic folder/file glyph so icon-less systems
    /// still get something visible.
    static QPixmap resolve(const QString& iconName, int size);

private:
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
