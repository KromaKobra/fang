import QtQuick as QQ
import QtQuick
import Fang
import "controls" as C

// Collapsible right dock (docs/05): for a single selected file shows a large
// thumbnail (images), a monospace text peek (text/code), or a themed icon;
// always lists metadata underneath. All data comes from Application.previews.
Rectangle {
    id: root
    property PaneController pane: null

    readonly property var th: Application.theme
    readonly property var sel: pane ? pane.selection : null
    readonly property string currentPath:
        sel && sel.count === 1 && sel.current >= 0 ? pane.model.pathAt(sel.current) : ""

    property var details: ({})
    property string textPeek: ""

    color: th.backgroundAlt

    onCurrentPathChanged: refresh()
    Connections {
        // Metadata can change under us (rename, overwrite) while the path
        // stays selected; refresh with the listing.
        target: root.pane ? root.pane.model : null
        function onCountChanged() { root.refresh() }
    }

    function refresh() {
        if (currentPath === "" || !visible) {
            details = {}
            textPeek = ""
            return
        }
        details = Application.previews.fileDetails(currentPath)
        if (!details.isDir && !details.isImage
                && Application.previews.isProbablyText(currentPath))
            textPeek = Application.previews.readTextPeek(currentPath, 128)
        else
            textPeek = ""
    }
    onVisibleChanged: refresh()

    C.Separator {
        vertical: true
        anchors.left: parent.left
    }

    // Empty / multi selection placeholder
    Text {
        visible: root.currentPath === ""
        anchors.centerIn: parent
        width: parent.width - root.th.spacingLarge * 2
        text: root.sel && root.sel.count > 1
              ? root.sel.count + " items selected"
              : "Select a file to preview"
        color: root.th.foregroundMuted
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeNormal
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }

    QQ.Flickable {
        visible: root.currentPath !== ""
        anchors.fill: parent
        anchors.margins: root.th.spacingMedium
        contentHeight: previewCol.implicitHeight
        clip: true
        boundsBehavior: QQ.Flickable.StopAtBounds

        Column {
            id: previewCol
            width: parent.width
            spacing: root.th.spacingMedium

            // Visual: large thumbnail, text peek, or icon.
            Image {
                visible: root.details.isImage === true
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                height: visible ? Math.min(parent.width, 256) : 0
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                source: root.details.isImage === true
                        ? "image://fang-thumb" + root.currentPath : ""
                sourceSize: Qt.size(256, 256)
            }

            Rectangle {
                visible: root.textPeek !== ""
                width: parent.width
                height: visible ? Math.min(peekText.implicitHeight
                                           + root.th.spacingMedium * 2, 320) : 0
                radius: root.th.radiusSmall
                color: root.th.background
                border.color: root.th.border
                border.width: 1
                clip: true

                Text {
                    id: peekText
                    anchors.fill: parent
                    anchors.margins: root.th.spacingMedium
                    text: root.textPeek
                    color: root.th.foreground
                    font.family: root.th.fontFamilyMono
                    font.pointSize: root.th.fontSizeSmall
                    wrapMode: Text.WrapAnywhere
                }
            }

            Image {
                visible: root.details.isImage !== true && root.textPeek === ""
                anchors.horizontalCenter: parent.horizontalCenter
                width: visible ? root.th.iconSizeLarge * 1.5 : 0
                height: width
                source: root.details.iconName !== undefined
                        ? "image://fang-icon/" + root.details.iconName : ""
                sourceSize: Qt.size(width, height)
            }

            // Metadata
            Text {
                width: parent.width
                text: root.details.name || ""
                color: root.th.foreground
                font.family: root.th.fontFamily
                font.pointSize: root.th.fontSizeNormal
                font.bold: true
                elide: Text.ElideMiddle
            }

            Column {
                width: parent.width
                spacing: root.th.spacingSmall

                Repeater {
                    model: {
                        const d = root.details
                        if (!d.name)
                            return []
                        let rows = [
                            { k: "Type", v: d.typeText || d.mimeType || "" },
                        ]
                        if (!d.isDir)
                            rows.push({ k: "Size", v: d.sizeText || "" })
                        rows.push({ k: "Modified",
                                    v: Qt.formatDateTime(d.modified, "yyyy-MM-dd hh:mm") })
                        rows.push({ k: "Permissions", v: d.permissions || "" })
                        rows.push({ k: "Owner",
                                    v: (d.owner || "") + (d.group ? " · " + d.group : "") })
                        return rows
                    }
                    delegate: Row {
                        required property var modelData
                        spacing: root.th.spacingMedium
                        Text {
                            width: 78
                            text: modelData.k
                            color: root.th.foregroundMuted
                            font.family: root.th.fontFamily
                            font.pointSize: root.th.fontSizeSmall
                        }
                        Text {
                            width: previewCol.width - 78 - root.th.spacingMedium
                            text: modelData.v
                            color: root.th.foreground
                            font.family: root.th.fontFamily
                            font.pointSize: root.th.fontSizeSmall
                            elide: Text.ElideRight
                            wrapMode: Text.NoWrap
                        }
                    }
                }
            }
        }
    }
}
