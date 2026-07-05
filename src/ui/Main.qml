import QtQuick
import Fang
import "controls" as C

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
    property bool closeConfirmed: false

    // Mirror geometry into the controller so the session can restore it.
    Component.onCompleted: {
        const g = win.geometry
        if (g.width > 0 && g.height > 0) {
            root.x = g.x
            root.y = g.y
            root.width = g.width
            root.height = g.height
        }
    }
    function publishGeometry() {
        win.geometry = Qt.rect(root.x, root.y, root.width, root.height)
    }
    onXChanged: publishGeometry()
    onYChanged: publishGeometry()
    onWidthChanged: publishGeometry()
    onHeightChanged: publishGeometry()

    // behavior.close_confirm_multiple_tabs (docs/06): confirm before closing
    // a window that still has several tabs open.
    onClosing: function(close) {
        if (!root.closeConfirmed && root.win.tabCount > 1
                && Application.settings.closeConfirmMultipleTabs) {
            close.accepted = false
            closeConfirm.open()
        }
    }

    Connections {
        target: root.win
        function onCloseRequested() {
            root.closeConfirmed = true
            root.close()
        }
    }

    MainWindow {
        anchors.fill: parent
        win: root.win
    }

    NotificationOverlay {
        anchors.fill: parent
        z: 100
    }

    C.Popup {
        id: closeConfirm
        modal: true
        anchors.centerIn: parent
        contentWidth: 360
        contentHeight: closeCol.implicitHeight

        Column {
            id: closeCol
            width: parent.width
            spacing: Application.theme.spacingMedium

            Text {
                text: "Close window?"
                color: Application.theme.foreground
                font.family: Application.theme.fontFamily
                font.pointSize: Application.theme.fontSizeLarge
                font.bold: true
            }
            Text {
                width: parent.width
                text: root.win.tabCount + " tabs are open."
                color: Application.theme.foregroundMuted
                font.family: Application.theme.fontFamily
                font.pointSize: Application.theme.fontSizeNormal
                wrapMode: Text.WordWrap
            }
            Row {
                anchors.right: parent.right
                spacing: Application.theme.spacingSmall
                C.Button {
                    text: "Cancel"
                    onClicked: closeConfirm.close()
                }
                C.Button {
                    text: "Close Window"
                    accent: true
                    onClicked: {
                        closeConfirm.close()
                        root.closeConfirmed = true
                        root.close()
                    }
                }
            }
        }
    }
}
