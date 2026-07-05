import QtQuick
import Fang

Rectangle {
    id: root
    property string text: ""
    property string icon: ""
    property string shortcut: ""
    property bool checked: false
    property bool showChecked: false
    property var menu: null
    /// false for items that open a follow-up menu themselves.
    property bool closesMenu: true
    signal triggered()

    readonly property var th: Application.theme

    implicitWidth: row.implicitWidth + th.spacingMedium * 2
    implicitHeight: visible ? th.rowHeight : 0
    radius: th.radiusSmall
    color: mouse.containsMouse && enabled ? th.surfaceHover : "transparent"
    opacity: enabled ? 1.0 : 0.45

    Row {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        x: root.th.spacingMedium
        spacing: root.th.spacingMedium

        Text {
            visible: root.showChecked
            text: root.checked ? "✓" : " "
            width: 12
            color: root.th.accent
            font.pointSize: root.th.fontSizeNormal
            anchors.verticalCenter: parent.verticalCenter
        }
        Image {
            visible: root.icon !== ""
            width: root.th.iconSizeSmall
            height: root.th.iconSizeSmall
            anchors.verticalCenter: parent.verticalCenter
            source: root.icon ? "image://fang-icon/" + root.icon : ""
            sourceSize: Qt.size(width, height)
        }
        Text {
            text: root.text
            color: root.th.foreground
            font.family: root.th.fontFamily
            font.pointSize: root.th.fontSizeNormal
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: root.shortcut !== ""
            text: root.shortcut
            color: root.th.foregroundMuted
            font.family: root.th.fontFamily
            font.pointSize: root.th.fontSizeSmall
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if (!root.enabled)
                return
            root.triggered()
            if (root.menu && root.closesMenu)
                root.menu.close()
        }
    }
}
