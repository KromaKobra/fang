#include "Previews.h"

#include "fs/FileInfo.h"
#include "util/HumanFormat.h"
#include "util/Logging.h"

#include <QDirIterator>
#include <QFile>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>

namespace fang {

namespace {

bool looksLikeText(const QByteArray& head)
{
    return !head.isEmpty() && !head.contains('\0');
}

} // namespace

Previews::Previews(QObject* parent)
    : QObject(parent)
{
}

QString Previews::readTextPeek(const QString& path, int maxKb) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    const QByteArray head = f.read(qMax(1, maxKb) * qint64(1024));
    if (!looksLikeText(head))
        return QString();
    return QString::fromUtf8(head);
}

bool Previews::isProbablyText(const QString& path) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    return looksLikeText(f.read(8192));
}

QVariantMap Previews::fileDetails(const QString& path) const
{
    const FileInfo fi = FileInfo::fromPath(path);
    QVariantMap map;
    map.insert(QStringLiteral("exists"), fi.exists);
    map.insert(QStringLiteral("name"), fi.name);
    map.insert(QStringLiteral("path"), fi.path);
    map.insert(QStringLiteral("size"), fi.size);
    map.insert(QStringLiteral("sizeText"), fi.size >= 0 ? HumanFormat::size(fi.size) : QString());
    map.insert(QStringLiteral("isDir"), fi.isDir);
    map.insert(QStringLiteral("isSymlink"), fi.isSymlink);
    map.insert(QStringLiteral("isImage"), fi.isImage());
    map.insert(QStringLiteral("mimeType"), fi.mimeType);
    map.insert(QStringLiteral("typeText"), fi.mimeComment);
    map.insert(QStringLiteral("modified"), fi.modified);
    map.insert(QStringLiteral("accessed"), fi.accessed);
    map.insert(QStringLiteral("created"), fi.created);
    map.insert(QStringLiteral("permissions"), fi.permissions);
    map.insert(QStringLiteral("owner"), fi.owner);
    map.insert(QStringLiteral("group"), fi.group);
    map.insert(QStringLiteral("iconName"), fi.iconName);
    return map;
}

void Previews::computeDirSize(const QString& path)
{
    QPointer<Previews> self(this);
    auto future = QtConcurrent::run([self, path] {
        qint64 bytes = 0;
        int files = 0;
        QDirIterator it(path, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo e = it.fileInfo();
            ++files;
            if (e.isFile() && !e.isSymLink())
                bytes += e.size();
        }
        if (self) {
            QMetaObject::invokeMethod(
                self,
                [self, path, bytes, files] {
                    if (self)
                        emit self->dirSizeComputed(path, bytes, files);
                },
                Qt::QueuedConnection);
        }
    });
    Q_UNUSED(future)
}

} // namespace fang
