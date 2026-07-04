import QtQuick
import Fang

// Icon + optional label toolbar button.
Rectangle {
    id: root
    property string icon: ""
    property string text: ""
    property string tooltip: ""
    property bool enabled: true
    property bool checked: false
    signal clicked()

    readonly property var th: Application.theme

    implicitWidth: row.implicitWidth + th.spacingMedium * 2
    implicitHeight: th.iconSizeSmall + th.spacingMedium * 2
    radius: th.radiusSmall
    color: checked ? th.surfaceActive
         : mouse.pressed && enabled ? th.surfaceActive
         : mouse.containsMouse && enabled ? th.surfaceHover
         : "transparent"

    Row {
        id: row
        anchors.centerIn: parent
        spacing: root.th.spacingSmall

        Image {
            visible: root.icon !== ""
            width: root.th.iconSizeSmall
            height: root.th.iconSizeSmall
            anchors.verticalCenter: parent.verticalCenter
            source: root.icon ? "image://fang-icon/" + root.icon : ""
            sourceSize: Qt.size(width, height)
            opacity: root.enabled ? 1.0 : 0.35
        }
        Text {
            visible: root.text !== ""
            text: root.text
            anchors.verticalCenter: parent.verticalCenter
            color: root.th.foreground
            font.family: root.th.fontFamily
            font.pointSize: root.th.fontSizeNormal
            opacity: root.enabled ? 1.0 : 0.35
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: if (root.enabled) root.clicked()
        onEntered: if (root.tooltip !== "") tipTimer.restart()
        onExited: tipTimer.stop()
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
