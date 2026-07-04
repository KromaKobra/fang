#include "FileOperationEngine.h"

#include "app/Application.h"
#include "ops/Clipboard.h"
#include "ops/CopyWorker.h"
#include "ops/DeleteWorker.h"
#include "ops/MoveWorker.h"
#include "ops/Operation.h"
#include "ops/UndoStack.h"
#include "util/Logging.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QThread>

#include <sys/stat.h>

namespace fang {

namespace {
constexpr int kMaxRetainedOps = 20;

QString opTitle(OpType type)
{
    switch (type) {
    case OpType::Copy: return QObject::tr("Copy failed");
    case OpType::Move: return QObject::tr("Move failed");
    case OpType::Delete: return QObject::tr("Delete failed");
    case OpType::Trash: return QObject::tr("Move to trash failed");
    case OpType::Restore: return QObject::tr("Restore failed");
    case OpType::Rename: return QObject::tr("Rename failed");
    case OpType::CreateFolder: return QObject::tr("Create failed");
    }
    return QObject::tr("Operation failed");
}
} // namespace

FileOperationEngine* FileOperationEngine::s_instance = nullptr;

FileOperationEngine::FileOperationEngine(QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    m_undo = new UndoStack(this, this);
    m_clipboard = new Clipboard(this, this);
}

FileOperationEngine::~FileOperationEngine()
{
    for (Operation* op : std::as_const(m_operations))
        op->cancel();
    for (QThread* thread : std::as_const(m_threads)) {
        thread->quit();
        thread->wait(3000);
    }
    s_instance = nullptr;
}

FileOperationEngine* FileOperationEngine::instance()
{
    return s_instance;
}

QStringList FileOperationEngine::localPaths(const QList<QUrl>& urls)
{
    QStringList paths;
    for (const QUrl& url : urls) {
        if (url.isLocalFile())
            paths << QDir::cleanPath(url.toLocalFile());
        else if (url.scheme().isEmpty() && !url.path().isEmpty())
            paths << QDir::cleanPath(url.path());
    }
    return paths;
}

// ------------------------------------------------------------- async ops

Operation* FileOperationEngine::startAsync(OpType type, const WorkerFactory& makeWorker,
                                           bool record)
{
    auto* op = new Operation(type, this);
    addOperation(op);

    OpWorker* worker = makeWorker(op->control());
    auto* thread = new QThread(this);
    m_threads.append(thread);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &OpWorker::run);
    connect(worker, &OpWorker::progressed, op, &Operation::workerProgress);
    connect(worker, &OpWorker::conflictAsked, op, &Operation::workerConflict);
    connect(worker, &OpWorker::stateHint, op, &Operation::workerState);
    connect(worker, &OpWorker::done, this,
            [this, op, record](bool ok, const QString& error, const QVariant& undoData) {
                op->setUndoData(undoData);
                op->workerFinished(ok, error);
                if (record && op->state() == OpState::Completed)
                    recordCompleted(op);
                if (op->state() == OpState::Completed && !error.isEmpty()
                    && Application::instance())
                    Application::instance()->notify(tr("Completed with errors"), error);
                pruneOperations();
            });
    connect(worker, &OpWorker::done, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread] {
        m_threads.removeAll(thread);
        thread->deleteLater();
    });

    thread->start();
    return op;
}

Operation* FileOperationEngine::copy(const QList<QUrl>& sources, const QUrl& destDir, bool record)
{
    const QStringList paths = localPaths(sources);
    const QString dest = destDir.toLocalFile();
    return startAsync(OpType::Copy,
                      [paths, dest](std::shared_ptr<OpControl> ctl) {
                          return new CopyWorker(paths, dest, std::move(ctl));
                      },
                      record);
}

Operation* FileOperationEngine::move(const QList<QUrl>& sources, const QUrl& destDir, bool record)
{
    const QStringList paths = localPaths(sources);
    const QString dest = destDir.toLocalFile();
    return startAsync(OpType::Move,
                      [paths, dest](std::shared_ptr<OpControl> ctl) {
                          return new MoveWorker(paths, dest, std::move(ctl));
                      },
                      record);
}

Operation* FileOperationEngine::remove(const QList<QUrl>& urls)
{
    const QStringList paths = localPaths(urls);
    return startAsync(OpType::Delete,
                      [paths](std::shared_ptr<OpControl> ctl) {
                          return new DeleteWorker(DeleteWorker::Mode::Remove, paths, {},
                                                  std::move(ctl));
                      },
                      false);
}

Operation* FileOperationEngine::removeList(const QStringList& paths)
{
    return startAsync(OpType::Delete,
                      [paths](std::shared_ptr<OpControl> ctl) {
                          return new DeleteWorker(DeleteWorker::Mode::Remove, paths, {},
                                                  std::move(ctl), /*preciseList=*/true);
                      },
                      false);
}

