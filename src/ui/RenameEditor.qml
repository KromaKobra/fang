import QtQuick
import Fang
import "controls" as C

// Inline rename editor (docs/05): preselects the basename (not the
// extension), commits on Enter, cancels on Escape or focus loss. Batch
// rename is Phase 2.
C.TextField {
    id: root
    required property var fileView
    property int row: -1
    property string fileName: ""
    readonly property bool renaming: fileView && fileView.renameRow === row

    visible: renaming

    onVisibleChanged: {
        if (visible) {
            text = fileName
            forceFocus()
            // Select the basename, not the extension.
            const dot = fileName.lastIndexOf(".")
            if (dot > 0)
                input.select(0, dot)
            else
                input.selectAll()
        }
    }
    onAccepted: fileView.finishRename(row, text)
    onEscapePressed: fileView.cancelRename()
    input.onActiveFocusChanged: {
        if (!input.activeFocus && renaming)
            fileView.cancelRename()
    }
}
