import QtQuick
import Fang

Rectangle {
    id: root
    property string icon: ""
    property string tooltip: ""
    property bool enabled: true
    property bool checked: false
    property bool checkable: false
    property int iconSize: Application.theme.iconSizeSmall
    signal clicked()
    signal rightClicked()

    readonly property var th: Application.theme

    implicitWidth: iconSize + th.spacingMedium * 2
    implicitHeight: implicitWidth
    radius: th.radiusSmall
    color: checked ? th.surfaceActive
         : mouse.pressed && enabled ? th.surfaceActive
         : mouse.containsMouse && enabled ? th.surfaceHover
         : "transparent"

    Image {
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
        source: root.icon ? "image://fang-icon/" + root.icon : ""
        sourceSize: Qt.size(width, height)
        opacity: root.enabled ? 1.0 : 0.35
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(ev) {
            if (!root.enabled)
                return
            if (ev.button === Qt.RightButton) {
                root.rightClicked()
            } else {
                if (root.checkable)
                    root.checked = !root.checked
                root.clicked()
            }
        }
        onExited: tipTimer.stop()
        onEntered: if (root.tooltip !== "") tipTimer.restart()
    }

    Timer {
        id: tipTimer
        interval: 650
        onTriggered: tip.open()
    }

    Tooltip {
        id: tip
        text: root.tooltip
        visualParent: root
    }
}
