#pragma once

#include <QQuickAsyncImageProvider>
#include <QThreadPool>

namespace fang {

/// Async image provider "fang-thumb" (docs/03 §10): QML asks for
/// image://fang-thumb/<abs-path>; generation runs on a small private pool
/// and lands in the shared freedesktop cache. Never blocks the GUI thread.
class ThumbnailProvider : public QQuickAsyncImageProvider {
public:
    ThumbnailProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize& requestedSize) override;

private:
    QThreadPool m_pool;
};

} // namespace fang
