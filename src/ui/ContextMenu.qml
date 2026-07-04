import QtQuick
import Fang
import "controls" as C

// File/background context menus (docs/05). Custom actions join in stage 8.
Item {
    id: root
    property PaneController pane: null
    property var fileView: null
    property string menuState: "file"

    readonly property var sel: pane ? pane.selection : null
    readonly property bool single: sel ? sel.count === 1 : false
    readonly property bool singleDir:
        single && pane && sel.current >= 0 && pane.model.isDirAt(sel.current)
    readonly property bool clipboardHasFiles: Application.clipboard.hasFiles

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
            visible: root.menuState === "file"
            text: "Cut"
            shortcut: "Ctrl+X"
            icon: "edit-cut"
            onTriggered: root.fileView.cutSelection()
        }
        C.MenuItem {
            visible: root.menuState === "file"
            text: "Copy"
            shortcut: "Ctrl+C"
            icon: "edit-copy"
            onTriggered: root.fileView.copySelection()
        }
        C.MenuItem {
            visible: root.menuState === "file"
            text: "Copy Path"
            onTriggered: Application.clipboard.copyPaths(root.sel.selectedUrls)
        }
        C.MenuItem {
            visible: root.menuState === "file" && root.singleDir
            enabled: root.clipboardHasFiles
            text: "Paste Into Folder"
            icon: "edit-paste"
            onTriggered: Application.clipboard.paste(
                "file://" + root.pane.model.pathAt(root.sel.current))
        }
        C.Separator { visible: root.menuState === "file" }
        C.MenuItem {
            visible: root.menuState === "file" && root.single
            text: "Rename"
            shortcut: "F2"
            icon: "edit-rename"
            onTriggered: root.fileView.startRenameCurrent()
        }
        C.MenuItem {
            visible: root.menuState === "file"
            text: "Move to Trash"
            shortcut: "Del"
            icon: "user-trash"
            onTriggered: root.fileView.trashSelection()
        }
        C.MenuItem {
            visible: root.menuState === "file"
            text: "Delete Permanently"
            shortcut: "Shift+Del"
            icon: "edit-delete"
            onTriggered: root.fileView.requestDeletePermanently()
        }

        // ---- background items
        C.MenuItem {
            visible: root.menuState === "background"
            enabled: root.clipboardHasFiles
            text: "Paste"
            shortcut: "Ctrl+V"
            icon: "edit-paste"
            onTriggered: root.fileView.pasteHere()
        }
        C.Separator { visible: root.menuState === "background" }
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
