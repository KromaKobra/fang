import QtQuick
import Fang

// Root of the bundled UI (docs/05): one window, one WindowController,
// window chrome in MainWindow, toasts on top.
Window {
    id: root
    visible: true
    width: 1100
    height: 700
    color: Application.theme.background
    title: win.activeTab ? win.activeTab.title + " — Fang" : "Fang"

    property WindowController win: Application.newWindow()

    Connections {
        target: root.win
        function onCloseRequested() { root.close() }
    }

    MainWindow {
        anchors.fill: parent
        win: root.win
    }

    NotificationOverlay {
        anchors.fill: parent
        z: 100
    }
}
