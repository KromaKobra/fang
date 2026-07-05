#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

namespace fang {

/// One [[action]] entry from actions.toml (docs/06).
struct ActionDef {
    QString id;
    QString label;
    QString iconName;
    QString command;                        // template with %f %F %u %U %d %n %%
    QString appliesTo = QStringLiteral("files"); // files | background | both
    QString submenu;                        // group label; empty = top level
    QStringList mimeTypes;                  // globs like "text/*"
    QStringList extensions;                 // lowercase, no dot
    int minSelection = 0;                   // 0 = no bound
    int maxSelection = 0;                   // 0 = no bound
    bool showInMenu = true;
    bool runInTerminal = false;
};

/// Parses actions.toml text into ActionDefs (docs/03 §14). Entries without
/// an id or command are dropped; a malformed file yields an empty list and
/// an error message, never a crash.
class ActionParser {
public:
    static QList<ActionDef> parse(const QByteArray& data, QString* errorOut = nullptr);
};

} // namespace fang
