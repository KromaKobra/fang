#pragma once

#include "util/Enums.h"

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QVariantMap>
#include <QWaitCondition>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <memory>

namespace fang {

/// Control block shared between an Operation (GUI thread) and its worker
/// (operation thread): pause/cancel flags and the blocking conflict
/// handshake (docs/03 §6).
struct OpControl {
    std::atomic<bool> cancelled{false};
    std::atomic<bool> paused{false};

    QMutex mutex;
    QWaitCondition cond;

    // Conflict handshake, guarded by mutex. The worker blocks on cond until
    // resolveConflict() (or cancel()) wakes it.
    bool conflictResolved = false;
    ConflictMode conflictMode = ConflictMode::Skip;
    bool hasCachedMode = false; // "apply to all" remembers the choice
    ConflictMode cachedMode = ConflictMode::Skip;
};

/// One enqueued file operation, exposed to QML with progress, pause/cancel
/// and the conflict-resolution entry point (docs/03 §6, docs/04).
class Operation : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Operations are created by the FileOperationEngine")

    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(fang::OpType type READ type CONSTANT)
    Q_PROPERTY(fang::OpState state READ state NOTIFY stateChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(qint64 bytesDone READ bytesDone NOTIFY progressChanged)
    Q_PROPERTY(qint64 bytesTotal READ bytesTotal NOTIFY progressChanged)
    Q_PROPERTY(int filesDone READ filesDone NOTIFY progressChanged)
    Q_PROPERTY(int filesTotal READ filesTotal NOTIFY progressChanged)
    Q_PROPERTY(QString currentSource READ currentSource NOTIFY progressChanged)
    Q_PROPERTY(QString currentDest READ currentDest NOTIFY progressChanged)
    Q_PROPERTY(qint64 speedBytesPerSec READ speedBytesPerSec NOTIFY progressChanged)
    Q_PROPERTY(qint64 etaSeconds READ etaSeconds NOTIFY progressChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    Operation(OpType type, QObject* parent);

    int id() const { return m_id; }
    OpType type() const { return m_type; }
    OpState state() const { return m_state; }
    qreal progress() const;
    qint64 bytesDone() const { return m_bytesDone; }
    qint64 bytesTotal() const { return m_bytesTotal; }
    int filesDone() const { return m_filesDone; }
    int filesTotal() const { return m_filesTotal; }
    QString currentSource() const { return m_currentSource; }
    QString currentDest() const { return m_currentDest; }
    qint64 speedBytesPerSec() const { return m_speed; }
    qint64 etaSeconds() const { return m_eta; }
    QString errorText() const { return m_errorText; }

    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void resolveConflict(fang::ConflictMode mode, bool applyToAll);

    std::shared_ptr<OpControl> control() const { return m_control; }
    bool isTerminal() const;

    /// Per-type undo payload delivered by the worker's done() (engine-set;
    /// consumed by UndoStack when replaying records).
    QVariant undoData() const { return m_undoData; }
    void setUndoData(const QVariant& data) { m_undoData = data; }

    /// Marks a synchronous operation done (rename/create paths).
    void finishSync(bool ok, const QString& error);

public slots:
    // Delivered from the worker thread via queued connections.
    void workerProgress(qint64 bytesDone, qint64 bytesTotal, int filesDone, int filesTotal,
                        const QString& currentSource, const QString& currentDest);
    void workerConflict(const QString& sourceUrl, const QString& destUrl,
                        const QVariantMap& info);
    void workerState(int state);
    void workerFinished(bool ok, const QString& error);

signals:
    void stateChanged();
    void progressChanged();
    void errorTextChanged();
    void conflict(const QString& sourceUrl, const QString& destUrl, const QVariantMap& info);
    void finished();
    void failed(const QString& text);

private:
    void setState(OpState state);

    static std::atomic<int> s_nextId;
    const int m_id;
    const OpType m_type;
    OpState m_state = OpState::Queued;
    qint64 m_bytesDone = 0;
    qint64 m_bytesTotal = 0;
    int m_filesDone = 0;
    int m_filesTotal = 0;
    QString m_currentSource;
    QString m_currentDest;
    qint64 m_speed = 0;
    qint64 m_eta = -1;
    QString m_errorText;
    QVariant m_undoData;
    QElapsedTimer m_clock;
    std::shared_ptr<OpControl> m_control;
};

/// Base for the thread-side workers: pause/cancel checks and the blocking
/// conflict request. Lives on its operation's QThread.
class OpWorker : public QObject {
    Q_OBJECT

public:
    explicit OpWorker(std::shared_ptr<OpControl> control, QObject* parent = nullptr);

public slots:
    virtual void run() = 0;

signals:
    void progressed(qint64 bytesDone, qint64 bytesTotal, int filesDone, int filesTotal,
                    const QString& currentSource, const QString& currentDest);
    void conflictAsked(const QString& sourceUrl, const QString& destUrl,
                       const QVariantMap& info);
    void stateHint(int state);
    /// undoData: per-type payload the engine turns into an UndoStack record.
    void done(bool ok, const QString& error, const QVariant& undoData);

protected:
    /// Returns false when cancelled; blocks while paused.
    bool keepGoing();
    /// Blocks until the GUI resolves the conflict (or cancel). Emits the
    /// conflict with both files' details.
    ConflictMode askConflict(const QString& srcPath, const QString& destPath);
    void addError(const QString& text) { m_errors << text; }
    QString errorSummary() const;

    std::shared_ptr<OpControl> m_ctl;
    QStringList m_errors;
};

} // namespace fang
