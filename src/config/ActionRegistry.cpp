#include "ActionRegistry.h"

#include "app/Application.h"
#include "config/ConfigStore.h"
#include "config/Settings.h"
#include "fs/DirectoryModel.h"
#include "nav/Selection.h"
#include "util/Logging.h"
#include "util/ShellQuote.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace fang {

namespace {

QList<ActionDef> parseFile(const QString& path, QString* errorOut)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return ActionParser::parse(f.readAll(), errorOut);
}

bool mimeGlobMatches(const QString& glob, const QString& mime)
{
    if (glob == QLatin1String("*") || glob == QLatin1String("*/*"))
        return true;
    if (glob.endsWith(QLatin1String("/*")))
        return mime.startsWith(QStringView(glob).left(glob.size() - 1));
    return glob.compare(mime, Qt::CaseInsensitive) == 0;
}

} // namespace

ActionRegistry::ActionRegistry(QObject* parent)
    : QObject(parent)
{
    reload();
}

void ActionRegistry::reload()
{
    QString parseError;
    QList<ActionDef> merged = parseFile(QStringLiteral(":/Fang/themes/actions.toml"), nullptr);

    const QString userFile = ConfigStore::configDir() + QLatin1String("/actions.toml");
    const QList<ActionDef> user = parseFile(userFile, &parseError);
    if (!parseError.isEmpty())
        emit error(tr("Could not parse actions.toml"), parseError);

    for (const ActionDef& def : user) {
        bool overrode = false;
        for (ActionDef& existing : merged) {
            if (existing.id == def.id) {
                existing = def; // user entry replaces the bundled one in place
                overrode = true;
                break;
            }
        }
        if (!overrode)
            merged << def;
    }

    m_actions = merged;
    qCDebug(fangActions) << "loaded" << m_actions.size() << "actions";
    emit changed();
}

bool ActionRegistry::matches(const ActionDef& def, const Selection* selection) const
{
    if (!def.showInMenu || def.command.isEmpty())
        return false;

    const int count = selection ? selection->count() : 0;
    if (count == 0)
        return def.appliesTo != QLatin1String("files");
    if (def.appliesTo == QLatin1String("background"))
        return false;

    if (def.minSelection > 0 && count < def.minSelection)
        return false;
    if (def.maxSelection > 0 && count > def.maxSelection)
        return false;

    if (def.mimeTypes.isEmpty() && def.extensions.isEmpty())
        return true;

    // Every selected file must match one of the mime globs or extensions.
    QMimeDatabase db;
    const QStringList paths = selection->selectedPaths();
    for (const QString& path : paths) {
        bool ok = false;
        const QString ext = QFileInfo(path).suffix().toLower();
        if (!ext.isEmpty() && def.extensions.contains(ext))
            ok = true;
        if (!ok && !def.mimeTypes.isEmpty()) {
            const QString mime = db.mimeTypeForFile(path).name();
            for (const QString& glob : def.mimeTypes) {
                if (mimeGlobMatches(glob, mime)) {
                    ok = true;
                    break;
                }
            }
        }
        if (!ok)
            return false;
    }
    return true;
}

QVariantList ActionRegistry::actionsFor(const Selection* selection) const
{
    QVariantList out;
    for (const ActionDef& def : m_actions) {
        if (!matches(def, selection))
            continue;
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), def.id);
        entry.insert(QStringLiteral("label"), def.label);
        entry.insert(QStringLiteral("iconName"), def.iconName);
        entry.insert(QStringLiteral("hasSubmenu"), !def.submenu.isEmpty());
        entry.insert(QStringLiteral("submenuLabel"), def.submenu);
        out << entry;
    }
    return out;
}

