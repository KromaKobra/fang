#include "ThumbnailGenerator.h"

#include "util/Logging.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

#include <atomic>

namespace fang {

namespace {

struct Generator {
    QString pattern; // "image/*" or exact mime
    ThumbnailGenerator::GenerateFn fn;
};

QMutex g_registryMutex;
QList<Generator>* g_registry = nullptr;
std::atomic<qint64> g_maxFileSize{qint64(64) * 1024 * 1024};

QImage generateImageThumb(const QString& absPath, int px)
{
    QImageReader reader(absPath);
    reader.setAutoTransform(true); // EXIF orientation
    QSize size = reader.size();
    if (size.isValid()) {
        // Transposed EXIF orientations swap width/height after transform.
        const auto t = reader.transformation();
        if (t & QImageIOHandler::TransformationRotate90)
            size.transpose();
        if (size.width() > px || size.height() > px)
            reader.setScaledSize(size.scaled(px, px, Qt::KeepAspectRatio));
    }
    QImage image = reader.read();
    if (image.isNull())
        return image;
    if (image.width() > px || image.height() > px)
        image = image.scaled(px, px, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return image;
}

bool mimeMatches(const QString& pattern, const QString& mime)
{
    if (pattern.endsWith(QLatin1String("/*")))
        return mime.startsWith(QStringView(pattern).left(pattern.size() - 1));
    return pattern == mime;
}

QList<Generator>& registry()
{
    if (!g_registry) {
        g_registry = new QList<Generator>;
        // v1 built-in: raster images + SVG, both via QImageReader.
        g_registry->append({QStringLiteral("image/*"), generateImageThumb});
    }
    return *g_registry;
}

QString fileUri(const QString& absPath)
{
    return QUrl::fromLocalFile(absPath).toString(QUrl::FullyEncoded);
}

} // namespace

ThumbnailGenerator::SizeClass ThumbnailGenerator::classFor(const QSize& requested)
{
    const int wanted = qMax(requested.width(), requested.height());
    return (wanted <= 0 || wanted > pixels(SizeClass::Normal)) ? SizeClass::Large
                                                               : SizeClass::Normal;
}

QString ThumbnailGenerator::cacheDir(SizeClass size)
{
    // QStandardPaths reads $XDG_CACHE_HOME at call time (test redirection).
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
        + QLatin1String("/thumbnails/")
        + (size == SizeClass::Normal ? QLatin1String("normal") : QLatin1String("large"));
}

QString ThumbnailGenerator::cachePathFor(const QString& absPath, SizeClass size)
{
    const QByteArray md5 =
        QCryptographicHash::hash(fileUri(absPath).toUtf8(), QCryptographicHash::Md5).toHex();
    return cacheDir(size) + QLatin1Char('/') + QString::fromLatin1(md5) + QLatin1String(".png");
}

QImage ThumbnailGenerator::loadCached(const QString& absPath, SizeClass size)
{
    const QString cachePath = cachePathFor(absPath, size);
    QImage cached(cachePath);
    if (cached.isNull())
        return QImage();
    const QFileInfo fi(absPath);
    if (cached.text(QStringLiteral("Thumb::URI")) != fileUri(absPath))
        return QImage();
    const qint64 mtime = fi.lastModified().toSecsSinceEpoch();
    if (cached.text(QStringLiteral("Thumb::MTime")).toLongLong() != mtime)
        return QImage();
    return cached;
}

bool ThumbnailGenerator::writeCache(const QString& absPath, SizeClass size, QImage image)
{
    if (image.isNull())
        return false;
    const QFileInfo fi(absPath);
    image.setText(QStringLiteral("Thumb::URI"), fileUri(absPath));
    image.setText(QStringLiteral("Thumb::MTime"),
                  QString::number(fi.lastModified().toSecsSinceEpoch()));
    image.setText(QStringLiteral("Thumb::Size"), QString::number(fi.size()));

    QDir().mkpath(cacheDir(size));
    QFile::setPermissions(cacheDir(size),
                          QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    QSaveFile out(cachePathFor(absPath, size));
    if (!out.open(QIODevice::WriteOnly))
        return false;
    if (!image.save(&out, "PNG") || !out.commit())
        return false;
    QFile::setPermissions(out.fileName(), QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

QImage ThumbnailGenerator::generate(const QString& absPath, SizeClass size)
{
    const QFileInfo fi(absPath);
    if (!fi.isFile() || fi.size() > g_maxFileSize.load())
        return QImage();

    const QString mime =
        QMimeDatabase().mimeTypeForFile(absPath, QMimeDatabase::MatchDefault).name();
    GenerateFn fn;
    {
        QMutexLocker lock(&g_registryMutex);
        const QList<Generator>& gens = registry();
        for (auto it = gens.crbegin(); it != gens.crend(); ++it) {
            if (mimeMatches(it->pattern, mime)) {
                fn = it->fn;
                break;
            }
        }
    }
    if (!fn)
        return QImage();
    return fn(absPath, pixels(size));
}

QImage ThumbnailGenerator::getOrCreate(const QString& absPath, SizeClass size)
{
    QImage cached = loadCached(absPath, size);
    if (!cached.isNull())
        return cached;
    QImage fresh = generate(absPath, size);
    if (!fresh.isNull())
        writeCache(absPath, size, fresh);
    return fresh;
}

void ThumbnailGenerator::registerGenerator(const QString& mimePattern, GenerateFn fn)
{
    QMutexLocker lock(&g_registryMutex);
    registry().append({mimePattern, std::move(fn)});
}

void ThumbnailGenerator::setMaxFileSizeBytes(qint64 bytes)
{
    g_maxFileSize.store(bytes);
}

qint64 ThumbnailGenerator::maxFileSizeBytes()
{
    return g_maxFileSize.load();
}

} // namespace fang
