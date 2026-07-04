#include "MimeResolver.h"

#include "util/Logging.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeType>
#include <QProcess>
#include <QQmlEngine>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

namespace fang {

MimeResolver* MimeResolver::s_instance = nullptr;

MimeResolver::MimeResolver(QObject* parent)
    : QObject(parent)
{
    if (!s_instance)
        s_instance = this;
}

MimeResolver* MimeResolver::instance()
{
    if (!s_instance)
        s_instance = new MimeResolver();
    return s_instance;
}

MimeResolver* MimeResolver::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    MimeResolver* self = instance();
    QQmlEngine::setObjectOwnership(self, QQmlEngine::CppOwnership);
    return self;
}

QString MimeResolver::mimeTypeFor(const QString& path) const
{
    return m_db.mimeTypeForFile(path, QMimeDatabase::MatchDefault).name();
}

QString MimeResolver::commentFor(const QString& mimeType) const
{
    return m_db.mimeTypeForName(mimeType).comment();
}

QString MimeResolver::iconNameFor(const QString& mimeType) const
{
    const QMimeType mt = m_db.mimeTypeForName(mimeType);
    QString icon = mt.iconName();
    if (icon.isEmpty())
        icon = mt.genericIconName();
    return icon;
}

// ------------------------------------------------------------- launching

bool MimeResolver::launchDetached(const QStringList& argv, const QString& workingDir)
{
    if (argv.isEmpty())
        return false;
    const QString exe = QStandardPaths::findExecutable(argv.first());
    if (exe.isEmpty())
        return false;
    return QProcess::startDetached(exe, argv.mid(1), workingDir);
}

bool MimeResolver::openDefault(const QList<QUrl>& urls)
{
    if (urls.isEmpty())
        return false;

    // Preferred launchers per docs/02 (runtime-optional, degrade gracefully).
    if (!QStandardPaths::findExecutable(QStringLiteral("gio")).isEmpty()) {
        QStringList argv{QStringLiteral("gio"), QStringLiteral("open")};
        for (const QUrl& u : urls)
            argv << u.toString();
        if (launchDetached(argv))
            return true;
    }
    if (!QStandardPaths::findExecutable(QStringLiteral("xdg-open")).isEmpty()) {
        bool ok = true;
        for (const QUrl& u : urls)
            ok = launchDetached({QStringLiteral("xdg-open"), u.toString()}) && ok;
        if (ok)
            return true;
    }

    // Resolve the .desktop entry ourselves.
    const QString mime = mimeTypeFor(urls.first().toLocalFile());
    const QString appId = defaultAppId(mime);
    if (!appId.isEmpty() && openWith(urls, appId))
        return true;

    bool any = false;
    for (const QUrl& u : urls)
        any = QDesktopServices::openUrl(u) || any;
    return any;
}

bool MimeResolver::openWith(const QList<QUrl>& urls, const QString& desktopFileId)
{
    const DesktopApp app = desktopAppById(desktopFileId);
    if (!app.valid) {
        qCWarning(fangMime) << "unknown desktop id" << desktopFileId;
        return false;
    }
    const QStringList argv = expandExecLine(app.exec, urls);
    if (argv.isEmpty())
        return false;
    if (launchDetached(argv))
        return true;
    emit const_cast<MimeResolver*>(this)->error(tr("Could not launch application"), app.name);
    return false;
}

// -------------------------------------------------------------- desktop db

QStringList MimeResolver::applicationDirs() const
{
    QStringList dirs;
    const QString dataHome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    dirs << dataHome + QStringLiteral("/applications");
    const QString xdgDataDirs = qEnvironmentVariable("XDG_DATA_DIRS",
                                                     QStringLiteral("/usr/local/share:/usr/share"));
    const QStringList parts = xdgDataDirs.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString& d : parts) {
        const QString appDir = d + QStringLiteral("/applications");
        if (!dirs.contains(appDir))
            dirs << appDir;
    }
    return dirs;
}

