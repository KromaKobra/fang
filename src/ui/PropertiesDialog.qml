import QtQuick as QQ
import QtQuick
import Fang
import "controls" as C

// File/folder properties (docs/05): General (metadata + async recursive dir
// size), Permissions (read-only rwx grid) and the default-app selector.
C.Popup {
    id: root
    property string path: ""
    property var details: ({})
    property string tab: "general" // general | permissions | openwith
    property string dirSizeText: ""
    property var apps: []
    property string mime: ""

    modal: true
    anchors.centerIn: parent
    contentWidth: 380
    contentHeight: col.implicitHeight

    function openFor(targetPath) {
        if (!targetPath)
            return
        path = targetPath
        details = Application.previews.fileDetails(path)
        tab = "general"
        dirSizeText = ""
        mime = details.mimeType || ""
        apps = details.isDir ? [] : MimeResolver.appsFor(mime)
        if (details.isDir) {
            dirSizeText = "Calculating…"
            Application.previews.computeDirSize(path)
        }
        open()
    }

    Connections {
        target: Application.previews
        function onDirSizeComputed(dirPath, bytes, files) {
            if (dirPath === root.path)
                root.dirSizeText = Application.formatSize(bytes) + " in " + files
                    + (files === 1 ? " file" : " files")
        }
    }

    // rwx helper: position 0..8 into the "rwxr-xr-x" string.
    function perm(pos) {
        const p = details.permissions || ""
        return pos < p.length && p[pos] !== "-"
    }

    Column {
        id: col
        width: parent.width
        spacing: root.th.spacingMedium

        // Header: icon + name
        Row {
            spacing: root.th.spacingMedium
            width: parent.width
            Image {
                width: root.th.iconSizeLarge
                height: root.th.iconSizeLarge
                source: root.details.iconName !== undefined
                        ? "image://fang-icon/" + root.details.iconName : ""
                sourceSize: Qt.size(width, height)
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - root.th.iconSizeLarge - root.th.spacingMedium
                Text {
                    width: parent.width
                    text: root.details.name || ""
                    color: root.th.foreground
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeLarge
                    font.bold: true
                    elide: Text.ElideMiddle
                }
                Text {
                    text: root.details.typeText || root.mime
                    color: root.th.foregroundMuted
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeSmall
                }
            }
        }

        // Tab strip
        Row {
            spacing: root.th.spacingSmall
            C.Button {
                text: "General"
                accent: root.tab === "general"
                onClicked: root.tab = "general"
            }
            C.Button {
                text: "Permissions"
                accent: root.tab === "permissions"
                onClicked: root.tab = "permissions"
            }
            C.Button {
                visible: root.apps.length > 0
                text: "Open With"
                accent: root.tab === "openwith"
                onClicked: root.tab = "openwith"
            }
        }

        C.Separator { width: parent.width }

        // ---- General
        Column {
            visible: root.tab === "general"
            width: parent.width
            spacing: root.th.spacingSmall

            Repeater {
                model: {
                    const d = root.details
                    if (!d.name)
                        return []
                    const dir = root.path.substring(0, root.path.lastIndexOf("/")) || "/"
                    let rows = [
                        { k: "Name", v: d.name },
                        { k: "Type", v: (d.typeText || "") + (root.mime ? "  (" + root.mime + ")" : "") },
                        { k: "Location", v: dir },
                        { k: "Size", v: d.isDir ? root.dirSizeText : (d.sizeText || "") },
                        { k: "Modified", v: Qt.formatDateTime(d.modified, "yyyy-MM-dd hh:mm:ss") },
                        { k: "Created", v: Qt.formatDateTime(d.created, "yyyy-MM-dd hh:mm:ss") },
                        { k: "Accessed", v: Qt.formatDateTime(d.accessed, "yyyy-MM-dd hh:mm:ss") },
                    ]
                    if (d.isSymlink)
                        rows.splice(1, 0, { k: "", v: "Symbolic link" })
                    return rows
                }
                delegate: Row {
                    required property var modelData
                    spacing: root.th.spacingMedium
                    Text {
                        width: 84
                        text: modelData.k
                        color: root.th.foregroundMuted
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeNormal
                    }
                    Text {
                        width: col.width - 84 - root.th.spacingMedium
                        text: modelData.v
                        color: root.th.foreground
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeNormal
                        elide: Text.ElideMiddle
                    }
                }
            }
        }

        // ---- Permissions (read-only in v1)
        Column {
            visible: root.tab === "permissions"
            width: parent.width
            spacing: root.th.spacingSmall

            Row {
                spacing: root.th.spacingMedium
                Text {
                    width: 84
                    text: ""
                    font.pointSize: root.th.fontSizeNormal
                }
                Repeater {
                    model: ["Read", "Write", "Execute"]
                    delegate: Text {
                        required property var modelData
                        width: 64
                        text: modelData
                        color: root.th.foregroundMuted
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeSmall
                    }
                }
            }
            Repeater {
                model: [
                    { label: "Owner" + (root.details.owner ? " (" + root.details.owner + ")" : ""), base: 0 },
                    { label: "Group" + (root.details.group ? " (" + root.details.group + ")" : ""), base: 3 },
                    { label: "Others", base: 6 },
                ]
                delegate: Row {
                    required property var modelData
                    spacing: root.th.spacingMedium
                    Text {
                        width: 84
                        text: modelData.label
                        color: root.th.foreground
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeNormal
                        elide: Text.ElideRight
                    }
                    Repeater {
                        model: 3
                        delegate: Item {
                            required property int index
                            readonly property int permPos: modelData.base + index
                            width: 64
                            height: 18
                            C.Checkbox {
                                checked: root.perm(parent.permPos)
                                enabled: false
                                opacity: 0.8
                            }
                        }
                    }
                }
            }
            Text {
                text: "Mode: " + (root.details.permissions || "")
                color: root.th.foregroundMuted
                font.family: root.th.fontFamilyMono
                font.pointSize: root.th.fontSizeSmall
            }
        }

        // ---- Open With (default application for this type)
        Column {
            visible: root.tab === "openwith"
            width: parent.width
            spacing: root.th.spacingSmall

            Text {
                text: "Default application for " + root.mime
                color: root.th.foregroundMuted
                font.family: root.th.fontFamily
                font.pointSize: root.th.fontSizeSmall
            }
            Repeater {
                model: root.apps
                delegate: Rectangle {
                    id: appRow
                    required property var modelData
                    width: col.width
                    height: root.th.rowHeight
                    radius: root.th.radiusSmall
                    color: appMouse.containsMouse ? root.th.surfaceHover : "transparent"

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        x: root.th.spacingMedium
                        spacing: root.th.spacingMedium
                        Text {
                            width: 12
                            text: appRow.modelData.isDefault ? "●" : ""
                            color: root.th.accent
                            font.pointSize: root.th.fontSizeSmall
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Image {
                            width: root.th.iconSizeSmall
                            height: root.th.iconSizeSmall
                            anchors.verticalCenter: parent.verticalCenter
                            source: "image://fang-icon/" + (appRow.modelData.iconName
                                    || "application-x-executable")
                            sourceSize: Qt.size(width, height)
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: appRow.modelData.name
                            color: root.th.foreground
                            font.family: root.th.fontFamily
                            font.pointSize: root.th.fontSizeNormal
                        }
                    }
                    MouseArea {
                        id: appMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            MimeResolver.setDefaultApp(root.mime, appRow.modelData.id)
                            root.apps = MimeResolver.appsFor(root.mime)
                        }
                    }
                }
            }
        }

        Row {
            anchors.right: parent.right
            C.Button {
                text: "Close"
                accent: true
                onClicked: root.close()
            }
        }
    }
}
