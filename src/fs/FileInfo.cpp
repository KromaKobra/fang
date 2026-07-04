#include "FileInfo.h"

#include <QFileDevice>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QUrl>

namespace fang {

namespace {

QString permissionString(const QFileInfo& fi)
{
    const QFileDevice::Permissions p = fi.permissions();
    QString s;
    s.reserve(9);
    s += (p & QFileDevice::ReadOwner) ? QLatin1Char('r') : QLatin1Char('-');
    s += (p & QFileDevice::WriteOwner) ? QLatin1Char('w') : QLatin1Char('-');
    s += (p & QFileDevice::ExeOwner) ? QLatin1Char('x') : QLatin1Char('-');
    s += (p & QFileDevice::ReadGroup) ? QLatin1Char('r') : QLatin1Char('-');
    s += (p & QFileDevice::WriteGroup) ? QLatin1Char('w') : QLatin1Char('-');
    s += (p & QFileDevice::ExeGroup) ? QLatin1Char('x') : QLatin1Char('-');
    s += (p & QFileDevice::ReadOther) ? QLatin1Char('r') : QLatin1Char('-');
    s += (p & QFileDevice::WriteOther) ? QLatin1Char('w') : QLatin1Char('-');
    s += (p & QFileDevice::ExeOther) ? QLatin1Char('x') : QLatin1Char('-');
    return s;
}

} // namespace

FileInfo FileInfo::fromPath(const QString& path, bool withMime)
{
    FileInfo out;
    const QFileInfo fi(path);
    out.path = fi.absoluteFilePath();
    out.name = fi.fileName();
    if (out.name.isEmpty())
        out.name = out.path; // e.g. "/"
    out.exists = fi.exists() || fi.isSymLink();
    out.isSymlink = fi.isSymLink();
    out.isDir = fi.isDir();
    out.isHidden = out.name.startsWith(QLatin1Char('.'));
    out.extension = fi.suffix().toLower();
    out.size = out.isDir ? -1 : fi.size();
    out.modified = fi.lastModified();
    out.created = fi.birthTime().isValid() ? fi.birthTime() : fi.metadataChangeTime();
    out.accessed = fi.lastRead();
    out.permissions = permissionString(fi);
    out.owner = fi.owner();
    out.group = fi.group();

    if (withMime) {
        static thread_local QMimeDatabase mimeDb;
        if (out.isDir) {
            out.mimeType = QStringLiteral("inode/directory");
            out.mimeComment = QStringLiteral("Folder");
            out.iconName = QStringLiteral("folder");
        } else {
            const QMimeType mime = mimeDb.mimeTypeForFile(fi, QMimeDatabase::MatchExtension);
            out.mimeType = mime.name();
            out.mimeComment = mime.comment();
            out.iconName = mime.iconName().isEmpty() ? mime.genericIconName() : mime.iconName();
            if (out.iconName.isEmpty())
                out.iconName = QStringLiteral("text-x-generic");
        }
    }
    return out;
}

QString FileInfo::url() const
{
    return QUrl::fromLocalFile(path).toString();
}

bool FileInfo::isImage() const
{
    return mimeType.startsWith(QLatin1String("image/"));
}

} // namespace fang
