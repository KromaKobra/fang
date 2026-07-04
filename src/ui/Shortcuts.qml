import QtQuick
import Fang

// Central shortcut map (docs/05). One place so the config/override can
// remap them; bound from Settings in stage 7. Search joins with stage 6,
// preview with stage 5.
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

    readonly property var nav: pane ? pane.navigation : null
    readonly property var sel: pane ? pane.selection : null
    /// File shortcuts only fire while the file view has focus and no inline
    /// rename is running (otherwise Del/Ctrl+C belong to the text editor).
    readonly property bool fileKeysActive:
        fileView && fileView.activeFocus && fileView.renameRow < 0

    function zoomBy(delta) {
        if (pane)
            pane.iconSize = Math.min(256, Math.max(16, pane.iconSize + delta))
    }

    // ------------------------------------------------------ tabs & window
    Shortcut {
        sequence: "Ctrl+T"
        onActivated: root.win.addTab(root.pane ? root.pane.location : "")
    }
    Shortcut { sequence: "Ctrl+W"; onActivated: root.win.closeTab(root.win.activeTabIndex) }
    Shortcut {
        sequence: "Ctrl+N"
        onActivated: Application.openPath(root.pane ? root.pane.location : "")
    }
    Shortcut {
        sequence: "Ctrl+Tab"
        onActivated: root.win.activeTabIndex =
            (root.win.activeTabIndex + 1) % root.win.tabCount
    }
    Shortcut {
        sequence: "Ctrl+Shift+Tab"
        onActivated: root.win.activeTabIndex =
            (root.win.activeTabIndex + root.win.tabCount - 1) % root.win.tabCount
    }
    Shortcut { sequence: "Ctrl+Q"; onActivated: Application.quit() }

    // ----------------------------------------------------------- navigate
    Shortcut { sequence: "Alt+Left"; onActivated: if (root.nav) root.nav.back() }
    Shortcut { sequence: "Alt+Right"; onActivated: if (root.nav) root.nav.forward() }
    Shortcut { sequence: "Alt+Up"; onActivated: if (root.nav) root.nav.up() }
    Shortcut { sequence: "F5"; onActivated: if (root.nav) root.nav.refresh() }
    Shortcut { sequence: "Ctrl+L"; onActivated: root.locationBarRequested() }
    Shortcut { sequence: "Ctrl+F"; onActivated: root.searchRequested() }

    // --------------------------------------------------------------- view
    Shortcut {
        sequence: "Ctrl+H"
        onActivated: if (root.pane) root.pane.showHidden = !root.pane.showHidden
    }
    Shortcut { sequence: "F3"; onActivated: root.win.toggleSplit() }
    Shortcut {
        sequence: "Ctrl+1"
        onActivated: if (root.pane) root.pane.viewMode = Fang.ViewMode.Icons
    }
    Shortcut {
        sequence: "Ctrl+2"
        onActivated: if (root.pane) root.pane.viewMode = Fang.ViewMode.List
    }
    Shortcut {
        sequence: "Ctrl+3"
        onActivated: if (root.pane) root.pane.viewMode = Fang.ViewMode.Details
    }
    Shortcut {
        sequence: "Ctrl+4"
        onActivated: if (root.pane) root.pane.viewMode = Fang.ViewMode.Tree
    }
    Shortcut { sequence: "Ctrl++"; onActivated: root.zoomBy(8) }
    Shortcut { sequence: "Ctrl+="; onActivated: root.zoomBy(8) }
    Shortcut { sequence: "Ctrl+-"; onActivated: root.zoomBy(-8) }
    Shortcut { sequence: "Ctrl+0"; onActivated: if (root.pane) root.pane.iconSize = 48 }

    // ---------------------------------------------------------- selection
    Shortcut { sequence: "Ctrl+A"; onActivated: if (root.sel) root.sel.selectAll() }
    Shortcut { sequence: "Ctrl+Shift+A"; onActivated: if (root.sel) root.sel.invert() }

    // ------------------------------------------------------------- files
    Shortcut {
        sequence: "F2"
        onActivated: if (root.fileView) root.fileView.startRenameCurrent()
    }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: root.createFolderRequested() }

    // -------------------------------------------------- clipboard & trash
    Shortcut {
        sequence: "Ctrl+C"
        onActivated: if (root.fileKeysActive) root.fileView.copySelection()
    }
    Shortcut {
        sequence: "Ctrl+X"
        onActivated: if (root.fileKeysActive) root.fileView.cutSelection()
    }
    Shortcut {
        sequence: "Ctrl+V"
        onActivated: if (root.fileKeysActive) root.fileView.pasteHere()
    }
    Shortcut {
        sequence: "Delete"
        onActivated: if (root.fileKeysActive) root.fileView.trashSelection()
    }
    Shortcut {
        sequence: "Shift+Delete"
        onActivated: if (root.fileKeysActive) root.fileView.requestDeletePermanently()
    }

    // ---------------------------------------------------------- undo/redo
    Shortcut {
        sequence: "Ctrl+Z"
        onActivated: if (!root.fileView || root.fileView.renameRow < 0) Application.undo.undo()
    }
    Shortcut {
        sequence: "Ctrl+Shift+Z"
        onActivated: if (!root.fileView || root.fileView.renameRow < 0) Application.undo.redo()
    }
}
