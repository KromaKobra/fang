#pragma once

#include "config/ActionParser.h"

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace fang {

class Selection;

/// Custom context-menu actions (docs/03 §14, docs/06): bundled defaults
/// merged with the user's actions.toml (same id overrides), filtered per
/// selection, expanded with shell-quoted placeholders and launched detached.
/// Exposed as Application.actions.
class ActionRegistry : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ActionRegistry is owned by Application")

public:
    explicit ActionRegistry(QObject* parent = nullptr);

    /// Re-reads bundled + user actions.toml (Application.reloadActions()).
    void reload();

    /// Menu entries matching the selection: an empty/null selection is the
    /// background context, otherwise the file context. Each entry is a map
    /// { id, label, iconName, hasSubmenu, submenuLabel } per docs/04.
    Q_INVOKABLE QVariantList actionsFor(const fang::Selection* selection) const;

    /// Expands the action's command for the selection and launches it
    /// detached (optionally wrapped in a terminal).
    Q_INVOKABLE void run(const QString& actionId, const fang::Selection* selection);

    /// Placeholder expansion, shell-quoted (security-critical; public for
    /// tests). `paths` are the selected files, `dir` the current directory.
    static QString expandCommand(const QString& command, const QStringList& paths,
                                 const QString& dir);

    /// argv prefix that runs its arguments in a terminal, resolved from
    /// general.terminal_command ("auto" probes $TERMINAL and known
    /// emulators). Empty when none is available.
    static QStringList terminalPrefix(const QString& configured);

    const QList<ActionDef>& actions() const { return m_actions; }

signals:
    /// The action set changed (config reload); menus re-query on popup.
    void changed();
    void error(const QString& title, const QString& detail);

private:
    bool matches(const ActionDef& def, const Selection* selection) const;

    QList<ActionDef> m_actions;
};

} // namespace fang
