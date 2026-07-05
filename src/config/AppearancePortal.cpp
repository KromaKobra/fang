#include "AppearancePortal.h"

#include "util/Logging.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>

namespace fang {

namespace {
const auto kService = QStringLiteral("org.freedesktop.portal.Desktop");
const auto kPath = QStringLiteral("/org/freedesktop/portal/desktop");
const auto kInterface = QStringLiteral("org.freedesktop.portal.Settings");
const auto kNamespace = QStringLiteral("org.freedesktop.appearance");
const auto kKey = QStringLiteral("color-scheme");

quint32 unwrapScheme(const QVariant& value)
{
    QVariant v = value;
    // Read returns v(v(u)); SettingChanged delivers v(u).
    while (v.canConvert<QDBusVariant>())
        v = v.value<QDBusVariant>().variant();
    if (v.userType() == qMetaTypeId<QDBusArgument>()) {
        const QDBusArgument arg = v.value<QDBusArgument>();
        QVariant inner;
        arg >> inner;
        v = inner;
    }
    return v.toUInt();
}
} // namespace

AppearancePortal::AppearancePortal(QObject* parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return;

    QDBusInterface portal(kService, kPath, kInterface, bus);
    if (portal.isValid()) {
        QDBusReply<QVariant> reply = portal.call(QStringLiteral("Read"), kNamespace, kKey);
        if (reply.isValid()) {
            m_available = true;
            m_prefersDark = unwrapScheme(reply.value()) != 2; // 2 = prefer light
        }
    }

    bus.connect(kService, kPath, kInterface, QStringLiteral("SettingChanged"), this,
                SLOT(onSettingChanged(QString,QString,QDBusVariant)));
}

void AppearancePortal::onSettingChanged(const QString& ns, const QString& key,
                                        const QDBusVariant& value)
{
    if (ns != kNamespace || key != kKey)
        return;
    m_available = true;
    applyScheme(unwrapScheme(value.variant()));
}

void AppearancePortal::applyScheme(quint32 scheme)
{
    const bool dark = scheme != 2;
    if (dark == m_prefersDark)
        return;
    m_prefersDark = dark;
    emit colorSchemeChanged(dark);
}

} // namespace fang
