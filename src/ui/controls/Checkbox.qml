import QtQuick
import Fang

Item {
    id: root
    property bool checked: false
    property alias text: label.text
    signal toggled(bool checked)

    readonly property var th: Application.theme

    implicitWidth: box.width + (label.text !== "" ? label.implicitWidth + th.spacingMedium : 0)
    implicitHeight: Math.max(box.height, label.implicitHeight)

    Rectangle {
        id: box
        width: 16
        height: 16
        anchors.verticalCenter: parent.verticalCenter
        radius: 3
        color: root.checked ? root.th.accent : root.th.backgroundAlt
        border.color: root.checked ? root.th.accent : root.th.border
        border.width: 1

        Text {
            anchors.centerIn: parent
            visible: root.checked
            text: "✓"
            color: root.th.accentForeground
            font.pointSize: root.th.fontSizeSmall
            font.bold: true
        }
    }

    Text {
        id: label
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: box.right
        anchors.leftMargin: root.th.spacingMedium
        color: root.th.foreground
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeNormal
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.checked = !root.checked
            root.toggled(root.checked)
        }
    }
}
