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
        onSearchToggled: {} // search panel arrives in stage 6
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
        anchors.top: toolbar.bottom
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

        Item {
            id: paneArea
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: sidebarSplitter.right
            anchors.right: parent.right

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
                            pane: tabPage.tabCtl ? tabPage.tabCtl.rightPane : null
                            paneActive: tabPage.tabCtl
                                && tabPage.tabCtl.activePane === tabPage.tabCtl.rightPane
                            onActivatedByUser: tabPage.tabCtl.setActivePane(pane)
                            onRequestCreateFolder: createDialog.openFor("folder")
                            onRequestCreateFile: createDialog.openFor("file")

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

    Shortcuts {
        win: root.win
        pane: root.pane
        fileView: root.currentFileView
        onLocationBarRequested: root.focusLocationBar()
        onSearchRequested: {} // stage 6
        onCreateFolderRequested: createDialog.openFor("folder")
        onCreateFileRequested: createDialog.openFor("file")
    }
}
