#include "ThumbnailProvider.h"

#include "preview/ThumbnailGenerator.h"
#include "util/Logging.h"

#include <QRunnable>

namespace fang {

namespace {

class ThumbnailResponse : public QQuickImageResponse, public QRunnable {
public:
    ThumbnailResponse(const QString& path, const QSize& requestedSize)
        : m_path(path)
        , m_requestedSize(requestedSize)
    {
        setAutoDelete(false); // QQuickImageResponse manages its own lifetime
    }

    void run() override
    {
        m_image = ThumbnailGenerator::getOrCreate(
            m_path, ThumbnailGenerator::classFor(m_requestedSize));
        emit finished();
    }

    QQuickTextureFactory* textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    QString errorString() const override
    {
        return m_image.isNull() ? QStringLiteral("no thumbnail for %1").arg(m_path)
                                : QString();
    }

private:
    const QString m_path;
    const QSize m_requestedSize;
    QImage m_image;
};

} // namespace

ThumbnailProvider::ThumbnailProvider()
{
    m_pool.setMaxThreadCount(3);
}

QQuickImageResponse* ThumbnailProvider::requestImageResponse(const QString& id,
                                                             const QSize& requestedSize)
{
    // The engine strips the leading slash from the URL path; restore it.
    const QString path = id.startsWith(QLatin1Char('/')) ? id : QLatin1Char('/') + id;
    auto* response = new ThumbnailResponse(path, requestedSize);
    m_pool.start(response);
    return response;
}

} // namespace fang
