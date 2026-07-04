import QtQuick
import Fang
import "controls" as C

// Hosts the four view modes and owns shared selection/keyboard/context-menu
// behavior for one pane (docs/05).
FocusScope {
    id: root
    property PaneController pane: null
    property bool paneActive: true
    // v1 default: single-click opens (docs/05 interaction rules); flipped by
    // behavior.open_with = "double".
    property bool singleClickOpens: true
    property int renameRow: -1
    readonly property var cutPaths: Application.clipboard.cutPaths
    signal activatedByUser()
    signal requestCreateFolder()
    signal requestCreateFile()
    /// Asks the window chrome to confirm a permanent delete of the selection.
    signal requestDeletePermanently()

    readonly property var th: Application.theme
    readonly property var currentView: viewLoader.item

    // ---------------------------------------------------------- interaction

    function clickRow(row, modifiers) {
        activatedByUser()
        const sel = pane.selection
        if (modifiers & Qt.ControlModifier) {
            sel.toggle(row)
        } else if (modifiers & Qt.ShiftModifier) {
            sel.range(sel.anchor >= 0 ? sel.anchor : 0, row)
        } else {
            sel.select(row)
            if (singleClickOpens)
                pane.activate(row)
        }
        root.forceActiveFocus()
    }

    function doubleClickRow(row) {
        if (!singleClickOpens)
            pane.activate(row)
    }

    function middleClickRow(row) {
        pane.openInNewTab(row)
    }

    function rightClickRow(row, sceneX, sceneY) {
        activatedByUser()
        if (!pane.selection.isSelected(row))
            pane.selection.select(row)
        contextMenu.popupFile(sceneX, sceneY)
        root.forceActiveFocus()
    }

    function backgroundClick() {
        activatedByUser()
        pane.selection.clear()
        root.forceActiveFocus()
    }

    function backgroundRightClick(sceneX, sceneY) {
        activatedByUser()
        contextMenu.popupBackground(sceneX, sceneY)
    }

    // ----------------------------------------------------- clipboard & ops

    function locationUrl() {
        return "file://" + pane.location
    }

    function copySelection() {
        if (pane.selection.hasSelection)
            Application.clipboard.copy(pane.selection.selectedUrls)
    }

    function cutSelection() {
        if (pane.selection.hasSelection)
            Application.clipboard.cut(pane.selection.selectedUrls)
    }

    function pasteHere() {
        if (Application.clipboard.hasFiles)
            Application.clipboard.paste(locationUrl())
    }

    function trashSelection() {
        if (pane.selection.hasSelection)
            pane.fileOperations.trash(pane.selection.selectedUrls)
    }

    /// Permanent delete without confirmation — the chrome confirms first
    /// (Shift+Del / context menu go through requestDeletePermanently()).
    function deleteSelectionNow() {
        if (pane.selection.hasSelection)
            pane.fileOperations.remove(pane.selection.selectedUrls)
    }

    // -------------------------------------------------------- drag & drop

    /// text/uri-list payload for a drag that started on `row`: the whole
    /// selection when that row is part of it, else just that row's file.
    function dragUriList(row, url) {
        const sel = pane.selection
        const urls = (row >= 0 && sel.isSelected(row)) ? sel.selectedUrls : [url]
        let lines = []
        for (let i = 0; i < urls.length; ++i)
            lines.push(String(urls[i]))
        return lines.join("\r\n")
    }

    /// Shared drop policy (delegates, background, sidebar forwards here too):
    /// Shift-drags arrive as MoveAction and force a move; internal drags
    /// default to auto (move on same device, copy across); external drops
    /// copy. DragEvent exposes no modifiers in QML, so Ctrl-forced copies of
    /// internal drags degrade to auto.
    function handleDrop(drop, destUrl) {
        const urls = drop.urls
        if (!urls || urls.length === 0)
            return
        const dest = String(destUrl)
        for (let i = 0; i < urls.length; ++i) {
            if (String(urls[i]) === dest)
                return // dropping something onto itself
        }
        const fo = pane.fileOperations
        if (drop.proposedAction === Qt.MoveAction)
            fo.move(urls, destUrl)
        else if (drop.source)
            fo.autoTransfer(urls, destUrl)
        else
            fo.copy(urls, destUrl)
        drop.acceptProposedAction()
    }

    /// True when every dragged url already lives directly in this pane's
    /// directory (a background drop would be a no-op move).
    function allInCurrentDir(urls) {
        const here = "file://" + pane.location
        for (let i = 0; i < urls.length; ++i) {
            const s = String(urls[i])
            const slash = s.lastIndexOf("/")
            if (slash < 0 || s.substring(0, slash) !== here)
                return false
        }
        return true
    }

    // ------------------------------------------------------------- renaming

    function startRename(row) {
        if (row >= 0)
            renameRow = row
    }
    function startRenameCurrent() {
        startRename(pane.selection.current)
    }
    function finishRename(row, newName) {
        renameRow = -1
        pane.renameAt(row, newName)
        root.forceActiveFocus()
    }
    function cancelRename() {
        renameRow = -1
        root.forceActiveFocus()
    }

    // ------------------------------------------------------------- keyboard

    function moveCurrent(delta, extendSelection) {
        const count = pane.itemCount
        if (count === 0)
            return
        const sel = pane.selection
        let next = sel.current < 0 ? 0 : sel.current + delta
        next = Math.min(count - 1, Math.max(0, next))
        if (extendSelection && sel.anchor >= 0)
            sel.range(sel.anchor, next)
        else
            sel.select(next)
        if (currentView && currentView.ensureVisible)
            currentView.ensureVisible(next)
    }

    Keys.onPressed: function(ev) {
        if (renameRow >= 0)
            return
        const sel = pane.selection
        const cols = currentView && currentView.columnsPerRow ? currentView.columnsPerRow : 1
        const shift = ev.modifiers & Qt.ShiftModifier
        switch (ev.key) {
        case Qt.Key_Down:
            moveCurrent(cols === 1 ? 1 : cols, shift); ev.accepted = true; break
        case Qt.Key_Up:
            moveCurrent(cols === 1 ? -1 : -cols, shift); ev.accepted = true; break
        case Qt.Key_Right:
            if (cols > 1) { moveCurrent(1, shift); ev.accepted = true }
            break
        case Qt.Key_Left:
            if (cols > 1) { moveCurrent(-1, shift); ev.accepted = true }
            break
        case Qt.Key_Home:
            moveCurrent(-pane.itemCount, shift); ev.accepted = true; break
        case Qt.Key_End:
            moveCurrent(pane.itemCount, shift); ev.accepted = true; break
        case Qt.Key_PageDown:
            moveCurrent(cols * 8, shift); ev.accepted = true; break
        case Qt.Key_PageUp:
            moveCurrent(-cols * 8, shift); ev.accepted = true; break
        case Qt.Key_Return:
        case Qt.Key_Enter:
            if (sel.current >= 0)
                pane.activate(sel.current)
            ev.accepted = true
            break
        case Qt.Key_Backspace:
            pane.navigation.up(); ev.accepted = true; break
        case Qt.Key_Escape:
            pane.filterText = ""; sel.clear(); ev.accepted = true; break
        default:
            // Type-to-filter: printable characters feed the live filter.
            if (ev.text.length === 1 && ev.text >= " " && !(ev.modifiers
                    & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))) {
                pane.filterText = pane.filterText + ev.text
                ev.accepted = true
            }
        }
    }

    // ---------------------------------------------------------------- views

    // Background drop target (→ this pane's directory). Delegate DropAreas
    // sit above and take precedence for drops onto folders.
    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: function(drop) {
            if (drop.source && root.allInCurrentDir(drop.urls))
                return // internal reshuffle into the same dir: no-op
            root.handleDrop(drop, root.locationUrl())
        }
    }

    Loader {
        id: viewLoader
        anchors.fill: parent
        sourceComponent: {
            switch (root.pane ? root.pane.viewMode : Fang.ViewMode.Details) {
            case Fang.ViewMode.Icons: return iconsComp
            case Fang.ViewMode.List: return listComp
            case Fang.ViewMode.Tree: return treeComp
            default: return detailsComp
            }
        }
    }

    Component { id: iconsComp; IconsView { fileView: root; pane: root.pane } }
    Component { id: listComp; ListView { fileView: root; pane: root.pane } }
    Component { id: detailsComp; DetailsView { fileView: root; pane: root.pane } }
    Component { id: treeComp; TreeView { fileView: root; pane: root.pane } }

    ContextMenu {
        id: contextMenu
        pane: root.pane
        fileView: root
    }

    // Loading indicator (top-right corner, never blocks the view)
    C.Spinner {
        visible: root.pane && root.pane.loading
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: root.th.spacingMedium
    }
}
