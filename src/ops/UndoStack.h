#pragma once

#include "util/Enums.h"

#include <QList>
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <memory>

namespace fang {

class FileOperationEngine;
class Operation;

/// Bounded multi-level undo/redo (docs/03 §8). Each completed mutating
/// engine operation pushes a record; undo/redo run the inverse as a new
/// engine operation with recording suppressed and move the record between
/// the two stacks on success.
class UndoStack : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("The UndoStack is owned by the application")

    Q_PROPERTY(bool canUndo READ canUndo NOTIFY changed)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY changed)
    Q_PROPERTY(QString undoText READ undoText NOTIFY changed)
    Q_PROPERTY(QString redoText READ redoText NOTIFY changed)

public:
    struct Record {
        OpType type = OpType::Copy;
        QString text; ///< e.g. "Move (3 items)"
        QVariantMap data;
    };

    explicit UndoStack(FileOperationEngine* engine, QObject* parent = nullptr);

    bool canUndo() const { return !m_busy && !m_undoStack.isEmpty(); }
    bool canRedo() const { return !m_busy && !m_redoStack.isEmpty(); }
    QString undoText() const;
    QString redoText() const;

    /// Called by the engine when a recordable operation completes.
    void push(const Record& record);

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

signals:
    void changed();

private:
    void apply(Record record, bool asUndo);
    /// Starts the inverse (asUndo) or forward (redo) operations for a record.
    /// May update `record` data from the finished ops' undo payloads before
    /// it lands on the target stack.
    QList<Operation*> startOps(Record* record, bool asUndo,
                               std::function<void(Record*)>* onSuccess);
    void track(const QList<Operation*>& ops, std::function<void(bool)> whenAllDone);

    static constexpr int kLimit = 50;

    FileOperationEngine* m_engine;
    QList<Record> m_undoStack;
    QList<Record> m_redoStack;
    bool m_busy = false; ///< an inverse operation is in flight
};

} // namespace fang
