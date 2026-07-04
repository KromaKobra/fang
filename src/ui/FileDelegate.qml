import QtQuick
import Fang
import "controls" as C

// Shared delegate behavior: selection highlight, hover, click policy,
// cut-dimming, inline rename editor (docs/05).
Item {
    id: root
    required property var fileView
    property int row: -1
    property string filePath: ""
    property string fileName: ""
    property bool selected: false
    property bool renamePadExtension: true
    default property alias content: contentHolder.data

    readonly property var th: Application.theme
    readonly property bool renaming: fileView && fileView.renameRow === row
    readonly property bool cutDim: fileView && fileView.cutPaths.indexOf(filePath) >= 0

    Rectangle {
        anchors.fill: parent
        radius: root.th.radiusSmall
        color: root.selected ? root.th.selectionBackground
             : mouse.containsMouse ? root.th.surfaceHover
             : "transparent"
    }

    Item {
        id: contentHolder
        anchors.fill: parent
        opacity: root.cutDim ? 0.45 : 1.0
        visible: !root.renaming
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        enabled: !root.renaming
        onClicked: function(ev) {
            if (ev.button === Qt.RightButton) {
                const p = mapToItem(null, ev.x, ev.y)
                root.fileView.rightClickRow(root.row, p.x, p.y)
            } else if (ev.button === Qt.MiddleButton) {
                root.fileView.middleClickRow(root.row)
            } else {
                root.fileView.clickRow(root.row, ev.modifiers)
            }
        }
        onDoubleClicked: function(ev) {
            if (ev.button === Qt.LeftButton)
                root.fileView.doubleClickRow(root.row)
        }
    }

    // Inline rename editor
    C.TextField {
        id: renameField
        visible: root.renaming
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: Math.min(root.height, root.th.rowHeight)

        onVisibleChanged: {
            if (visible) {
                text = root.fileName
                forceFocus()
                // Select the basename, not the extension.
                const dot = root.fileName.lastIndexOf(".")
                if (dot > 0)
                    input.select(0, dot)
                else
                    input.selectAll()
            }
        }
        onAccepted: {
            const newName = text
            root.fileView.finishRename(root.row, newName)
        }
        onEscapePressed: root.fileView.cancelRename()
        input.onActiveFocusChanged: {
            if (!input.activeFocus && root.renaming)
                root.fileView.cancelRename()
        }
    }
}
