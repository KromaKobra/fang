import QtQuick
import QtQuick.Controls.Basic as QQC
import Fang

// Themed popup base: QQC2's unstyled Popup used purely for overlay/close
// behavior; all visuals are Fang's own (docs/02, docs/05).
QQC.Popup {
    id: root
    readonly property var th: Application.theme

    padding: th.spacingSmall
    closePolicy: QQC.Popup.CloseOnEscape | QQC.Popup.CloseOnPressOutside

    background: Rectangle {
        color: root.th.surface
        border.color: root.th.border
        border.width: 1
        radius: root.th.radiusMedium
    }
}
