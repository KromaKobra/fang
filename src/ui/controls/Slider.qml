import QtQuick
import Fang

Item {
    id: root
    property real from: 0
    property real to: 100
    property real value: 0
    signal moved(real value)

    readonly property var th: Application.theme
    readonly property real frac: to > from ? (value - from) / (to - from) : 0

    implicitWidth: 120
    implicitHeight: 18

    Rectangle {
        id: track
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: 4
        radius: 2
        color: root.th.surfaceActive
    }
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: track.width * root.frac
        height: 4
        radius: 2
        color: root.th.accent
    }
    Rectangle {
        id: handle
        x: (parent.width - width) * root.frac
        anchors.verticalCenter: parent.verticalCenter
        width: 12
        height: 12
        radius: 6
        color: mouse.pressed || mouse.containsMouse ? root.th.accent : root.th.foregroundMuted
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true
        function apply(x) {
            const f = Math.min(1, Math.max(0, x / root.width))
            const v = root.from + f * (root.to - root.from)
            root.value = v
            root.moved(v)
        }
        onPressed: function(ev) { apply(ev.x) }
        onPositionChanged: function(ev) { if (pressed) apply(ev.x) }
    }
}
