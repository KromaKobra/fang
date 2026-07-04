import QtQuick as QQ
import QtQuick
import Fang
import "controls" as C

// Grid of thumbnails/icons with names (docs/05).
Item {
    id: root
    required property var fileView
    required property PaneController pane

    readonly property var th: Application.theme
    readonly property int columnsPerRow: Math.max(1, Math.floor(grid.width / grid.cellWidth))

    function ensureVisible(row) {
        grid.positionViewAtIndex(row, QQ.GridView.Contain)
    }

    // Empty-area interaction + rubber band (behind the view; clicks on empty
    // spots fall through the grid onto this).
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
        anchors.rightMargin: 12
        clip: true
        model: root.pane.model
        cellWidth: root.pane.iconSize + root.th.spacingLarge * 3
        cellHeight: root.pane.iconSize + root.th.spacingLarge * 2 + root.th.fontSizeNormal * 3
        cacheBuffer: 400
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
            width: grid.cellWidth - 6
            height: grid.cellHeight - 6

            Column {
                anchors.fill: parent
                anchors.margins: root.th.spacingSmall
                spacing: root.th.spacingSmall

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: root.pane.iconSize
                    height: root.pane.iconSize
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    source: dele.model.thumbnail !== ""
                        ? dele.model.thumbnail
                        : "image://fang-icon/" + dele.model.iconName
                    sourceSize: Qt.size(width, height)
                }
                Text {
                    width: parent.width
                    text: dele.model.name
                    color: dele.selected ? root.th.selectionForeground : root.th.foreground
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeNormal
                    font.italic: dele.model.isSymlink
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideMiddle
                    maximumLineCount: 2
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }
        }
    }

    // Rubber-band selection overlay
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
                const ix = item.x
                const iy = item.y - grid.contentY
                if (ix < x + width && ix + item.width > x
                        && iy < y + height && iy + item.height > y)
                    sel.toggle(i)
            }
        }
        function end() { active = false }
    }

    C.ScrollBar {
        target: grid
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }
}
