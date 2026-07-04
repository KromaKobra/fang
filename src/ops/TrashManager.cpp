#include "TrashManager.h"

#include "util/Logging.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QUrl>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

namespace fang {

namespace {

// st_dev of the deepest existing ancestor (the path itself when it exists).
bool deviceOf(const QString& path, dev_t* dev)
{
    QString p = QDir::cleanPath(path);
    while (!p.isEmpty()) {
        struct stat st{};
        if (::lstat(QFile::encodeName(p).constData(), &st) == 0) {
            *dev = st.st_dev;
            return true;
        }
        const int slash = p.lastIndexOf(QLatin1Char('/'));
        if (slash <= 0) {
            p = QStringLiteral("/");
            struct stat root{};
            if (::stat("/", &root) == 0) {
                *dev = root.st_dev;
                return true;
            }
            return false;
        }
        p.truncate(slash);
    }
    return false;
}

bool ensureTrashSubdirs(const QString& trashDir)
{
    QDir dir;
    return dir.mkpath(trashDir + QLatin1String("/files"))
        && dir.mkpath(trashDir + QLatin1String("/info"));
}

QString decodeTrashInfoPath(const QByteArray& value)
{
    return QUrl::fromPercentEncoding(value);
}

} // namespace

QString TrashManager::homeTrashDir()
{
    // QStandardPaths reads $XDG_DATA_HOME at call time (test redirection).
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QLatin1String("/Trash");
}

QString TrashManager::trashDirFor(const QString& path, QString* error)
{
    dev_t pathDev = 0;
    dev_t homeDev = 0;
    const QString dataHome =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (!deviceOf(path, &pathDev) || !deviceOf(dataHome, &homeDev)) {
        if (error)
            *error = QObject::tr("Cannot stat %1").arg(path);
        return QString();
    }
    if (pathDev == homeDev) {
        const QString home = homeTrashDir();
        if (!ensureTrashSubdirs(home)) {
            if (error)
                *error = QObject::tr("Cannot create trash directory %1").arg(home);
            return QString();
        }
        return home;
    }

    const QString topdir = QStorageInfo(path).rootPath();
    const uint uid = ::getuid();

    // Admin-created $topdir/.Trash: must be a sticky, non-symlink directory.
    const QString adminTrash = topdir + QLatin1String("/.Trash");
    const QFileInfo adminInfo(adminTrash);
    if (adminInfo.exists() && adminInfo.isDir() && !adminInfo.isSymLink()) {
        struct stat st{};
        if (::lstat(QFile::encodeName(adminTrash).constData(), &st) == 0
            && (st.st_mode & S_ISVTX)) {
            const QString mine = adminTrash + QLatin1Char('/') + QString::number(uid);
            if (ensureTrashSubdirs(mine))
                return mine;
        }
    }

    const QString userTrash = topdir + QLatin1String("/.Trash-") + QString::number(uid);
    if (ensureTrashSubdirs(userTrash))
        return userTrash;

    if (error)
        *error = QObject::tr("No usable trash directory on the volume of %1").arg(path);
    return QString();
}

bool TrashManager::trashFile(const QString& path, TrashItem* itemOut, QString* error)
{
    const QFileInfo fi(path);
    if (!fi.exists() && !fi.isSymLink()) {
        if (error)
            *error = QObject::tr("No such file: %1").arg(path);
        return false;
    }

    const QString trashDir = trashDirFor(path, error);
    if (trashDir.isEmpty())
        return false;

    // Reserve a unique name via O_EXCL create of the .trashinfo.
    const QString base = fi.fileName();
    QString name;
    QFile infoFile;
    for (int attempt = 1; attempt < 10000; ++attempt) {
        name = attempt == 1 ? base : base + QLatin1Char('.') + QString::number(attempt);
        infoFile.setFileName(trashDir + QLatin1String("/info/") + name
                             + QLatin1String(".trashinfo"));
        if (infoFile.open(QIODevice::WriteOnly | QIODevice::NewOnly))
            break;
        infoFile.setFileName(QString());
    }
    if (infoFile.fileName().isEmpty()) {
        if (error)
            *error = QObject::tr("Could not reserve a trash slot for %1").arg(path);
        return false;
    }

    const QString absPath = fi.absoluteFilePath();
    QByteArray content = "[Trash Info]\n";
    content += "Path=" + QUrl::toPercentEncoding(absPath, "/") + "\n";
    content += "DeletionDate="
        + QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8() + "\n";
    if (infoFile.write(content) != content.size()) {
        infoFile.close();
        infoFile.remove();
        if (error)
            *error = QObject::tr("Could not write %1").arg(infoFile.fileName());
        return false;
    }
    infoFile.close();

    const QString destPath = trashDir + QLatin1String("/files/") + name;
    if (::rename(QFile::encodeName(absPath).constData(),
                 QFile::encodeName(destPath).constData()) != 0) {
        const QString why = QString::fromLocal8Bit(::strerror(errno));
        infoFile.remove();
        if (error)
            *error = QObject::tr("Could not move %1 to trash: %2").arg(absPath, why);
        return false;
    }

    if (itemOut) {
        itemOut->name = name;
        itemOut->originalPath = absPath;
        itemOut->deletionDate = QDateTime::currentDateTime();
        itemOut->filesPath = destPath;
        itemOut->infoPath = infoFile.fileName();
        itemOut->corrupt = false;
    }
    return true;
}

QList<TrashItem> TrashManager::list(const QString& trashDir)
{
    const QString dir = trashDir.isEmpty() ? homeTrashDir() : trashDir;
    const QString filesDir = dir + QLatin1String("/files");
    const QString infoDir = dir + QLatin1String("/info");

    QList<TrashItem> items;
    QSet<QString> seen;

    const QFileInfoList infos = QDir(infoDir).entryInfoList(
        {QStringLiteral("*.trashinfo")}, QDir::Files | QDir::Hidden);
    for (const QFileInfo& infoFi : infos) {
        TrashItem item;
        item.name = infoFi.completeBaseName(); // drops only the ".trashinfo" suffix
        item.infoPath = infoFi.absoluteFilePath();
        item.filesPath = filesDir + QLatin1Char('/') + item.name;

        QFile f(item.infoPath);
        bool headerSeen = false;
        if (f.open(QIODevice::ReadOnly)) {
            while (!f.atEnd()) {
                const QByteArray line = f.readLine().trimmed();
                if (line == "[Trash Info]") {
                    headerSeen = true;
                } else if (line.startsWith("Path=")) {
                    item.originalPath = decodeTrashInfoPath(line.mid(5));
                } else if (line.startsWith("DeletionDate=")) {
                    item.deletionDate =
                        QDateTime::fromString(QString::fromUtf8(line.mid(13)), Qt::ISODate);
                }
            }
        }
        item.corrupt = !headerSeen || item.originalPath.isEmpty();
        seen.insert(item.name);
        items.append(item);
    }

    // files/ entries without a matching .trashinfo: unknown origin.
    const QStringList orphans = QDir(filesDir).entryList(
        QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const QString& name : orphans) {
        if (seen.contains(name))
            continue;
        TrashItem item;
        item.name = name;
        item.filesPath = filesDir + QLatin1Char('/') + name;
        item.corrupt = true;
        items.append(item);
    }
    return items;
}

bool TrashManager::restore(const TrashItem& item, bool overwrite, QString* error)
{
    if (item.originalPath.isEmpty()) {
        if (error)
            *error = QObject::tr("Unknown original location for %1").arg(item.name);
        return false;
    }
    const QFileInfo origin(item.originalPath);
    if ((origin.exists() || origin.isSymLink()) && !overwrite) {
        if (error)
            *error = QObject::tr("%1 already exists").arg(item.originalPath);
        return false;
    }
    if (origin.exists() && overwrite) {
        if (origin.isDir() && !origin.isSymLink())
            QDir(item.originalPath).removeRecursively();
        else
            QFile::remove(item.originalPath);
    }
    QDir().mkpath(origin.absolutePath());
    if (::rename(QFile::encodeName(item.filesPath).constData(),
                 QFile::encodeName(item.originalPath).constData()) != 0) {
        if (error)
            *error = QObject::tr("Could not restore %1: %2")
                         .arg(item.originalPath, QString::fromLocal8Bit(::strerror(errno)));
        return false;
    }
    if (!item.infoPath.isEmpty())
        QFile::remove(item.infoPath);
    return true;
}

bool TrashManager::removeItem(const TrashItem& item, QString* error)
{
    bool ok = true;
    const QFileInfo fi(item.filesPath);
    if (fi.isDir() && !fi.isSymLink())
        ok = QDir(item.filesPath).removeRecursively();
    else if (fi.exists() || fi.isSymLink())
        ok = QFile::remove(item.filesPath);
    if (!item.infoPath.isEmpty())
        QFile::remove(item.infoPath);
    if (!ok && error)
        *error = QObject::tr("Could not delete %1").arg(item.filesPath);
    return ok;
}

bool TrashManager::emptyAll(const QString& trashDir, QString* error)
{
    bool ok = true;
    const QList<TrashItem> items = list(trashDir);
    for (const TrashItem& item : items) {
        QString itemError;
        if (!removeItem(item, &itemError)) {
            ok = false;
            if (error && error->isEmpty())
                *error = itemError;
        }
    }
    return ok;
}

int TrashManager::itemCount(const QString& trashDir)
{
    const QString dir = trashDir.isEmpty() ? homeTrashDir() : trashDir;
    return QDir(dir + QLatin1String("/files"))
        .entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)
        .size();
}

} // namespace fang
