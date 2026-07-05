#pragma once

#include <QDBusVariant>
#include <QObject>

namespace fang {

/// Opt-in freedesktop appearance portal listener (docs/03 §15, docs/06):
/// reads org.freedesktop.appearance color-scheme and tracks SettingChanged.
/// Only instantiated while appearance.follow_system is true; absence of the
/// portal degrades gracefully (stays on the configured theme).
class AppearancePortal : public QObject {
    Q_OBJECT

public:
    explicit AppearancePortal(QObject* parent = nullptr);

    /// True when the portal answered the initial Read.
    bool available() const { return m_available; }
    /// 1 = prefer dark, 2 = prefer light, 0 = no preference.
    bool prefersDark() const { return m_prefersDark; }

signals:
    void colorSchemeChanged(bool prefersDark);

private slots:
    void onSettingChanged(const QString& ns, const QString& key, const QDBusVariant& value);

private:
    void applyScheme(quint32 scheme);

    bool m_available = false;
    bool m_prefersDark = true;
};

} // namespace fang