Operation* FileOperationEngine::trash(const QList<QUrl>& urls, bool record)
{
    const QStringList paths = localPaths(urls);
    return startAsync(OpType::Trash,
                      [paths](std::shared_ptr<OpControl> ctl) {
                          return new DeleteWorker(DeleteWorker::Mode::Trash, paths, {},
                                                  std::move(ctl));
                      },
                      record);
}

Operation* FileOperationEngine::restore(const QVariantList& trashItems, bool record)
{
    return startAsync(OpType::Restore,
                      [trashItems](std::shared_ptr<OpControl> ctl) {
                          return new DeleteWorker(DeleteWorker::Mode::Restore, {}, trashItems,
                                                  std::move(ctl));
                      },
                      record);
}

// -------------------------------------------------------------- sync ops

Operation* FileOperationEngine::finishSyncOp(OpType type, bool ok, const QString& error)
{
    auto* op = new Operation(type, this);
    addOperation(op);
    op->finishSync(ok, error);
    pruneOperations();
    return op;
}

Operation* FileOperationEngine::rename(const QUrl& url, const QString& newName, bool record)
{
    const QString path = QDir::cleanPath(url.toLocalFile());
    const QFileInfo fi(path);
    const QString oldName = fi.fileName();
    const QString dir = fi.absolutePath();
    const QString dest = dir + QLatin1Char('/') + newName;

    QString error;
    bool ok = false;
    if (newName.isEmpty() || newName.contains(QLatin1Char('/')))
        error = tr("Invalid name: %1").arg(newName);
    else if (!fi.exists() && !fi.isSymLink())
        error = tr("No such file: %1").arg(path);
    else if (newName == oldName)
        ok = true;
    else if (QFileInfo::exists(dest) || QFileInfo(dest).isSymLink())
        error = tr("%1 already exists").arg(newName);
    else if (!QFile::rename(path, dest))
        error = tr("Could not rename %1").arg(path);
    else
        ok = true;

    Operation* op = finishSyncOp(OpType::Rename, ok, error);
    if (ok && record && newName != oldName) {
        UndoStack::Record rec;
        rec.type = OpType::Rename;
        rec.text = tr("Rename \"%1\"").arg(oldName);
        rec.data.insert(QStringLiteral("dir"), dir);
        rec.data.insert(QStringLiteral("oldName"), oldName);
        rec.data.insert(QStringLiteral("newName"), newName);
        m_undo->push(rec);
    }
    return op;
}

Operation* FileOperationEngine::createFolder(const QUrl& destDir, const QString& name, bool record)
{
    const QString dir = QDir::cleanPath(destDir.toLocalFile());
    const QString path = dir + QLatin1Char('/') + name;

    QString error;
    bool ok = false;
    if (name.isEmpty() || name.contains(QLatin1Char('/')))
        error = tr("Invalid folder name: %1").arg(name);
    else if (QFileInfo::exists(path))
        error = tr("%1 already exists").arg(name);
    else if (!QDir(dir).mkdir(name))
        error = tr("Could not create folder %1").arg(path);
    else
        ok = true;

    Operation* op = finishSyncOp(OpType::CreateFolder, ok, error);
    if (ok && record) {
        UndoStack::Record rec;
        rec.type = OpType::CreateFolder;
        rec.text = tr("Create Folder \"%1\"").arg(name);
        rec.data.insert(QStringLiteral("path"), path);
        rec.data.insert(QStringLiteral("isFolder"), true);
        m_undo->push(rec);
    }
    return op;
}

Operation* FileOperationEngine::createFile(const QUrl& destDir, const QString& name, bool record)
{
    const QString dir = QDir::cleanPath(destDir.toLocalFile());
    const QString path = dir + QLatin1Char('/') + name;

    QString error;
    bool ok = false;
    if (name.isEmpty() || name.contains(QLatin1Char('/'))) {
        error = tr("Invalid file name: %1").arg(name);
    } else {
        QFile f(path);
        if (f.exists())
            error = tr("%1 already exists").arg(name);
        else if (!f.open(QIODevice::WriteOnly | QIODevice::NewOnly))
            error = tr("Could not create file %1").arg(path);
        else
            ok = true;
    }

    Operation* op = finishSyncOp(OpType::CreateFolder, ok, error);
    if (ok && record) {
        UndoStack::Record rec;
        rec.type = OpType::CreateFolder;
        rec.text = tr("Create File \"%1\"").arg(name);
        rec.data.insert(QStringLiteral("path"), path);
        rec.data.insert(QStringLiteral("isFolder"), false);
        m_undo->push(rec);
    }
    return op;
}

