import QtQuick
import QtQuick.Controls.Basic as QQC
import Fang

QQC.Popup {
    id: root
    property alias text: label.text
    property Item visualParent: null

    parent: visualParent
    x: visualParent ? (visualParent.width - width) / 2 : 0
    y: visualParent ? visualParent.height + 6 : 0
    padding: 6
    closePolicy: QQC.Popup.CloseOnEscape | QQC.Popup.CloseOnPressOutsideParent
    readonly property var th: Application.theme

    Timer {
        interval: 3000
        running: root.opened
        onTriggered: root.close()
    }

    background: Rectangle {
        color: root.th.backgroundAlt
        border.color: root.th.border
        border.width: 1
        radius: root.th.radiusSmall
    }

    contentItem: Text {
        id: label
        color: root.th.foreground
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeSmall
    }
}
