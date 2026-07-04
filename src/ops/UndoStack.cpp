#include "UndoStack.h"

#include "app/Application.h"
#include "ops/FileOperationEngine.h"
#include "ops/Operation.h"
#include "util/Logging.h"

#include <QFileInfo>
#include <QHash>
#include <QUrl>

#include <algorithm>

namespace fang {

UndoStack::UndoStack(FileOperationEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

QString UndoStack::undoText() const
{
    return m_undoStack.isEmpty() ? QString() : tr("Undo %1").arg(m_undoStack.last().text);
}

QString UndoStack::redoText() const
{
    return m_redoStack.isEmpty() ? QString() : tr("Redo %1").arg(m_redoStack.last().text);
}

void UndoStack::push(const Record& record)
{
    m_undoStack.append(record);
    while (m_undoStack.size() > kLimit)
        m_undoStack.removeFirst();
    m_redoStack.clear();
    emit changed();
}

void UndoStack::undo()
{
    if (!canUndo())
        return;
    apply(m_undoStack.takeLast(), true);
}

void UndoStack::redo()
{
    if (!canRedo())
        return;
    apply(m_redoStack.takeLast(), false);
}

void UndoStack::apply(Record record, bool asUndo)
{
    m_busy = true;
    emit changed();

    auto shared = std::make_shared<Record>(record);
    std::function<void(Record*)> onSuccess;
    const QList<Operation*> ops = startOps(shared.get(), asUndo, &onSuccess);
    if (ops.isEmpty()) {
        m_busy = false;
        emit changed();
        if (Application::instance())
            Application::instance()->notify(asUndo ? tr("Undo failed") : tr("Redo failed"),
                                            record.text);
        return;
    }

    track(ops, [this, shared, asUndo, onSuccess](bool ok) {
        m_busy = false;
        if (ok) {
            if (onSuccess)
                onSuccess(shared.get());
            if (asUndo) {
                m_redoStack.append(*shared);
            } else {
                m_undoStack.append(*shared);
                while (m_undoStack.size() > kLimit)
                    m_undoStack.removeFirst();
            }
        } else if (Application::instance()) {
            // Dropped: the filesystem no longer matches what the record
            // describes, so replaying it later would be unsafe.
            Application::instance()->notify(asUndo ? tr("Undo failed") : tr("Redo failed"),
                                            shared->text);
        }
        emit changed();
    });
}

QList<Operation*> UndoStack::startOps(Record* record, bool asUndo,
                                      std::function<void(Record*)>* onSuccess)
{
    QList<Operation*> ops;
    const QVariantMap& data = record->data;

    const auto urlsFromPaths = [](const QStringList& paths) {
        QList<QUrl> urls;
        for (const QString& p : paths)
            urls << QUrl::fromLocalFile(p);
        return urls;
    };

    switch (record->type) {
    case OpType::Copy: {
        if (asUndo) {
            // Delete exactly what the copy created, deepest-first.
            QStringList created = data.value(QStringLiteral("createdPaths")).toStringList();
            std::reverse(created.begin(), created.end());
            if (!created.isEmpty())
                ops << m_engine->removeList(created);
        } else {
            Operation* op =
                m_engine->copy(urlsFromPaths(data.value(QStringLiteral("sources")).toStringList()),
                               QUrl::fromLocalFile(data.value(QStringLiteral("destDir")).toString()),
                               false);
            ops << op;
            *onSuccess = [op](Record* r) {
                r->data.insert(QStringLiteral("createdPaths"),
                               op->undoData().toMap().value(QStringLiteral("createdPaths")));
            };
        }
        break;
    }

    case OpType::Move: {
        // One engine op per group of pairs sharing a destination parent.
        QHash<QString, QList<QUrl>> groups;
        const QVariantList pairs = data.value(QStringLiteral("pairs")).toList();
        for (const QVariant& v : pairs) {
            const QVariantMap pair = v.toMap();
            const QString from = pair.value(QStringLiteral("from")).toString();
            const QString to = pair.value(QStringLiteral("to")).toString();
            if (asUndo)
                groups[QFileInfo(from).absolutePath()] << QUrl::fromLocalFile(to);
            else
                groups[QFileInfo(to).absolutePath()] << QUrl::fromLocalFile(from);
        }
        for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
            ops << m_engine->move(it.value(), QUrl::fromLocalFile(it.key()), false);
        break;
    }

    case OpType::Rename: {
        const QString dir = data.value(QStringLiteral("dir")).toString();
        const QString oldName = data.value(QStringLiteral("oldName")).toString();
        const QString newName = data.value(QStringLiteral("newName")).toString();
        const QString from = dir + QLatin1Char('/') + (asUndo ? newName : oldName);
        const QString to = asUndo ? oldName : newName;
        ops << m_engine->rename(QUrl::fromLocalFile(from), to, false);
        break;
    }

    case OpType::Trash: {
        if (asUndo) {
            ops << m_engine->restore(data.value(QStringLiteral("items")).toList(), false);
        } else {
            QStringList paths;
            const QVariantList items = data.value(QStringLiteral("items")).toList();
            for (const QVariant& v : items)
                paths << v.toMap().value(QStringLiteral("originalPath")).toString();
            Operation* op = m_engine->trash(urlsFromPaths(paths), false);
            ops << op;
            *onSuccess = [op](Record* r) {
                // Re-trashing lands in fresh slots; refresh them for undo.
                r->data.insert(QStringLiteral("items"),
                               op->undoData().toMap().value(QStringLiteral("items")));
            };
        }
        break;
    }

    case OpType::Restore: {
        if (asUndo) {
            Operation* op = m_engine->trash(
                urlsFromPaths(data.value(QStringLiteral("restoredPaths")).toStringList()), false);
            ops << op;
            *onSuccess = [op](Record* r) {
                r->data.insert(QStringLiteral("items"),
                               op->undoData().toMap().value(QStringLiteral("items")));
            };
        } else {
            Operation* op = m_engine->restore(data.value(QStringLiteral("items")).toList(), false);
            ops << op;
            *onSuccess = [op](Record* r) {
                r->data.insert(QStringLiteral("restoredPaths"),
                               op->undoData().toMap().value(QStringLiteral("restoredPaths")));
            };
        }
        break;
    }

    case OpType::CreateFolder: {
        const QString path = data.value(QStringLiteral("path")).toString();
        if (asUndo) {
            ops << m_engine->removeList({path});
        } else {
            const QFileInfo fi(path);
            const QUrl parent = QUrl::fromLocalFile(fi.absolutePath());
            const bool isFolder = data.value(QStringLiteral("isFolder")).toBool();
            ops << (isFolder ? m_engine->createFolder(parent, fi.fileName(), false)
                             : m_engine->createFile(parent, fi.fileName(), false));
        }
        break;
    }

    case OpType::Delete:
        break; // permanent deletes are not undoable, never recorded
    }
    return ops;
}

void UndoStack::track(const QList<Operation*>& ops, std::function<void(bool)> whenAllDone)
{
    struct Tracker {
        int pending = 0;
        bool anyFailed = false;
        std::function<void(bool)> finalize;
    };
    auto tracker = std::make_shared<Tracker>();
    tracker->pending = ops.size();
    tracker->finalize = std::move(whenAllDone);

    const auto settle = [tracker](bool ok) {
        if (!ok)
            tracker->anyFailed = true;
        if (--tracker->pending == 0)
            tracker->finalize(!tracker->anyFailed);
    };

    for (Operation* op : ops) {
        if (op->isTerminal()) {
            // Synchronous ops (rename/create) finish inside the engine call.
            settle(op->state() == OpState::Completed);
            continue;
        }
        connect(op, &Operation::finished, this,
                [op, settle] { settle(op->state() == OpState::Completed); });
        connect(op, &Operation::failed, this, [settle](const QString&) { settle(false); });
    }
}

} // namespace fang
