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
    property var cutPaths: []
    signal activatedByUser()
    signal requestCreateFolder()
    signal requestCreateFile()

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
