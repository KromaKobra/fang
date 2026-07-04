import QtQuick
import Fang
import "controls" as C

// Icons / List / Details / Tree selector (docs/05).
Row {
    id: root
    property PaneController pane: null
    spacing: 2

    C.IconButton {
        icon: "view-list-icons"
        tooltip: "Icons (Ctrl+1)"
        checked: root.pane && root.pane.viewMode === Fang.ViewMode.Icons
        onClicked: root.pane.viewMode = Fang.ViewMode.Icons
    }
    C.IconButton {
        icon: "view-list-compact"
        tooltip: "List (Ctrl+2)"
        checked: root.pane && root.pane.viewMode === Fang.ViewMode.List
        onClicked: root.pane.viewMode = Fang.ViewMode.List
    }
    C.IconButton {
        icon: "view-list-details"
        tooltip: "Details (Ctrl+3)"
        checked: root.pane && root.pane.viewMode === Fang.ViewMode.Details
        onClicked: root.pane.viewMode = Fang.ViewMode.Details
    }
    C.IconButton {
        icon: "view-list-tree"
        tooltip: "Tree (Ctrl+4)"
        checked: root.pane && root.pane.viewMode === Fang.ViewMode.Tree
        onClicked: root.pane.viewMode = Fang.ViewMode.Tree
    }
}
