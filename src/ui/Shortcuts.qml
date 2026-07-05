import QtQuick
import Fang

// Central shortcut map (docs/05). Defaults below; any entry in config.toml's
// [shortcuts] table overrides by name, e.g. `new_tab = "Ctrl+T"`.
Item {
    id: root
    property WindowController win: null
    property PaneController pane: null
    /// The active pane's FileView (for rename/clipboard/trash).
    property var fileView: null
    signal locationBarRequested()
    signal searchRequested()
    signal createFolderRequested()
    signal createFileRequested()
    signal previewToggleRequested()
    signal propertiesRequested()

    readonly property var nav: pane ? pane.navigation : null
    readonly property var sel: pane ? pane.selection : null
    /// File shortcuts only fire while the file view has focus and no inline
    /// rename is running (otherwise Del/Ctrl+C belong to the text editor).
    readonly property bool fileKeysActive:
        fileView && fileView.activeFocus && fileView.renameRow < 0

    /// Bumped when [shortcuts] overrides change; key() reads it so every
    /// sequence binding re-evaluates.
    property int keymapRevision: 0
    Connections {
        target: Application.settings
        function onShortcutsChanged() { root.keymapRevision++ }
    }
    function key(name, fallback) {
        root.keymapRevision
        return Application.settings.shortcut(name, fallback)
    }

    function zoomBy(delta) {
        if (pane)
            pane.iconSize = Math.min(256, Math.max(16, pane.iconSize + delta))
    }

    // ------------------------------------------------------ tabs & window
    Shortcut {
        sequence: root.key("new_tab", "Ctrl+T")
        onActivated: root.win.addTab(root.pane ? root.pane.location : "")
    }
    Shortcut {
        sequence: root.key("close_tab", "Ctrl+W")
        onActivated: root.win.closeTab(root.win.activeTabIndex)
    }
    Shortcut {
        sequence: root.key("new_window", "Ctrl+N")
        onActivated: Application.openPath(root.pane ? root.pane.location : "")
    }
    Shortcut {
        sequence: root.key("next_tab", "Ctrl+Tab")
        onActivated: root.win.activeTabIndex =
            (root.win.activeTabIndex + 1) % root.win.tabCount
    }
    Shortcut {
        sequence: root.key("prev_tab", "Ctrl+Shift+Tab")
        onActivated: root.win.activeTabIndex =
            (root.win.activeTabIndex + root.win.tabCount - 1) % root.win.tabCount
    }
    Shortcut { sequence: root.key("quit", "Ctrl+Q"); onActivated: Application.quit() }

    // ----------------------------------------------------------- navigate
    Shortcut {
        sequence: root.key("back", "Alt+Left")
        onActivated: if (root.nav) root.nav.back()
    }
    Shortcut {
        sequence: root.key("forward", "Alt+Right")
        onActivated: if (root.nav) root.nav.forward()
    }
    Shortcut { sequence: root.key("up", "Alt+Up"); onActivated: if (root.nav) root.nav.up() }
    Shortcut {
        sequence: root.key("reload", "F5")
        onActivated: if (root.nav) root.nav.refresh()
    }
    Shortcut {
        sequence: root.key("location_bar", "Ctrl+L")
        onActivated: root.locationBarRequested()
    }
    Shortcut {
        sequence: root.key("focus_search", "Ctrl+F")
        onActivated: root.searchRequested()
    }

    // --------------------------------------------------------------- view
    Shortcut {
        sequence: root.key("toggle_hidden", "Ctrl+H")
        onActivated: if (root.pane) root.pane.showHidden = !root.pane.showHidden
    }
    Shortcut { sequence: root.key("toggle_split", "F3"); onActivated: root.win.toggleSplit() }
    Shortcut {
        sequence: root.key("toggle_preview", "F11")
        onActivated: root.previewToggleRequested()
    }
    Shortcut {
        sequence: root.key("toggle_preview_alt", "F7")
        onActivated: root.previewToggleRequested()
    }
    Shortcut {
        sequence: root.key("view_icons", "Ctrl+1")
        onActivated: if (root.pane) root.pane.viewMode = Fang.ViewMode.Icons
    }
    Shortcut {
        sequence: root.key("view_list", "Ctrl+2")
        onActivated: if (root.pane) root.pane.viewMode = Fang.ViewMode.List
    }
    Shortcut {
        sequence: root.key("view_details", "Ctrl+3")
        onActivated: if (root.pane) root.pane.viewMode = Fang.ViewMode.Details
    }
    Shortcut {
        sequence: root.key("view_tree", "Ctrl+4")
        onActivated: if (root.pane) root.pane.viewMode = Fang.ViewMode.Tree
    }
    Shortcut { sequence: root.key("zoom_in", "Ctrl++"); onActivated: root.zoomBy(8) }
    Shortcut { sequence: "Ctrl+="; onActivated: root.zoomBy(8) }
    Shortcut { sequence: root.key("zoom_out", "Ctrl+-"); onActivated: root.zoomBy(-8) }
    Shortcut {
        sequence: root.key("zoom_reset", "Ctrl+0")
        onActivated: if (root.pane) root.pane.iconSize = 48
    }

    // ---------------------------------------------------------- selection
    Shortcut {
        sequence: root.key("select_all", "Ctrl+A")
        onActivated: if (root.sel) root.sel.selectAll()
    }
    Shortcut {
        sequence: root.key("invert_selection", "Ctrl+Shift+A")
        onActivated: if (root.sel) root.sel.invert()
    }

    // ------------------------------------------------------------- files
    Shortcut {
        sequence: root.key("rename", "F2")
        onActivated: if (root.fileView) root.fileView.startRenameCurrent()
    }
    Shortcut {
        sequence: root.key("new_folder", "Ctrl+Shift+N")
        onActivated: root.createFolderRequested()
    }
    Shortcut {
        sequence: root.key("properties", "Alt+Return")
        onActivated: root.propertiesRequested()
    }

    // -------------------------------------------------- clipboard & trash
    Shortcut {
        sequence: root.key("copy", "Ctrl+C")
        onActivated: if (root.fileKeysActive) root.fileView.copySelection()
    }
    Shortcut {
        sequence: root.key("cut", "Ctrl+X")
        onActivated: if (root.fileKeysActive) root.fileView.cutSelection()
    }
    Shortcut {
        sequence: root.key("paste", "Ctrl+V")
        onActivated: if (root.fileKeysActive) root.fileView.pasteHere()
    }
    Shortcut {
        sequence: root.key("trash", "Delete")
        onActivated: if (root.fileKeysActive) root.fileView.trashSelection()
    }
    Shortcut {
        sequence: root.key("delete", "Shift+Delete")
        onActivated: if (root.fileKeysActive) root.fileView.requestDeletePermanently()
    }

    // ---------------------------------------------------------- undo/redo
    Shortcut {
        sequence: root.key("undo", "Ctrl+Z")
        onActivated: if (!root.fileView || root.fileView.renameRow < 0) Application.undo.undo()
    }
    Shortcut {
        sequence: root.key("redo", "Ctrl+Shift+Z")
        onActivated: if (!root.fileView || root.fileView.renameRow < 0) Application.undo.redo()
    }
}
