#include "Operation.h"

#include <QFileInfo>
#include <QMutexLocker>
#include <QUrl>

namespace fang {

std::atomic<int> Operation::s_nextId{1};

Operation::Operation(OpType type, QObject* parent)
    : QObject(parent)
    , m_id(s_nextId.fetch_add(1))
    , m_type(type)
    , m_control(std::make_shared<OpControl>())
{
    m_clock.start();
}

qreal Operation::progress() const
{
    if (m_bytesTotal > 0)
        return qBound<qreal>(0.0, qreal(m_bytesDone) / qreal(m_bytesTotal), 1.0);
    if (m_filesTotal > 0)
        return qBound<qreal>(0.0, qreal(m_filesDone) / qreal(m_filesTotal), 1.0);
    return isTerminal() ? 1.0 : 0.0;
}

bool Operation::isTerminal() const
{
    return m_state == OpState::Completed || m_state == OpState::Failed
        || m_state == OpState::Cancelled;
}

void Operation::pause()
{
    if (isTerminal())
        return;
    m_control->paused.store(true);
    if (m_state == OpState::Running)
        setState(OpState::Paused);
}

void Operation::resume()
{
    if (isTerminal())
        return;
    m_control->paused.store(false);
    m_control->cond.wakeAll();
    if (m_state == OpState::Paused)
        setState(OpState::Running);
}

void Operation::cancel()
{
    if (isTerminal())
        return;
    m_control->cancelled.store(true);
    m_control->paused.store(false);
    {
        // A worker blocked in askConflict() must wake and observe the cancel.
        QMutexLocker lock(&m_control->mutex);
        m_control->conflictResolved = true;
        m_control->conflictMode = ConflictMode::Skip;
    }
    m_control->cond.wakeAll();
}

void Operation::resolveConflict(ConflictMode mode, bool applyToAll)
{
    {
        QMutexLocker lock(&m_control->mutex);
        m_control->conflictResolved = true;
        m_control->conflictMode = mode;
        if (applyToAll) {
            m_control->hasCachedMode = true;
            m_control->cachedMode = mode;
        }
    }
    m_control->cond.wakeAll();
    if (m_state == OpState::WaitingConflict)
        setState(m_control->paused.load() ? OpState::Paused : OpState::Running);
}

void Operation::finishSync(bool ok, const QString& error)
{
    m_filesTotal = 1;
    m_filesDone = ok ? 1 : 0;
    if (!error.isEmpty()) {
        m_errorText = error;
        emit errorTextChanged();
    }
    emit progressChanged();
    setState(ok ? OpState::Completed : OpState::Failed);
    if (ok)
        emit finished();
    else
        emit failed(m_errorText);
}

void Operation::workerProgress(qint64 bytesDone, qint64 bytesTotal, int filesDone, int filesTotal,
                               const QString& currentSource, const QString& currentDest)
{
    m_bytesDone = bytesDone;
    m_bytesTotal = bytesTotal;
    m_filesDone = filesDone;
    m_filesTotal = filesTotal;
    m_currentSource = currentSource;
    m_currentDest = currentDest;

    const qint64 elapsedMs = qMax<qint64>(1, m_clock.elapsed());
    m_speed = bytesDone * 1000 / elapsedMs;
    m_eta = (m_speed > 0 && bytesTotal > bytesDone) ? (bytesTotal - bytesDone) / m_speed : -1;
    emit progressChanged();
}

void Operation::workerConflict(const QString& sourceUrl, const QString& destUrl,
                               const QVariantMap& info)
{
    setState(OpState::WaitingConflict);
    emit conflict(sourceUrl, destUrl, info);
}

void Operation::workerState(int state)
{
    setState(static_cast<OpState>(state));
}

void Operation::workerFinished(bool ok, const QString& error)
{
    if (!error.isEmpty()) {
        m_errorText = error;
        emit errorTextChanged();
    }
    emit progressChanged(); // final speed/eta refresh

    if (m_control->cancelled.load())
        setState(OpState::Cancelled);
    else
        setState(ok ? OpState::Completed : OpState::Failed);

    if (m_state == OpState::Failed)
        emit failed(m_errorText);
    else
        emit finished();
}

void Operation::setState(OpState state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged();
}

// ---------------------------------------------------------------- OpWorker

OpWorker::OpWorker(std::shared_ptr<OpControl> control, QObject* parent)
    : QObject(parent)
    , m_ctl(std::move(control))
{
}

bool OpWorker::keepGoing()
{
    if (m_ctl->cancelled.load())
        return false;
    if (m_ctl->paused.load()) {
        QMutexLocker lock(&m_ctl->mutex);
        while (m_ctl->paused.load() && !m_ctl->cancelled.load())
            m_ctl->cond.wait(&m_ctl->mutex, 200);
    }
    return !m_ctl->cancelled.load();
}

ConflictMode OpWorker::askConflict(const QString& srcPath, const QString& destPath)
{
    {
        QMutexLocker lock(&m_ctl->mutex);
        if (m_ctl->hasCachedMode)
            return m_ctl->cachedMode;
        m_ctl->conflictResolved = false;
    }

    const QFileInfo src(srcPath);
    const QFileInfo dest(destPath);
    QVariantMap info;
    info.insert(QStringLiteral("sourceName"), src.fileName());
    info.insert(QStringLiteral("sourceSize"), src.size());
    info.insert(QStringLiteral("sourceModified"), src.lastModified());
    info.insert(QStringLiteral("sourceIsDir"), src.isDir());
    info.insert(QStringLiteral("destName"), dest.fileName());
    info.insert(QStringLiteral("destSize"), dest.size());
    info.insert(QStringLiteral("destModified"), dest.lastModified());
    info.insert(QStringLiteral("destIsDir"), dest.isDir());
    emit conflictAsked(QUrl::fromLocalFile(srcPath).toString(),
                       QUrl::fromLocalFile(destPath).toString(), info);

    QMutexLocker lock(&m_ctl->mutex);
    while (!m_ctl->conflictResolved && !m_ctl->cancelled.load())
        m_ctl->cond.wait(&m_ctl->mutex, 200);
    if (m_ctl->cancelled.load())
        return ConflictMode::Skip;
    return m_ctl->conflictMode;
}

QString OpWorker::errorSummary() const
{
    if (m_errors.isEmpty())
        return QString();
    if (m_errors.size() == 1)
        return m_errors.first();
    return tr("%1 (and %2 more errors)").arg(m_errors.first()).arg(m_errors.size() - 1);
}

} // namespace fang
