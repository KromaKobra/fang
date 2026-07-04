#pragma once

#include "ops/Operation.h"

#include <QElapsedTimer>
#include <QStringList>
#include <QVariantList>

namespace fang {

/// The removal family (docs/03 §6-7, PROGRESS stage-4 design): permanent
/// delete, move-to-trash and restore-from-trash all run here.
class DeleteWorker : public OpWorker {
    Q_OBJECT

public:
    enum class Mode {
        Remove,  ///< permanent recursive delete of `paths`
        Trash,   ///< TrashManager::trashFile per entry in `paths`
        Restore, ///< restore `items` (maps: name/originalPath/filesPath/infoPath)
    };

    DeleteWorker(Mode mode, const QStringList& paths, const QVariantList& items,
                 std::shared_ptr<OpControl> control, bool preciseList = false);

public slots:
    void run() override;

private:
    void runRemove();
    void runTrash();
    void runRestore();
    bool removeRecursive(const QString& path);
    void reportProgress(const QString& src, const QString& dest = QString(), bool force = false);

    const Mode m_mode;
    const QStringList m_paths;
    const QVariantList m_items;
    /// Undo-of-copy: delete exactly the given paths in the given order
    /// (files removed, dirs rmdir'd only when empty) — never recurse into
    /// anything the copy did not create.
    const bool m_preciseList;

    int m_filesDone = 0;
    int m_filesTotal = 0;
    int m_successes = 0;
    QElapsedTimer m_progressClock;
};

} // namespace fang
