import QtQuick
import Fang
import "controls" as C

// Window chrome: TabBar + Toolbar + (Sidebar | panes) + StatusBar
// (docs/05). Tab pages live in a Repeater and are hidden rather than
// destroyed so each tab keeps its view state.
Rectangle {
    id: root
    property WindowController win: null

    readonly property var th: Application.theme
    readonly property TabController tab: win ? win.activeTab : null
    readonly property PaneController pane: tab ? tab.activePane : null
    /// The FileView of the active pane in the active tab (rename shortcut,
    /// future preview wiring). Published by the visible tab page.
    property var currentFileView: null

    property real sidebarW: th.sidebarWidth
    // view.preview_pane / preview_pane_width give the startup state (docs/06);
    // F11 and the splitter take over from there.
    property bool previewVisible: Application.settings.previewPane
    property real previewW: Application.settings.previewPaneWidth
    property bool searchVisible: false

    function toggleSearch() {
        searchVisible = !searchVisible
        if (searchVisible)
            searchPanel.openPanel()
        else
            Application.search.cancel()
    }

    color: th.background

    function focusLocationBar() {
        toolbar.focusLocationBar()
    }

    TabBar {
        id: tabBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        win: root.win
    }

    Toolbar {
        id: toolbar
        anchors.top: tabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        pane: root.pane
        win: root.win
        onSearchToggled: root.toggleSearch()
    }

    SearchPanel {
        id: searchPanel
        visible: root.searchVisible
        height: visible ? implicitHeight : 0
        anchors.top: toolbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        pane: root.pane
        onClosed: root.toggleSearch()
    }

    StatusBar {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        pane: root.pane
    }

    Item {
        id: body
        anchors.top: searchPanel.bottom
        anchors.bottom: statusBar.top
        anchors.left: parent.left
        anchors.right: parent.right

        Sidebar {
            id: sidebar
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: root.sidebarW
            pane: root.pane
            win: root.win
        }

        C.Splitter {
            id: sidebarSplitter
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: sidebar.right
            onDragged: function(delta) {
                root.sidebarW = Math.min(420, Math.max(120, root.sidebarW + delta))
            }
        }

        PreviewPane {
            id: previewPane
            visible: root.previewVisible
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: visible ? root.previewW : 0
            pane: root.pane
        }

        C.Splitter {
            id: previewSplitter
            visible: root.previewVisible
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: previewPane.left
            onDragged: function(delta) {
                root.previewW = Math.min(480, Math.max(180, root.previewW - delta))
            }
        }

        Item {
            id: paneArea
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: sidebarSplitter.right
            anchors.right: root.previewVisible ? previewSplitter.left : parent.right

            Repeater {
                model: root.win ? root.win.tabCount : 0
                delegate: Item {
                    id: tabPage
                    required property int index
                    readonly property TabController tabCtl: root.win.tabAt(index)
                    readonly property bool split: tabCtl ? tabCtl.splitView : false
                    property real splitRatio: 0.5
                    readonly property var activeFileView:
                        split && tabCtl.activePane === tabCtl.rightPane
                            ? rightLoader.item : leftView

                    anchors.fill: parent
                    visible: index === root.win.activeTabIndex

                    function publish() {
                        if (visible && tabCtl) {
                            root.currentFileView = activeFileView
                            if (activeFileView)
                                activeFileView.forceActiveFocus()
                        }
                    }
                    onVisibleChanged: publish()
                    onActiveFileViewChanged: publish()
                    Component.onCompleted: publish()

                    FileView {
                        id: leftView
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        width: tabPage.split
                               ? (parent.width - paneSplitter.width) * tabPage.splitRatio
                               : parent.width
                        pane: tabPage.tabCtl ? tabPage.tabCtl.leftPane : null
                        paneActive: !tabPage.split
                                    || tabPage.tabCtl.activePane === tabPage.tabCtl.leftPane
                        onActivatedByUser: tabPage.tabCtl.setActivePane(pane)
                        onRequestCreateFolder: createDialog.openFor("folder")
                        onRequestCreateFile: createDialog.openFor("file")
                        onRequestDeletePermanently: deleteConfirm.openFor(leftView)
                        onRequestTrash: deleteConfirm.openForTrash(leftView)
                        onRequestProperties: root.openProperties(leftView)
                        onRequestOpenWith: openWithDialog.openFor(leftView.pane)

                        // Active-pane accent border (only meaningful in split).
                        Rectangle {
                            anchors.fill: parent
                            visible: tabPage.split && leftView.paneActive
                            color: "transparent"
                            border.color: root.th.accent
                            border.width: 1
                        }
                    }

                    C.Splitter {
                        id: paneSplitter
                        visible: tabPage.split
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: leftView.right
                        onDragged: function(delta) {
                            tabPage.splitRatio = Math.min(0.85, Math.max(0.15,
                                tabPage.splitRatio + delta / Math.max(1, paneArea.width)))
                        }
                    }

                    Loader {
                        id: rightLoader
                        active: tabPage.split
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: paneSplitter.right
                        anchors.right: parent.right

                        sourceComponent: FileView {
                            id: rightView
                            pane: tabPage.tabCtl ? tabPage.tabCtl.rightPane : null
                            paneActive: tabPage.tabCtl
                                && tabPage.tabCtl.activePane === tabPage.tabCtl.rightPane
                            onActivatedByUser: tabPage.tabCtl.setActivePane(pane)
                            onRequestCreateFolder: createDialog.openFor("folder")
                            onRequestCreateFile: createDialog.openFor("file")
                            onRequestDeletePermanently: deleteConfirm.openFor(rightView)
                            onRequestTrash: deleteConfirm.openForTrash(rightView)
                            onRequestProperties: root.openProperties(rightView)
                            onRequestOpenWith: openWithDialog.openFor(rightView.pane)

                            Rectangle {
                                anchors.fill: parent
                                visible: parent.paneActive
                                color: "transparent"
                                border.color: root.th.accent
                                border.width: 1
                            }
                        }
                    }
                }
            }
        }
    }

    /// Properties for the selection's current item, else the directory.
    function openProperties(fv) {
        const p = fv ? fv.pane : root.pane
        if (!p)
            return
        const path = p.selection.hasSelection && p.selection.current >= 0
                   ? p.model.pathAt(p.selection.current) : p.location
        propertiesDialog.openFor(path)
    }

    PropertiesDialog {
        id: propertiesDialog
        parent: root
    }

    OpenWithDialog {
        id: openWithDialog
        parent: root
    }

    CreateDialog {
        id: createDialog
        parent: root
        onAccepted: function(mode, name) {
            if (!root.pane)
                return
            if (mode === "folder")
                root.pane.createFolder(name)
            else
                root.pane.createFile(name)
        }
    }

    // -------------------------------------------- file operation overlays

    // The engine forwards every conflict through each pane's FileOperations;
    // one Connections on the active pane's controller sees them all.
    Connections {
        target: root.pane ? root.pane.fileOperations : null
        function onConflictRequested(op, sourceUrl, destUrl, info) {
            conflictDialog.enqueue(op, sourceUrl, destUrl, info)
        }
    }

    ConflictDialog {
        id: conflictDialog
        parent: root
    }

    OperationOverlay {
        fileOps: root.pane ? root.pane.fileOperations : null
        anchors.right: parent.right
        anchors.bottom: statusBar.top
        anchors.margins: th.spacingMedium
        z: 50
    }

    // Delete/trash confirmation (Shift+Del, Del with confirm_trash, menu).
    C.Popup {
        id: deleteConfirm
        property var fileView: null
        property bool trashMode: false
        modal: true
        anchors.centerIn: parent
        contentWidth: 360
        contentHeight: confirmCol.implicitHeight

        function openFor(fv) {
            if (!fv || !fv.pane || !fv.pane.selection.hasSelection)
                return
            // general.confirm_delete = false skips straight to the delete.
            if (!Application.settings.confirmDelete) {
                fv.deleteSelectionNow()
                return
            }
            fileView = fv
            trashMode = false
            open()
        }

        function openForTrash(fv) {
            if (!fv || !fv.pane || !fv.pane.selection.hasSelection)
                return
            fileView = fv
            trashMode = true
            open()
        }

        Column {
            id: confirmCol
            width: parent.width
            spacing: root.th.spacingMedium

            Text {
                text: deleteConfirm.trashMode ? "Move to trash?" : "Delete permanently?"
                color: root.th.foreground
                font.family: root.th.fontFamily
                font.pointSize: root.th.fontSizeLarge
                font.bold: true
            }
            Text {
                width: parent.width
                text: {
                    const n = deleteConfirm.fileView
                            ? deleteConfirm.fileView.pane.selection.count : 0
                    const items = n === 1 ? "1 item" : n + " items"
                    return deleteConfirm.trashMode
                        ? items + " will be moved to the trash."
                        : items + " will be deleted permanently. This cannot be undone."
                }
                color: root.th.foregroundMuted
                font.family: root.th.fontFamily
                font.pointSize: root.th.fontSizeNormal
                wrapMode: Text.WordWrap
            }
            Row {
                anchors.right: parent.right
                spacing: root.th.spacingSmall
                C.Button {
                    text: "Cancel"
                    onClicked: deleteConfirm.close()
                }
                C.Button {
                    text: deleteConfirm.trashMode ? "Move to Trash" : "Delete"
                    accent: true
                    onClicked: {
                        if (deleteConfirm.trashMode)
                            deleteConfirm.fileView.trashSelectionNow()
                        else
                            deleteConfirm.fileView.deleteSelectionNow()
                        deleteConfirm.close()
                    }
                }
            }
        }
    }

    Shortcuts {
        win: root.win
        pane: root.pane
        fileView: root.currentFileView
        onLocationBarRequested: root.focusLocationBar()
        onSearchRequested: root.toggleSearch()
        onCreateFolderRequested: createDialog.openFor("folder")
        onCreateFileRequested: createDialog.openFor("file")
        onPreviewToggleRequested: root.previewVisible = !root.previewVisible
        onPropertiesRequested: root.openProperties(root.currentFileView)
    }
}