// -------------------------------------------------------------- plumbing

void FileOperationEngine::addOperation(Operation* op)
{
    m_operations.append(op);
    connect(op, &Operation::conflict, this,
            [this, op](const QString& src, const QString& dest, const QVariantMap& info) {
                emit conflictRequested(op, src, dest, info);
            });
    connect(op, &Operation::failed, this, [op](const QString& text) {
        if (Application::instance())
            Application::instance()->notify(opTitle(op->type()), text);
    });
    emit operationsChanged();
}

void FileOperationEngine::pruneOperations()
{
    QList<Operation*> pruned;
    int terminalCount = 0;
    // Keep every live op and the most recent terminal ones.
    for (auto it = m_operations.crbegin(); it != m_operations.crend(); ++it) {
        Operation* op = *it;
        if (op->isTerminal() && ++terminalCount > kMaxRetainedOps) {
            op->deleteLater();
            continue;
        }
        pruned.prepend(op);
    }
    if (pruned.size() == m_operations.size())
        return;
    m_operations = pruned;
    emit operationsChanged();
}

void FileOperationEngine::recordCompleted(Operation* op)
{
    const QVariantMap data = op->undoData().toMap();
    UndoStack::Record rec;
    rec.type = op->type();
    rec.data = data;

    switch (op->type()) {
    case OpType::Copy: {
        const int n = data.value(QStringLiteral("sources")).toStringList().size();
        if (data.value(QStringLiteral("createdPaths")).toStringList().isEmpty())
            return; // nothing created (all skipped) — nothing to undo
        rec.text = tr("Copy (%n item(s))", nullptr, n);
        break;
    }
    case OpType::Move: {
        const QVariantList pairs = data.value(QStringLiteral("pairs")).toList();
        if (pairs.isEmpty())
            return;
        rec.text = tr("Move (%n item(s))", nullptr, int(pairs.size()));
        break;
    }
    case OpType::Trash: {
        const QVariantList items = data.value(QStringLiteral("items")).toList();
        if (items.isEmpty())
            return;
        rec.text = tr("Move to Trash (%n item(s))", nullptr, int(items.size()));
        break;
    }
    case OpType::Restore: {
        const QStringList paths = data.value(QStringLiteral("restoredPaths")).toStringList();
        if (paths.isEmpty())
            return;
        rec.text = tr("Restore (%n item(s))", nullptr, int(paths.size()));
        break;
    }
    default:
        return; // Delete never records; Rename/Create record synchronously
    }
    m_undo->push(rec);
}

// --------------------------------------------------------- FileOperations

FileOperations::FileOperations(QObject* parent)
    : QObject(parent)
{
    auto* engine = FileOperationEngine::instance();
    Q_ASSERT(engine);
    connect(engine, &FileOperationEngine::operationsChanged, this,
            &FileOperations::operationsChanged);
    connect(engine, &FileOperationEngine::conflictRequested, this,
            &FileOperations::conflictRequested);
}

QList<Operation*> FileOperations::operations() const
{
    return FileOperationEngine::instance()->operations();
}

Operation* FileOperations::copy(const QList<QUrl>& urls, const QUrl& destDir)
{
    return FileOperationEngine::instance()->copy(urls, destDir);
}

Operation* FileOperations::move(const QList<QUrl>& urls, const QUrl& destDir)
{
    return FileOperationEngine::instance()->move(urls, destDir);
}

Operation* FileOperations::remove(const QList<QUrl>& urls)
{
    return FileOperationEngine::instance()->remove(urls);
}

Operation* FileOperations::trash(const QList<QUrl>& urls)
{
    return FileOperationEngine::instance()->trash(urls);
}

Operation* FileOperations::restore(const QVariantList& trashItems)
{
    return FileOperationEngine::instance()->restore(trashItems);
}

Operation* FileOperations::rename(const QUrl& url, const QString& newName)
{
    return FileOperationEngine::instance()->rename(url, newName);
}

Operation* FileOperations::createFolder(const QUrl& destDir, const QString& name)
{
    return FileOperationEngine::instance()->createFolder(destDir, name);
}

Operation* FileOperations::autoTransfer(const QList<QUrl>& urls, const QUrl& destDir)
{
    if (urls.isEmpty())
        return nullptr;
    struct stat src{};
    struct stat dst{};
    const QByteArray srcPath = QFile::encodeName(urls.first().toLocalFile());
    const QByteArray dstPath = QFile::encodeName(destDir.toLocalFile());
    const bool sameDev = ::lstat(srcPath.constData(), &src) == 0
        && ::stat(dstPath.constData(), &dst) == 0 && src.st_dev == dst.st_dev;
    return sameDev ? move(urls, destDir) : copy(urls, destDir);
}

} // namespace fang
