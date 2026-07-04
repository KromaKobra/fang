import QtQuick
import Fang

// One tab in the TabBar: title, close button, middle-click close, active
// highlight; drag horizontally to reorder (docs/05). The drag moves the
// button via a Translate transform (a Row positioner owns x) and the TabBar
// applies the reorder on release.
Rectangle {
    id: root
    property var tab: null // TabController
    property bool active: false
    signal clicked()
    signal closeRequested()
    /// offsetX: how far the tab was dragged from its slot when released.
    signal reorderRequested(real offsetX)

    readonly property var th: Application.theme
    property real dragOffset: 0

    implicitWidth: Math.min(220, titleLabel.implicitWidth + closeBtn.width
                            + th.spacingMedium * 3)
    implicitHeight: th.rowHeight
    radius: th.radiusSmall
    z: dragOffset !== 0 ? 2 : 0
    transform: Translate { x: root.dragOffset }
    color: active ? th.surface
         : mouse.containsMouse ? th.surfaceHover
         : "transparent"

    Text {
        id: titleLabel
        anchors.left: parent.left
        anchors.right: closeBtn.left
        anchors.leftMargin: root.th.spacingMedium
        anchors.verticalCenter: parent.verticalCenter
        text: root.tab ? root.tab.title : ""
        elide: Text.ElideRight
        color: root.active ? root.th.foreground : root.th.foregroundMuted
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeNormal
    }

    Rectangle {
        id: closeBtn
        anchors.right: parent.right
        anchors.rightMargin: root.th.spacingSmall
        anchors.verticalCenter: parent.verticalCenter
        width: 18
        height: 18
        radius: root.th.radiusSmall
        color: closeMouse.containsMouse ? root.th.surfaceActive : "transparent"

        Text {
            anchors.centerIn: parent
            text: "×"
            color: closeMouse.containsMouse ? root.th.foreground : root.th.foregroundMuted
            font.pointSize: root.th.fontSizeNormal
        }
        MouseArea {
            id: closeMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.closeRequested()
        }
    }

    // Bottom accent line marks the active tab.
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 2
        color: root.active ? root.th.accent : "transparent"
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        anchors.rightMargin: closeBtn.width
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        preventStealing: true
        property real pressX: 0
        property bool dragging: false
        onPressed: function(ev) {
            pressX = ev.x
            dragging = false
            if (ev.button === Qt.LeftButton)
                root.clicked() // activate on press, like real tab bars
        }
        onPositionChanged: function(ev) {
            if (!pressed || !(ev.buttons & Qt.LeftButton))
                return
            if (!dragging && Math.abs(ev.x - pressX) > 8)
                dragging = true
            if (dragging)
                root.dragOffset = ev.x - pressX
        }
        onReleased: function(ev) {
            if (dragging) {
                const off = root.dragOffset
                root.dragOffset = 0
                dragging = false
                root.reorderRequested(off)
            } else if (ev.button === Qt.MiddleButton) {
                root.closeRequested()
            }
        }
    }
}
