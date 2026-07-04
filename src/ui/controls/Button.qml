import QtQuick
import Fang

Rectangle {
    id: root
    property alias text: label.text
    property bool accent: false
    property bool enabled: true
    signal clicked()

    readonly property var th: Application.theme

    implicitWidth: label.implicitWidth + th.spacingLarge * 2
    implicitHeight: th.rowHeight
    radius: th.radiusSmall
    color: !enabled ? th.surface
         : mouse.pressed ? (accent ? Qt.darker(th.accent, 1.2) : th.surfaceActive)
         : mouse.containsMouse ? (accent ? Qt.lighter(th.accent, 1.1) : th.surfaceHover)
         : (accent ? th.accent : th.surface)
    border.color: accent ? "transparent" : th.border
    border.width: 1
    opacity: enabled ? 1.0 : 0.5

    Text {
        id: label
        anchors.centerIn: parent
        color: root.accent ? root.th.accentForeground : root.th.foreground
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeNormal
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: if (root.enabled) root.clicked()
    }
}
