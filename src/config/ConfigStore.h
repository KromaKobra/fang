#pragma once

#include <QObject>
#include <QVariant>

#include <memory>

namespace toml {
inline namespace v3 {
class table;
}
} // namespace toml

namespace fang {

/// TOML persistence for config.toml (docs/03 §13): keeps the parsed table
/// so unknown keys survive rewrites, writes atomically, exposes typed
/// access by group/key with documented defaults, and emits granular change
/// signals consumed by Settings/Theme. XDG paths are resolved at call time
/// so tests can redirect them.
class ConfigStore : public QObject {
    Q_OBJECT

public:
    explicit ConfigStore(QObject* parent = nullptr);
    ~ConfigStore() override;

    static QString configDir(); ///< $XDG_CONFIG_HOME/fang
    static QString stateDir();  ///< $XDG_STATE_HOME/fang
    QString configFilePath() const;

    /// Missing file → defaults (no error); malformed TOML → defaults +
    /// error signal, never a crash.
    bool load();
    /// Atomic write (QSaveFile); ensures schema_version is present.
    bool save();

    QVariant value(const QString& group, const QString& key,
                   const QVariant& fallback = {}) const;
    /// Sets, persists and emits groupChanged. No-op when unchanged.
    void setValue(const QString& group, const QString& key, const QVariant& value);

    /// Whole [shortcuts] table as name → sequence.
    QVariantMap shortcutOverrides() const;

signals:
    void groupChanged(const QString& group);
    void error(const QString& title, const QString& detail);

private:
    std::unique_ptr<toml::table> m_root;
};

} // namespace fang
