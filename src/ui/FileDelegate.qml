import QtQuick
import Fang

// Shared delegate behavior: selection highlight, hover, click policy,
// cut-dimming, inline rename editor, drag source and (for dirs) drop
// target (docs/05).
Item {
    id: root
    required property var fileView
    property int row: -1
    property string filePath: ""
    property string fileName: ""
    property url fileUrl: ""
    property bool isDir: false
    property bool selected: false
    property bool renamePadExtension: true
    default property alias content: contentHolder.data

    readonly property var th: Application.theme
    readonly property bool renaming: fileView && fileView.renameRow === row
    readonly property bool cutDim: fileView && fileView.cutPaths.indexOf(filePath) >= 0
    readonly property bool dropHover: dropArea.enabled && dropArea.containsDrag

    Rectangle {
        anchors.fill: parent
        radius: root.th.radiusSmall
        color: root.selected ? root.th.selectionBackground
             : (mouse.containsMouse || root.dropHover) ? root.th.surfaceHover
             : "transparent"
        border.color: root.th.accent
        border.width: root.dropHover ? 1 : 0
    }

    Item {
        id: contentHolder
        anchors.fill: parent
        opacity: root.cutDim ? 0.45 : 1.0
        visible: !root.renaming
    }

    // Native drag source: text/uri-list of the full selection (or just this
    // row when it is not part of the selection), set at press time.
    Item {
        id: dragProxy
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction | Qt.MoveAction
        Drag.active: mouse.drag.active
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        enabled: !root.renaming
        drag.target: dragProxy
        onPressed: function(ev) {
            if (ev.button === Qt.LeftButton)
                dragProxy.Drag.mimeData = {
                    "text/uri-list": root.fileView.dragUriList(root.row, root.fileUrl)
                }
        }
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

    // Folders accept drops of files.
    DropArea {
        id: dropArea
        anchors.fill: parent
        enabled: root.isDir && !root.renaming
        keys: ["text/uri-list"]
        onDropped: function(drop) { root.fileView.handleDrop(drop, root.fileUrl) }
    }

    RenameEditor {
        fileView: root.fileView
        row: root.row
        fileName: root.fileName
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: Math.min(root.height, root.th.rowHeight)
    }
}
