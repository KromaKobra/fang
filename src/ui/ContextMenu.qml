import QtQuick
import Fang
import "controls" as C

// File/background context menus (docs/05). Clipboard/trash/actions items
// join in stages 4 and 8.
Item {
    id: root
    property PaneController pane: null
    property var fileView: null
    property string menuState: "file"

    readonly property var sel: pane ? pane.selection : null
    readonly property bool single: sel ? sel.count === 1 : false

    function popupFile(sceneX, sceneY) {
        menuState = "file"
        menu.popup(sceneX, sceneY)
    }
    function popupBackground(sceneX, sceneY) {
        menuState = "background"
        menu.popup(sceneX, sceneY)
    }

    C.Menu {
        id: menu

        // ---- file items
        C.MenuItem {
            visible: root.menuState === "file"
            text: "Open"
            icon: "document-open"
            onTriggered: if (root.sel.current >= 0) root.pane.activate(root.sel.current)
        }
        C.MenuItem {
            visible: root.menuState === "file" && root.single
            text: "Open in New Tab"
            icon: "tab-new"
            onTriggered: root.pane.openInNewTab(root.sel.current)
        }
        C.MenuItem {
            visible: root.menuState === "file" && root.single
            text: "Open in New Window"
            icon: "window-new"
            onTriggered: root.pane.openInNewWindow(root.sel.current)
        }
        C.Separator { visible: root.menuState === "file" }
        C.MenuItem {
            visible: root.menuState === "file" && root.single
            text: "Rename"
            shortcut: "F2"
            icon: "edit-rename"
            onTriggered: root.fileView.startRenameCurrent()
        }

        // ---- background items
        C.MenuItem {
            visible: root.menuState === "background"
            text: "New Folder"
            shortcut: "Ctrl+Shift+N"
            icon: "folder-new"
            onTriggered: root.fileView.requestCreateFolder()
        }
        C.MenuItem {
            visible: root.menuState === "background"
            text: "New File"
            icon: "document-new"
            onTriggered: root.fileView.requestCreateFile()
        }
        C.Separator { visible: root.menuState === "background" }
        C.MenuItem {
            visible: root.menuState === "background"
            text: "Show Hidden Files"
            shortcut: "Ctrl+H"
            showChecked: true
            checked: root.pane ? root.pane.showHidden : false
            onTriggered: root.pane.showHidden = !root.pane.showHidden
        }
        C.MenuItem {
            visible: root.menuState === "background"
            text: "Refresh"
            shortcut: "F5"
            icon: "view-refresh"
            onTriggered: root.pane.refresh()
        }
    }
}