MimeResolver::DesktopApp MimeResolver::desktopAppById(const QString& desktopFileId) const
{
    DesktopApp app;
    app.id = desktopFileId;
    QString fileName = desktopFileId;
    if (!fileName.endsWith(QLatin1String(".desktop")))
        fileName += QLatin1String(".desktop");

    QString path;
    for (const QString& dir : applicationDirs()) {
        QString candidate = dir + QLatin1Char('/') + fileName;
        if (QFileInfo::exists(candidate)) {
            path = candidate;
            break;
        }
        // "foo-bar.desktop" may live at applications/foo/bar.desktop
        candidate = dir + QLatin1Char('/') + QString(fileName).replace(QLatin1Char('-'),
                                                                       QLatin1Char('/'));
        if (QFileInfo::exists(candidate)) {
            path = candidate;
            break;
        }
    }
    if (path.isEmpty())
        return app;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return app;

    bool inEntry = false;
    bool hidden = false;
    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.startsWith(QLatin1Char('['))) {
            inEntry = line == QLatin1String("[Desktop Entry]");
            continue;
        }
        if (!inEntry || line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const qsizetype eq = line.indexOf(QLatin1Char('='));
        if (eq < 0)
            continue;
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (key == QLatin1String("Name") && app.name.isEmpty())
            app.name = value;
        else if (key == QLatin1String("Icon"))
            app.iconName = value;
        else if (key == QLatin1String("Exec"))
            app.exec = value;
        else if (key == QLatin1String("Terminal"))
            app.terminal = value.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
        else if (key == QLatin1String("NoDisplay") || key == QLatin1String("Hidden"))
            hidden = hidden || value.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
    }
    app.valid = !app.exec.isEmpty() && !hidden;
    return app;
}

QStringList MimeResolver::expandExecLine(const QString& exec, const QList<QUrl>& urls)
{
    // Tokenize per the desktop entry spec (double quotes + backslash escapes).
    QStringList tokens;
    QString cur;
    bool inQuotes = false;
    for (qsizetype i = 0; i < exec.size(); ++i) {
        const QChar c = exec.at(i);
        if (c == QLatin1Char('"')) {
            inQuotes = !inQuotes;
        } else if (c == QLatin1Char('\\') && i + 1 < exec.size()) {
            cur += exec.at(++i);
        } else if (c == QLatin1Char(' ') && !inQuotes) {
            if (!cur.isEmpty())
                tokens << std::exchange(cur, {});
        } else {
            cur += c;
        }
    }
    if (!cur.isEmpty())
        tokens << cur;

    QStringList argv;
    bool filesInserted = false;
    for (const QString& tok : tokens) {
        if (tok == QLatin1String("%f")) {
            if (!urls.isEmpty())
                argv << urls.first().toLocalFile();
            filesInserted = true;
        } else if (tok == QLatin1String("%F")) {
            for (const QUrl& u : urls)
                argv << u.toLocalFile();
            filesInserted = true;
        } else if (tok == QLatin1String("%u")) {
            if (!urls.isEmpty())
                argv << urls.first().toString();
            filesInserted = true;
        } else if (tok == QLatin1String("%U")) {
            for (const QUrl& u : urls)
                argv << u.toString();
            filesInserted = true;
        } else if (tok == QLatin1String("%i") || tok == QLatin1String("%c")
                   || tok == QLatin1String("%k")) {
            // icon/caption/desktop-file-path field codes: dropped
        } else if (tok == QLatin1String("%%")) {
            argv << QStringLiteral("%");
        } else {
            QString t = tok;
            t.replace(QLatin1String("%%"), QLatin1String("%"));
            argv << t;
        }
    }
    if (!filesInserted && !urls.isEmpty())
        for (const QUrl& u : urls)
            argv << u.toLocalFile();
    return argv;
}

