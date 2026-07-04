import QtQuick
import Fang
import "controls" as C

// Navigation buttons, location bar, search toggle, view switch, menu
// (docs/05).
Rectangle {
    id: root
    property PaneController pane: null
    property WindowController win: null
    signal searchToggled()
    signal menuRequested()

    readonly property var th: Application.theme
    readonly property var nav: pane ? pane.navigation : null

    implicitHeight: th.toolbarHeight
    color: th.backgroundAlt

    function focusLocationBar() {
        locationBar.startEditing()
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: root.th.spacingMedium
        anchors.rightMargin: root.th.spacingMedium
        spacing: root.th.spacingSmall

        C.IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "go-previous"
            tooltip: "Back (Alt+Left)"
            enabled: root.nav ? root.nav.canGoBack : false
            onClicked: root.nav.back()
        }
        C.IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "go-next"
            tooltip: "Forward (Alt+Right)"
            enabled: root.nav ? root.nav.canGoForward : false
            onClicked: root.nav.forward()
        }
        C.IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "go-up"
            tooltip: "Up (Alt+Up)"
            enabled: root.nav ? root.nav.canGoUp : false
            onClicked: root.nav.up()
        }
        C.IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "view-refresh"
            tooltip: "Reload (F5)"
            onClicked: root.nav.refresh()
        }
        C.IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "go-home"
            tooltip: "Home"
            onClicked: root.nav.home()
        }

        LocationBar {
            id: locationBar
            anchors.verticalCenter: parent.verticalCenter
            pane: root.pane
            width: parent.width
                - (parent.children.length - 2) * 0 // static layout below
                - backForwardWidth() - trailingWidth() - root.th.spacingSmall * 10
            function backForwardWidth() { return 5 * (root.th.iconSizeSmall + root.th.spacingMedium * 2) }
            function trailingWidth() {
                return viewSwitch.width + searchBtn.width + menuBtn.width
                    + root.th.spacingSmall * 3
            }
        }

        C.IconButton {
            id: searchBtn
            anchors.verticalCenter: parent.verticalCenter
            icon: "system-search"
            tooltip: "Search (Ctrl+F)"
            onClicked: root.searchToggled()
        }

        ViewModeSwitch {
            id: viewSwitch
            anchors.verticalCenter: parent.verticalCenter
            pane: root.pane
        }

        C.IconButton {
            id: menuBtn
            anchors.verticalCenter: parent.verticalCenter
            icon: "open-menu-symbolic"
            tooltip: "Menu"
            onClicked: mainMenu.popupBelow(menuBtn)
        }
    }

    C.Menu {
        id: mainMenu
        C.MenuItem {
            text: "New Tab"
            shortcut: "Ctrl+T"
            icon: "tab-new"
            onTriggered: root.win.addTab(root.pane.location)
        }
        C.MenuItem {
            text: "New Window"
            shortcut: "Ctrl+N"
            icon: "window-new"
            onTriggered: Application.openPath(root.pane.location)
        }
        C.Separator {}
        C.MenuItem {
            text: "Toggle Split View"
            shortcut: "F3"
            icon: "view-split-left-right"
            showChecked: true
            checked: root.win ? root.win.splitView : false
            onTriggered: root.win.toggleSplit()
        }
        C.MenuItem {
            text: "Show Hidden Files"
            shortcut: "Ctrl+H"
            showChecked: true
            checked: root.pane ? root.pane.showHidden : false
            onTriggered: root.pane.showHidden = !root.pane.showHidden
        }
        C.MenuItem {
            text: "Folders First"
            showChecked: true
            checked: root.pane ? root.pane.foldersFirst : true
            onTriggered: root.pane.foldersFirst = !root.pane.foldersFirst
        }
        C.Separator {}
        C.MenuItem {
            text: "Quit"
            shortcut: "Ctrl+Q"
            icon: "application-exit"
            onTriggered: Application.quit()
        }
    }
}
