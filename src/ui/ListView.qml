import QtQuick as QQ
import QtQuick
import Fang
import "controls" as C

// Compact view: single-line names flowing top-to-bottom, wrapping into
// additional columns (docs/05).
Item {
    id: root
    required property var fileView
    required property PaneController pane

    readonly property var th: Application.theme
    readonly property int columnsPerRow: 1 // vertical-first navigation

    function ensureVisible(row) {
        grid.positionViewAtIndex(row, QQ.GridView.Contain)
    }

    MouseArea {
        anchors.fill: grid
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(ev) {
            if (ev.button === Qt.RightButton) {
                const p = mapToItem(null, ev.x, ev.y)
                root.fileView.backgroundRightClick(p.x, p.y)
            } else {
                root.fileView.backgroundClick()
            }
        }
    }

    QQ.GridView {
        id: grid
        anchors.fill: parent
        anchors.bottomMargin: 12
        clip: true
        model: root.pane.model
        flow: QQ.GridView.FlowTopToBottom
        cellWidth: 230
        cellHeight: root.th.rowHeight
        boundsBehavior: QQ.Flickable.StopAtBounds

        delegate: FileDelegate {
            id: dele
            required property var model
            required property int index

            fileView: root.fileView
            row: index
            filePath: model.path
            fileName: model.name
            fileUrl: model.url
            isDir: model.isDir
            selected: model.selected
            width: grid.cellWidth - 4
            height: grid.cellHeight

            Row {
                anchors.fill: parent
                anchors.leftMargin: root.th.spacingSmall
                spacing: root.th.spacingMedium

                Image {
                    width: root.th.iconSizeSmall
                    height: root.th.iconSizeSmall
                    anchors.verticalCenter: parent.verticalCenter
                    asynchronous: true
                    source: "image://fang-icon/" + dele.model.iconName
                    sourceSize: Qt.size(width, height)
                }
                Text {
                    width: parent.width - root.th.iconSizeSmall - root.th.spacingMedium * 2
                    anchors.verticalCenter: parent.verticalCenter
                    text: dele.model.name
                    color: dele.selected ? root.th.selectionForeground : root.th.foreground
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeNormal
                    font.italic: dele.model.isSymlink
                    elide: Text.ElideRight
                }
            }
        }
    }

    C.ScrollBar {
        target: grid
        horizontal: true
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
