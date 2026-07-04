import QtQuick
import QtQuick.Window
import Fang

Window {
    id: root
    visible: true
    width: 1100
    height: 700
    color: Application.theme.background
    title: win.activeTab ? win.activeTab.title + " — Fang" : "Fang"

    property WindowController win: Application.newWindow()
    readonly property PaneController pane: win.activeTab ? win.activeTab.activePane : null

    Connections {
        target: root.win
        function onCloseRequested() { root.close() }
    }

    // Interim stage-2 chrome; replaced by MainWindow.qml components in stage 3.
    Column {
        anchors.fill: parent

        Rectangle {
            width: parent.width
            height: Application.theme.toolbarHeight
            color: Application.theme.backgroundAlt

            Row {
                anchors.verticalCenter: parent.verticalCenter
                x: Application.theme.spacingMedium
                spacing: Application.theme.spacingMedium

                Text {
                    text: "↑ Up"
                    color: Application.theme.accent
                    font.pixelSize: 14
                    TapHandler { onTapped: root.pane.navigation.up() }
                }
                Text {
                    text: root.pane ? root.pane.location : ""
                    color: Application.theme.foreground
                    font.pixelSize: 14
                }
                Text {
                    visible: root.pane && root.pane.loading
                    text: "loading…"
                    color: Application.theme.foregroundMuted
                    font.pixelSize: 14
                }
            }
        }

        ListView {
            width: parent.width
            height: parent.height - Application.theme.toolbarHeight
            model: root.pane ? root.pane.model : null
            clip: true

            delegate: Rectangle {
                required property int index
                required property string name
                required property string iconName
                required property string sizeText
                required property bool isDir
                required property var modified

                width: ListView.view.width
                height: Application.theme.rowHeight
                color: "transparent"

                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    x: Application.theme.spacingMedium
                    spacing: Application.theme.spacingMedium

                    Image {
                        width: Application.theme.iconSizeSmall
                        height: Application.theme.iconSizeSmall
                        anchors.verticalCenter: parent.verticalCenter
                        source: "image://fang-icon/" + iconName
                        sourceSize: Qt.size(width, height)
                    }
                    Text {
                        width: 420
                        text: name
                        elide: Text.ElideRight
                        color: Application.theme.foreground
                        font.pixelSize: 13
                    }
                    Text {
                        width: 90
                        text: sizeText
                        color: Application.theme.foregroundMuted
                        font.pixelSize: 13
                    }
                    Text {
                        text: Qt.formatDateTime(modified, "yyyy-MM-dd hh:mm")
                        color: Application.theme.foregroundMuted
                        font.pixelSize: 13
                    }
                }

                TapHandler {
                    onTapped: root.pane.activate(index)
                }
            }
        }
    }
}
