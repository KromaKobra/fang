#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace fang {

Q_NAMESPACE
QML_NAMED_ELEMENT(Fang)

enum class ViewMode {
    Icons,
    List,
    Details,
    Tree,
};
Q_ENUM_NS(ViewMode)

enum class SortRole {
    ByName,
    BySize,
    ByType,
    ByModified,
    ByCreated,
    ByAccessed,
    ByExtension,
    ByOwner,
};
Q_ENUM_NS(SortRole)

enum class OpType {
    Copy,
    Move,
    Delete,
    Trash,
    Restore,
    Rename,
    CreateFolder,
};
Q_ENUM_NS(OpType)

enum class OpState {
    Queued,
    Running,
    Paused,
    WaitingConflict,
    Completed,
    Failed,
    Cancelled,
};
Q_ENUM_NS(OpState)

enum class ConflictMode {
    Overwrite,
    Skip,
    Rename,
    Merge,
    KeepBoth,
};
Q_ENUM_NS(ConflictMode)

enum class ClipboardMode {
    Copy,
    Cut,
};
Q_ENUM_NS(ClipboardMode)

enum class PlaceSection {
    Places,
    Bookmarks,
    Devices,
    Network,
    Trash,
};
Q_ENUM_NS(PlaceSection)

} // namespace fang
