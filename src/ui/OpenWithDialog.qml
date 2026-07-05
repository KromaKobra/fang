import QtQuick as QQ
import QtQuick
import Fang
import "controls" as C

// "Open With" application picker (docs/05): lists MimeResolver.appsFor(mime)
// with an "always use" toggle that persists the default via setDefaultApp.
C.Popup {
    id: root
    property var urls: []
    property string mime: ""
    property string fileLabel: ""
    property var apps: []
    property int selectedIndex: -1

    modal: true
    anchors.centerIn: parent
    contentWidth: 360
    contentHeight: col.implicitHeight

    function openFor(pane) {
        const sel = pane.selection
        if (!sel.hasSelection || sel.current < 0)
            return
        const path = pane.model.pathAt(sel.current)
        urls = sel.selectedUrls
        mime = MimeResolver.mimeTypeFor(path)
        fileLabel = sel.count === 1
            ? path.substring(path.lastIndexOf("/") + 1)
            : sel.count + " files"
        apps = MimeResolver.appsFor(mime)
        selectedIndex = -1
        for (let i = 0; i < apps.length; ++i) {
            if (apps[i].isDefault) {
                selectedIndex = i
                break
            }
        }
        alwaysUse.checked = false
        open()
    }

    function launch() {
        if (selectedIndex < 0 || selectedIndex >= apps.length)
            return
        const app = apps[selectedIndex]
        if (alwaysUse.checked)
            MimeResolver.setDefaultApp(root.mime, app.id)
        MimeResolver.openWith(root.urls, app.id)
        root.close()
    }

    Column {
        id: col
        width: parent.width
        spacing: root.th.spacingMedium

        Text {
            text: "Open " + root.fileLabel + " with"
            width: parent.width
            color: root.th.foreground
            font.family: root.th.fontFamily
            font.pointSize: root.th.fontSizeNormal
            font.bold: true
            elide: Text.ElideMiddle
        }
        Text {
            text: root.mime
            color: root.th.foregroundMuted
            font.family: root.th.fontFamilyMono
            font.pointSize: root.th.fontSizeSmall
        }

        Rectangle {
            width: parent.width
            height: 240
            radius: root.th.radiusSmall
            color: root.th.background
            border.color: root.th.border
            border.width: 1
            clip: true

            QQ.ListView {
                id: appList
                anchors.fill: parent
                anchors.margins: root.th.spacingSmall
                model: root.apps
                clip: true
                boundsBehavior: QQ.Flickable.StopAtBounds

                delegate: Rectangle {
                    id: appRow
                    required property var modelData
                    required property int index
                    width: appList.width
                    height: root.th.rowHeight + root.th.spacingSmall
                    radius: root.th.radiusSmall
                    color: index === root.selectedIndex
                           ? root.th.selectionBackground
                           : rowMouse.containsMouse ? root.th.surfaceHover : "transparent"

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        x: root.th.spacingMedium
                        spacing: root.th.spacingMedium
                        Image {
                            width: root.th.iconSizeMedium
                            height: root.th.iconSizeMedium
                            anchors.verticalCenter: parent.verticalCenter
                            source: "image://fang-icon/" + (appRow.modelData.iconName
                                    || "application-x-executable")
                            sourceSize: Qt.size(width, height)
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: appRow.modelData.name
                                  + (appRow.modelData.isDefault ? "  (default)" : "")
                            color: appRow.index === root.selectedIndex
                                   ? root.th.selectionForeground : root.th.foreground
                            font.family: root.th.fontFamily
                            font.pointSize: root.th.fontSizeNormal
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.selectedIndex = appRow.index
                        onDoubleClicked: {
                            root.selectedIndex = appRow.index
                            root.launch()
                        }
                    }
                }

                Text {
                    visible: root.apps.length === 0
                    anchors.centerIn: parent
                    text: "No applications found for this type"
                    color: root.th.foregroundMuted
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeNormal
                }
            }

            C.ScrollBar {
                target: appList
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }
        }

        C.Checkbox {
            id: alwaysUse
            text: "Always open this type with the selected application"
        }

        Row {
            anchors.right: parent.right
            spacing: root.th.spacingSmall
            C.Button {
                text: "Cancel"
                onClicked: root.close()
            }
            C.Button {
                text: "Open"
                accent: true
                enabled: root.selectedIndex >= 0
                onClicked: root.launch()
            }
        }
    }
}
