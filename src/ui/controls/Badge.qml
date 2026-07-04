import QtQuick
import Fang

Rectangle {
    property alias text: label.text
    readonly property var th: Application.theme

    implicitWidth: Math.max(label.implicitWidth + 10, implicitHeight)
    implicitHeight: label.implicitHeight + 4
    radius: height / 2
    color: th.accent

    Text {
        id: label
        anchors.centerIn: parent
        color: th.accentForeground
        font.pointSize: th.fontSizeSmall
        font.bold: true
    }
}
