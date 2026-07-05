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
        onPressed: function(ev) {
            if (ev.button === Qt.LeftButton)
                rubber.begin(ev.x, ev.y)
        }
        onPositionChanged: function(ev) {
            if (pressed && (ev.buttons & Qt.LeftButton))
                rubber.update(ev.x, ev.y)
        }
        onReleased: rubber.end()
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

    // Rubber-band selection overlay (columns scroll horizontally here).
    Rectangle {
        id: rubber
        property real startX: 0
        property real startY: 0
        property bool active: false
        visible: active && (width > 4 || height > 4)
        color: Qt.alpha(root.th.accent, 0.15)
        border.color: root.th.accent
        border.width: 1
        z: 10

        function begin(x0, y0) {
            startX = x0; startY = y0
            x = x0; y = y0
            width = 0; height = 0
            active = true
        }
        function update(x1, y1) {
            if (!active)
                return
            x = Math.min(startX, x1)
            y = Math.min(startY, y1)
            width = Math.abs(x1 - startX)
            height = Math.abs(y1 - startY)
            const sel = root.pane.selection
            sel.clear()
            for (let i = 0; i < root.pane.itemCount; ++i) {
                const item = grid.itemAtIndex(i)
                if (!item)
                    continue
                const ix = item.x - grid.contentX
                const iy = item.y
                if (ix < x + width && ix + item.width > x
                        && iy < y + height && iy + item.height > y)
                    sel.toggle(i)
            }
        }
        function end() { active = false }
    }

    C.ScrollBar {
        target: grid
        horizontal: true
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