QString ActionRegistry::expandCommand(const QString& command, const QStringList& paths,
                                      const QString& dir)
{
    QStringList urls;
    urls.reserve(paths.size());
    for (const QString& p : paths)
        urls << QUrl::fromLocalFile(p).toString();
    const QString first = paths.isEmpty() ? QString() : paths.first();

    QString out;
    out.reserve(command.size());
    for (int i = 0; i < command.size(); ++i) {
        const QChar c = command.at(i);
        if (c != QLatin1Char('%') || i + 1 >= command.size()) {
            out += c;
            continue;
        }
        const QChar code = command.at(++i);
        if (code == QLatin1Char('f'))
            out += ShellQuote::quote(first);
        else if (code == QLatin1Char('F'))
            out += ShellQuote::quoteAll(paths);
        else if (code == QLatin1Char('u'))
            out += ShellQuote::quote(urls.isEmpty() ? QString() : urls.first());
        else if (code == QLatin1Char('U'))
            out += ShellQuote::quoteAll(urls);
        else if (code == QLatin1Char('d'))
            out += ShellQuote::quote(dir);
        else if (code == QLatin1Char('n'))
            out += ShellQuote::quote(QFileInfo(first).fileName());
        else if (code == QLatin1Char('%'))
            out += QLatin1Char('%');
        else {
            out += c;
            out += code; // unknown code: keep literally
        }
    }
    return out;
}

QStringList ActionRegistry::terminalPrefix(const QString& configured)
{
    // Emulators that take the command as direct argv vs. behind -e.
    static const QStringList direct{QStringLiteral("kitty"), QStringLiteral("foot"),
                                    QStringLiteral("alacritty")};
    static const QStringList dashE{QStringLiteral("konsole"), QStringLiteral("gnome-terminal"),
                                   QStringLiteral("xterm")};

    if (!configured.isEmpty() && configured != QLatin1String("auto"))
        return QProcess::splitCommand(configured);

    const auto prefixFor = [](const QString& exe) -> QStringList {
        const QString base = QFileInfo(exe).fileName();
        if (base == QLatin1String("wezterm"))
            return {exe, QStringLiteral("start"), QStringLiteral("--")};
        if (dashE.contains(base))
            return {exe, QStringLiteral("-e")};
        return {exe};
    };

    const QString envTerm = qEnvironmentVariable("TERMINAL");
    if (!envTerm.isEmpty()) {
        const QString exe = QStandardPaths::findExecutable(envTerm);
        if (!exe.isEmpty())
            return prefixFor(exe);
    }
    const QStringList candidates = QStringList() << direct << QStringLiteral("wezterm") << dashE;
    for (const QString& name : candidates) {
        const QString exe = QStandardPaths::findExecutable(name);
        if (!exe.isEmpty())
            return prefixFor(exe);
    }
    return {};
}

void ActionRegistry::run(const QString& actionId, const Selection* selection)
{
    const ActionDef* def = nullptr;
    for (const ActionDef& a : std::as_const(m_actions)) {
        if (a.id == actionId) {
            def = &a;
            break;
        }
    }
    if (!def) {
        emit error(tr("Unknown action"), actionId);
        return;
    }

    const QStringList paths = selection ? selection->selectedPaths() : QStringList();
    QString dir = QDir::homePath();
    if (selection && selection->model())
        dir = selection->model()->path();

    const QString expanded = expandCommand(def->command, paths, dir);

    QStringList argv;
    if (def->runInTerminal) {
        const Application* app = Application::instance();
        argv = terminalPrefix(app ? app->settings()->terminalCommand() : QStringLiteral("auto"));
        if (argv.isEmpty()) {
            emit error(tr("No terminal emulator found"),
                       tr("Set general.terminal_command in config.toml"));
            return;
        }
    }
    argv << QStringLiteral("/bin/sh") << QStringLiteral("-c") << expanded;

    qCDebug(fangActions) << "run" << actionId << argv;
    if (!QProcess::startDetached(argv.first(), argv.mid(1), dir))
        emit error(tr("Action failed to start"), def->label);
}

} // namespace fang
