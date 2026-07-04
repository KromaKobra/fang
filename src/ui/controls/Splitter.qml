import QtQuick
import Fang

// Draggable divider. Emits dragged(delta) with the movement since the last
// event; the parent adjusts the width/height it controls.
Rectangle {
    id: root
    property bool vertical: true // vertical bar dividing horizontal siblings
    signal dragged(real delta)

    readonly property var th: Application.theme

    implicitWidth: vertical ? 5 : 100
    implicitHeight: vertical ? 100 : 5
    color: mouse.pressed ? th.accent : mouse.containsMouse ? th.surfaceHover : "transparent"

    Rectangle {
        anchors.centerIn: parent
        width: root.vertical ? 1 : parent.width
        height: root.vertical ? parent.height : 1
        color: root.th.border
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.vertical ? Qt.SplitHCursor : Qt.SplitVCursor
        preventStealing: true
        property real lastPos: 0
        onPressed: function(ev) { lastPos = root.vertical ? ev.x : ev.y }
        onPositionChanged: function(ev) {
            if (!pressed)
                return
            const pos = root.vertical ? ev.x : ev.y
            root.dragged(pos - lastPos)
        }
    }
}
