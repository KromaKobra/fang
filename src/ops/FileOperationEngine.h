#pragma once

#include "util/Enums.h"

#include <QList>
#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <memory>

Q_MOC_INCLUDE("ops/Operation.h")

namespace fang {

struct OpControl;
class Clipboard;
class Operation;
class OpWorker;
class UndoStack;

/// Central owner of all file operations (docs/03 §6): spins one QThread per
/// async op, wires worker→Operation queued signals, records undo on success
/// and surfaces failures as app notifications. App-wide C++ singleton
/// (instance()), NOT a QML singleton — QML reaches it via Application
/// properties and per-pane FileOperations controllers.
class FileOperationEngine : public QObject {
    Q_OBJECT

public:
    explicit FileOperationEngine(QObject* parent = nullptr);
    ~FileOperationEngine() override;

    static FileOperationEngine* instance();

    UndoStack* undoStack() const { return m_undo; }
    Clipboard* clipboard() const { return m_clipboard; }
    QList<Operation*> operations() const { return m_operations; }

    Operation* copy(const QList<QUrl>& sources, const QUrl& destDir, bool record = true);
    Operation* move(const QList<QUrl>& sources, const QUrl& destDir, bool record = true);
    /// Permanent recursive delete; never undoable.
    Operation* remove(const QList<QUrl>& urls);
    /// Precise non-recursive delete of exact paths in the given order
    /// (undo of copy/create); never undoable itself.
    Operation* removeList(const QStringList& paths);
    Operation* trash(const QList<QUrl>& urls, bool record = true);
    Operation* restore(const QVariantList& trashItems, bool record = true);

    // Synchronous ops: the returned Operation is already terminal.
    Operation* rename(const QUrl& url, const QString& newName, bool record = true);
    Operation* createFolder(const QUrl& destDir, const QString& name, bool record = true);
    Operation* createFile(const QUrl& destDir, const QString& name, bool record = true);

signals:
    void operationsChanged();
    void conflictRequested(fang::Operation* op, const QString& sourceUrl, const QString& destUrl,
                           const QVariantMap& info);

private:
    using WorkerFactory = std::function<OpWorker*(std::shared_ptr<OpControl>)>;

    Operation* startAsync(OpType type, const WorkerFactory& makeWorker, bool record);
    Operation* finishSyncOp(OpType type, bool ok, const QString& error);
    void addOperation(Operation* op);
    void pruneOperations();
    void recordCompleted(Operation* op);
    static QStringList localPaths(const QList<QUrl>& urls);

    static FileOperationEngine* s_instance;

    QList<Operation*> m_operations;
    QList<class QThread*> m_threads;
    UndoStack* m_undo = nullptr;
    Clipboard* m_clipboard = nullptr;
};

/// Thin per-pane QML controller forwarding to the engine (docs/04
/// `FileOperations`). Owned by its PaneController.
class FileOperations : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PaneController")

    Q_PROPERTY(QList<fang::Operation*> operations READ operations NOTIFY operationsChanged)

public:
    explicit FileOperations(QObject* parent = nullptr);

    QList<Operation*> operations() const;

    Q_INVOKABLE fang::Operation* copy(const QList<QUrl>& urls, const QUrl& destDir);
    Q_INVOKABLE fang::Operation* move(const QList<QUrl>& urls, const QUrl& destDir);
    Q_INVOKABLE fang::Operation* remove(const QList<QUrl>& urls);
    Q_INVOKABLE fang::Operation* trash(const QList<QUrl>& urls);
    Q_INVOKABLE fang::Operation* restore(const QVariantList& trashItems);
    Q_INVOKABLE fang::Operation* rename(const QUrl& url, const QString& newName);
    Q_INVOKABLE fang::Operation* createFolder(const QUrl& destDir, const QString& name);
    /// DnD "auto" drop: move when source and destination share a device,
    /// copy across devices. Additive to API v1.
    Q_INVOKABLE fang::Operation* autoTransfer(const QList<QUrl>& urls, const QUrl& destDir);

signals:
    void operationsChanged();
    void conflictRequested(fang::Operation* op, const QString& sourceUrl, const QString& destUrl,
                           const QVariantMap& info);
};

} // namespace fang
