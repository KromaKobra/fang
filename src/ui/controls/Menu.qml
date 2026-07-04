import QtQuick
import QtQuick.Controls.Basic as QQC
import Fang

// Themed menu: a popup column of MenuItem/Separator children.
QQC.Popup {
    id: root
    default property alias content: column.data
    readonly property var th: Application.theme

    padding: th.spacingSmall
    modal: true
    dim: false
    closePolicy: QQC.Popup.CloseOnEscape | QQC.Popup.CloseOnPressOutside

    // Open at scene coordinates (e.g. from a right-click).
    function popup(sceneX, sceneY) {
        parent = QQC.Overlay.overlay
        x = Math.min(sceneX, parent.width - width - 4)
        y = Math.min(sceneY, parent.height - height - 4)
        open()
    }
    // Open below an item.
    function popupBelow(item) {
        parent = QQC.Overlay.overlay
        const p = item.mapToItem(parent, 0, item.height + 2)
        x = Math.min(p.x, parent.width - width - 4)
        y = Math.min(p.y, parent.height - height - 4)
        open()
    }

    onOpened: relayout()
    function relayout() {
        let w = 140
        for (let i = 0; i < column.children.length; ++i) {
            const c = column.children[i]
            if (c.visible && c.implicitWidth)
                w = Math.max(w, c.implicitWidth)
        }
        contentWidth = w
        // Give every child the menu (for auto-close) and full width.
        for (let i = 0; i < column.children.length; ++i) {
            const c = column.children[i]
            if (c.hasOwnProperty("menu"))
                c.menu = root
            c.width = w
        }
    }

    background: Rectangle {
        color: root.th.surface
        border.color: root.th.border
        border.width: 1
        radius: root.th.radiusMedium
    }

    contentItem: Column {
        id: column
        spacing: 1
    }
}
