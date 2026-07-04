import QtQuick
import Fang

Rectangle {
    id: root
    property alias text: input.text
    property alias input: input
    property string placeholder: ""
    property bool selectOnFocus: false
    signal accepted()
    signal escapePressed()

    readonly property var th: Application.theme

    implicitHeight: th.rowHeight
    implicitWidth: 200
    radius: th.radiusSmall
    color: th.backgroundAlt
    border.color: input.activeFocus ? th.accent : th.border
    border.width: 1

    function forceFocus() {
        input.forceActiveFocus()
        if (selectOnFocus)
            input.selectAll()
    }

    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: root.th.spacingMedium
        anchors.rightMargin: root.th.spacingMedium
        verticalAlignment: TextInput.AlignVCenter
        color: root.th.foreground
        selectionColor: root.th.selectionBackground
        selectedTextColor: root.th.selectionForeground
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeNormal
        clip: true
        onAccepted: root.accepted()
        Keys.onEscapePressed: root.escapePressed()
    }

    Text {
        anchors.verticalCenter: parent.verticalCenter
        x: root.th.spacingMedium
        text: root.placeholder
        color: root.th.foregroundMuted
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeNormal
        visible: input.text.length === 0 && !input.activeFocus
    }
}
