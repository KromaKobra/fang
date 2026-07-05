import QtQuick
import Fang
import "controls" as C

// File/background context menus with custom actions and the Open With
// flyout (docs/05).
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

    // Snapshot per popup: ActionRegistry entries for the current selection
    // (or the background when it is empty) and Open With candidates.
    property var customActions: []
    property var openWithApps: []
    property real lastX: 0
    property real lastY: 0

    function popupFile(sceneX, sceneY) {
        menuState = "file"
        lastX = sceneX
        lastY = sceneY
        customActions = Application.actions.actionsFor(sel)
        const path = sel.count > 0 && sel.current >= 0 ? pane.model.pathAt(sel.current) : ""
        openWithApps = path !== "" ? MimeResolver.appsFor(MimeResolver.mimeTypeFor(path)) : []
        menu.popup(sceneX, sceneY)
    }
    function popupBackground(sceneX, sceneY) {
        menuState = "background"
        lastX = sceneX
        lastY = sceneY
        // null = background context regardless of a lingering selection;
        // run() still gets the selection for %d (current directory).
        customActions = Application.actions.actionsFor(null)
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
            visible: root.menuState === "file"
            text: "Open With…"
            icon: "system-run"
            onTriggered: openWithMenu.popup(root.lastX, root.lastY)
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

        // ---- custom actions (ActionRegistry, above Properties per docs/05)
        C.Separator { visible: root.customActions.length > 0 }
        Repeater {
            model: root.customActions
            delegate: C.MenuItem {
                required property var modelData
                text: modelData.hasSubmenu
                      ? modelData.submenuLabel + " · " + modelData.label
                      : modelData.label
                icon: modelData.iconName
                onTriggered: Application.actions.run(modelData.id, root.sel)
            }
        }

        C.Separator {}
        C.MenuItem {
            text: "Properties"
            shortcut: "Alt+Enter"
            icon: "document-properties"
            onTriggered: root.fileView.requestProperties()
        }
    }

    // Open With flyout: replaces the main menu at the same position.
    C.Menu {
        id: openWithMenu

        Repeater {
            model: root.openWithApps
            delegate: C.MenuItem {
                required property var modelData
                text: modelData.name
                icon: modelData.iconName
                onTriggered: MimeResolver.openWith(root.sel.selectedUrls, modelData.id)
            }
        }
        C.Separator { visible: root.openWithApps.length > 0 }
        C.MenuItem {
            text: "Other Application…"
            onTriggered: root.fileView.requestOpenWith()
        }
    }
}
