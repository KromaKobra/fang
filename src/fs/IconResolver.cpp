#include "IconResolver.h"

#include "util/Logging.h"

#include <QIcon>
#include <QPainter>
#include <QPainterPath>

namespace fang {

void IconResolver::setIconTheme(const QString& themeName)
{
    if (!themeName.isEmpty() && themeName != QLatin1String("auto"))
        QIcon::setThemeName(themeName);
}

QPixmap IconResolver::resolve(const QString& iconName, int size)
{
    QStringList chain{iconName};
    if (iconName.contains(QLatin1Char('-'))) {
        // e.g. "text-x-cmake" -> "text-x-generic"
        const QStringList parts = iconName.split(QLatin1Char('-'));
        chain << parts.first() + QStringLiteral("-x-generic");
    }
    if (iconName.startsWith(QLatin1String("folder")))
        chain << QStringLiteral("folder");
    chain << QStringLiteral("text-x-generic") << QStringLiteral("application-octet-stream")
          << QStringLiteral("unknown");

    for (const QString& name : std::as_const(chain)) {
        const QIcon icon = QIcon::fromTheme(name);
        if (!icon.isNull()) {
            const QPixmap px = icon.pixmap(size, size);
            if (!px.isNull())
                return px;
        }
    }
    return paintFallback(iconName, size);
}

QPixmap IconResolver::paintFallback(const QString& iconName, int size)
{
    // Minimal generic glyphs so Fang remains usable without any icon theme.
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    const qreal s = size;
    if (iconName.startsWith(QLatin1String("folder"))
        || iconName == QLatin1String("inode-directory")) {
        const QColor fill(0x89, 0xb4, 0xfa);
        QPainterPath path;
        path.addRoundedRect(QRectF(s * 0.08, s * 0.28, s * 0.84, s * 0.55), s * 0.08, s * 0.08);
        path.addRoundedRect(QRectF(s * 0.08, s * 0.18, s * 0.38, s * 0.2), s * 0.06, s * 0.06);
        p.fillPath(path.simplified(), fill);
    } else {
        const QColor fill(0x93, 0x99, 0xb2);
        QPainterPath path;
        path.moveTo(s * 0.22, s * 0.08);
        path.lineTo(s * 0.62, s * 0.08);
        path.lineTo(s * 0.78, s * 0.24);
        path.lineTo(s * 0.78, s * 0.92);
        path.lineTo(s * 0.22, s * 0.92);
        path.closeSubpath();
        p.fillPath(path, fill);
        p.setPen(QPen(QColor(0x1e, 0x1e, 0x2e), qMax<qreal>(1.0, s * 0.02)));
        p.drawLine(QPointF(s * 0.62, s * 0.08), QPointF(s * 0.62, s * 0.24));
        p.drawLine(QPointF(s * 0.62, s * 0.24), QPointF(s * 0.78, s * 0.24));
    }
    return px;
}

QPixmap IconImageProvider::requestPixmap(const QString& id, QSize* size,
                                         const QSize& requestedSize)
{
    const int px = requestedSize.width() > 0 ? requestedSize.width() : 32;
    const QPixmap pixmap = IconResolver::resolve(id, px);
    if (size)
        *size = pixmap.size();
    return pixmap;
}

} // namespace fang
