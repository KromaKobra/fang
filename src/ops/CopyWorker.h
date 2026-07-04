#pragma once

#include "ops/Operation.h"

#include <QElapsedTimer>
#include <QStringList>
#include <QVariantList>

namespace fang {

/// Recursive copy worker (docs/03 §6): enumerate pass for totals, streamed
/// 1 MiB chunk copies with pause/cancel between chunks, conflict handshake
/// per top-level entry (and per file when merging), metadata preservation,
/// precise created-path tracking for undo. Also implements move mode
/// (same-device rename fast path, else copy+verify+delete-source) so
/// MoveWorker is a thin subclass.
class CopyWorker : public OpWorker {
    Q_OBJECT

public:
    CopyWorker(const QStringList& sources, const QString& destDir,
               std::shared_ptr<OpControl> control, bool moveMode = false);

public slots:
    void run() override;

protected:
    /// True when the whole subtree copied without errors or skips.
    bool copyEntry(const QString& src, const QString& dest, bool mergeConflicts);
    bool copyFileStream(const QString& src, const QString& dest);
    /// Move one entry (merge = recurse into an existing dest dir). True when
    /// the source was fully moved away.
    bool moveEntry(const QString& src, const QString& dest, bool merge);

    void enumerate(const QString& path, qint64* bytes, int* files) const;
    void countSubtreeAsDone(const QString& path);
    void applyMetadata(const QString& src, const QString& dest);
    void removeExisting(const QString& path);
    void reportProgress(const QString& src, const QString& dest, bool force = false);
    static QString uniqueName(const QString& destPath);
    static bool sameDevice(const QString& a, const QString& b);

    const QStringList m_sources;
    const QString m_destDir;
    const bool m_moveMode;

    qint64 m_bytesDone = 0;
    qint64 m_bytesTotal = 0;
    int m_filesDone = 0;
    int m_filesTotal = 0;
    int m_successes = 0;
    QElapsedTimer m_progressClock;
    QStringList m_createdPaths; ///< creation order (undo deletes in reverse)
    QVariantList m_movePairs;   ///< maps {from, to} actually moved
};

} // namespace fang
