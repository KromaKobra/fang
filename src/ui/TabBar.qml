import QtQuick
import Fang
import "controls" as C

// Tab strip: one TabButton per tab, a "+" button, middle-click close,
// double-click on empty space adds a tab, drag to reorder (docs/05).
Rectangle {
    id: root
    property WindowController win: null

    readonly property var th: Application.theme

    implicitHeight: th.rowHeight + th.spacingSmall
    color: th.background

    // Double-click on the empty strip adds a tab.
    MouseArea {
        anchors.fill: parent
        onDoubleClicked: root.win.addTab(root.currentLocation())
    }

    function currentLocation() {
        const tab = root.win ? root.win.activeTab : null
        return tab && tab.activePane ? tab.activePane.location : ""
    }

    Row {
        id: tabRow
        anchors.left: parent.left
        anchors.leftMargin: root.th.spacingSmall
        anchors.bottom: parent.bottom
        spacing: 2

        Repeater {
            id: tabRepeater
            model: root.win ? root.win.tabCount : 0
            delegate: TabButton {
                id: tabBtn
                required property int index
                tab: root.win.tabAt(index)
                active: index === root.win.activeTabIndex
                onClicked: root.win.activeTabIndex = index
                onCloseRequested: root.win.closeTab(index)
                onReorderRequested: function(offsetX) {
                    // Walk sibling widths to find how many slots we crossed.
                    let shift = 0
                    let remaining = Math.abs(offsetX)
                    const dir = offsetX > 0 ? 1 : -1
                    let i = index + dir
                    while (i >= 0 && i < root.win.tabCount) {
                        const item = tabRepeater.itemAt(i)
                        if (!item || remaining < item.width / 2)
                            break
                        remaining -= item.width + tabRow.spacing
                        shift += dir
                        i += dir
                    }
                    if (shift !== 0)
                        root.win.moveTab(index, index + shift)
                }
            }
        }
    }

    C.IconButton {
        anchors.left: tabRow.right
        anchors.leftMargin: root.th.spacingSmall
        anchors.verticalCenter: parent.verticalCenter
        icon: "list-add"
        tooltip: "New Tab (Ctrl+T)"
        onClicked: root.win.addTab(root.currentLocation())
    }

    C.Separator { anchors.bottom: parent.bottom }
}
