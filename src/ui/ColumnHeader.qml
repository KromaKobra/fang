import QtQuick
import Fang

// One Details/Tree header cell: click to sort, drag to reorder, drag the
// right edge to resize, right-click for the column chooser (docs/05).
Item {
    id: root
    property string key: ""
    property string label: ""
    property int columnIndex: 0
    property var detailsView: null
    property var pane: null

    readonly property var th: Application.theme
    readonly property bool sortable: detailsView && (key in detailsView.sortMap)
    readonly property bool sortedBy: sortable && pane.sortRole === detailsView.sortMap[key]

    Rectangle {
        anchors.fill: parent
        color: bodyArea.containsMouse ? root.th.surfaceHover : "transparent"
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        x: root.th.spacingMedium
        spacing: root.th.spacingSmall

        Text {
            text: root.label
            color: root.sortedBy ? root.th.foreground : root.th.foregroundMuted
            font.family: root.th.fontFamily
            font.pointSize: root.th.fontSizeNormal
            font.bold: root.sortedBy
        }
        Text {
            visible: root.sortedBy
            text: root.pane.sortDescending ? "▼" : "▲"
            color: root.th.accent
            font.pointSize: root.th.fontSizeSmall
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // Sort / reorder / chooser
    MouseArea {
        id: bodyArea
        anchors.fill: parent
        anchors.rightMargin: 6
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        property real pressX: 0
        property bool dragging: false

        onPressed: function(ev) {
            pressX = ev.x
            dragging = false
        }
        onPositionChanged: function(ev) {
            if (pressed && Math.abs(ev.x - pressX) > 12)
                dragging = true
        }
        onReleased: function(ev) {
            if (!dragging)
                return
            const headerX = root.x + ev.x
            const target = root.detailsView.headerDropIndex(headerX)
            if (target !== root.columnIndex)
                root.pane.moveColumn(root.columnIndex, target)
            dragging = false
        }
        onClicked: function(ev) {
            if (ev.button === Qt.RightButton) {
                const p = mapToItem(null, ev.x, ev.y)
                root.detailsView.openColumnsMenu(p.x, p.y)
            } else if (!dragging) {
                root.detailsView.toggleSort(root.key)
            }
        }
    }

    // Resize handle (right edge)
    MouseArea {
        width: 8
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        cursorShape: Qt.SplitHCursor
        preventStealing: true
        property real startWidth: 0
        property real pressSceneX: 0
        onPressed: function(ev) {
            startWidth = root.width
            pressSceneX = mapToItem(null, ev.x, 0).x
        }
        onPositionChanged: function(ev) {
            if (!pressed)
                return
            const sceneX = mapToItem(null, ev.x, 0).x
            root.pane.setColumnWidth(root.key, Math.round(startWidth + sceneX - pressSceneX))
        }
    }
}