QStringList MimeResolver::appIdsForMime(const QString& mimeType) const
{
    QStringList ids;
    auto addIds = [&ids](const QString& value) {
        const QStringList parts = value.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        for (const QString& p : parts)
            if (!ids.contains(p.trimmed()))
                ids << p.trimmed();
    };

    // mimeapps.list associations (config, then data dirs)
    QStringList listFiles;
    listFiles << QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
            + QStringLiteral("/mimeapps.list");
    for (const QString& dir : applicationDirs())
        listFiles << dir + QStringLiteral("/mimeapps.list");

    for (const QString& lf : std::as_const(listFiles)) {
        QFile f(lf);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        QString section;
        while (!f.atEnd()) {
            const QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.startsWith(QLatin1Char('['))) {
                section = line;
                continue;
            }
            if (section != QLatin1String("[Default Applications]")
                && section != QLatin1String("[Added Associations]"))
                continue;
            if (line.startsWith(mimeType + QLatin1Char('=')))
                addIds(line.mid(mimeType.size() + 1));
        }
    }

    // mimeinfo.cache built by update-desktop-database
    for (const QString& dir : applicationDirs()) {
        QFile f(dir + QStringLiteral("/mimeinfo.cache"));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        while (!f.atEnd()) {
            const QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.startsWith(mimeType + QLatin1Char('=')))
                addIds(line.mid(mimeType.size() + 1));
        }
    }
    return ids;
}

QString MimeResolver::defaultAppId(const QString& mimeType) const
{
    // File-based resolution first (fast, non-blocking, honors XDG env).
    const QString listFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/mimeapps.list");
    QFile f(listFile);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString section;
        while (!f.atEnd()) {
            const QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.startsWith(QLatin1Char('['))) {
                section = line;
                continue;
            }
            if (section == QLatin1String("[Default Applications]")
                && line.startsWith(mimeType + QLatin1Char('='))) {
                const QStringList ids =
                    line.mid(mimeType.size() + 1).split(QLatin1Char(';'), Qt::SkipEmptyParts);
                if (!ids.isEmpty())
                    return ids.first().trimmed();
            }
        }
    }

    // CLI fallback (xdg-mime query default) — short timeout, best-effort.
    const QString xdgMime = QStandardPaths::findExecutable(QStringLiteral("xdg-mime"));
    if (!xdgMime.isEmpty()) {
        QProcess proc;
        proc.start(xdgMime, {QStringLiteral("query"), QStringLiteral("default"), mimeType});
        if (proc.waitForFinished(500)) {
            const QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            if (!out.isEmpty())
                return out;
        } else {
            proc.kill();
        }
    }

    const QStringList ids = appIdsForMime(mimeType);
    return ids.isEmpty() ? QString() : ids.first();
}

QVariantList MimeResolver::appsFor(const QString& mimeType) const
{
    QVariantList out;
    const QString def = defaultAppId(mimeType);
    QStringList ids = appIdsForMime(mimeType);
    if (!def.isEmpty()) {
        ids.removeAll(def);
        ids.prepend(def);
    }
    for (const QString& id : std::as_const(ids)) {
        const DesktopApp app = desktopAppById(id);
        if (!app.valid)
            continue;
        QVariantMap m;
        m.insert(QStringLiteral("id"), app.id);
        m.insert(QStringLiteral("name"), app.name.isEmpty() ? app.id : app.name);
        m.insert(QStringLiteral("iconName"), app.iconName);
        m.insert(QStringLiteral("exec"), app.exec);
        m.insert(QStringLiteral("isDefault"), app.id == def);
        out << m;
    }
    return out;
}

bool MimeResolver::setDefaultApp(const QString& mimeType, const QString& desktopFileId)
{
    const QString cfgDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir().mkpath(cfgDir);
    const QString listFile = cfgDir + QStringLiteral("/mimeapps.list");

    QStringList lines;
    {
        QFile f(listFile);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            while (!f.atEnd())
                lines << QString::fromUtf8(f.readLine()).chopped(0).trimmed();
    }

    const QString entry = mimeType + QLatin1Char('=') + desktopFileId + QLatin1Char(';');
    int sectionStart = lines.indexOf(QStringLiteral("[Default Applications]"));
    if (sectionStart < 0) {
        lines.prepend(entry);
        lines.prepend(QStringLiteral("[Default Applications]"));
    } else {
        bool replaced = false;
        for (int i = sectionStart + 1; i < lines.size(); ++i) {
            if (lines[i].startsWith(QLatin1Char('[')))
                break;
            if (lines[i].startsWith(mimeType + QLatin1Char('='))) {
                lines[i] = entry;
                replaced = true;
                break;
            }
        }
        if (!replaced)
            lines.insert(sectionStart + 1, entry);
    }

    QSaveFile out(listFile);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    out.write(lines.join(QLatin1Char('\n')).toUtf8() + '\n');
    return out.commit();
}

} // namespace fang
