import QtQuick
import Fang
import "controls" as C

// Item/selection counts, zoom slider, free space (docs/05).
Rectangle {
    id: root
    property PaneController pane: null

    readonly property var th: Application.theme
    readonly property var sel: pane ? pane.selection : null

    implicitHeight: th.rowHeight
    color: th.backgroundAlt

    C.Separator { anchors.top: parent.top }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: root.th.spacingMedium
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: zoomRow.left
        anchors.rightMargin: root.th.spacingMedium
        elide: Text.ElideRight
        color: root.th.foregroundMuted
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeSmall
        text: {
            if (!root.pane)
                return ""
            let s = root.pane.itemCount === 1 ? "1 item"
                                              : root.pane.itemCount + " items"
            if (root.sel && root.sel.count > 0)
                s += ", " + root.sel.count + " selected ("
                    + Application.formatSize(root.sel.selectedSize) + ")"
            if (root.pane.filterText !== "")
                s += "  —  filter: \"" + root.pane.filterText + "\""
            return s
        }
    }

    Row {
        id: zoomRow
        anchors.right: freeText.left
        anchors.rightMargin: root.th.spacingLarge
        anchors.verticalCenter: parent.verticalCenter
        spacing: root.th.spacingSmall

        Text {
            text: "−"
            color: root.th.foregroundMuted
            font.pointSize: root.th.fontSizeSmall
            anchors.verticalCenter: parent.verticalCenter
        }
        C.Slider {
            anchors.verticalCenter: parent.verticalCenter
            width: 90
            from: 16
            to: 256
            value: root.pane ? root.pane.iconSize : 48
            onMoved: function(v) { if (root.pane) root.pane.iconSize = Math.round(v) }
        }
        Text {
            text: "+"
            color: root.th.foregroundMuted
            font.pointSize: root.th.fontSizeSmall
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Text {
        id: freeText
        anchors.right: parent.right
        anchors.rightMargin: root.th.spacingMedium
        anchors.verticalCenter: parent.verticalCenter
        color: root.th.foregroundMuted
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeSmall
        text: root.pane && root.pane.freeBytes >= 0
              ? Application.formatSize(root.pane.freeBytes) + " free"
              : ""
    }
}
