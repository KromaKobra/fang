import QtQuick
import QtQuick.Controls.Basic as QQC
import Fang
import "controls" as C

// Breadcrumb bar that flips into an editable path field on click / Ctrl+L,
// with per-segment sibling dropdowns and subdirectory autocomplete (docs/05).
Item {
    id: root
    property PaneController pane: null
    property bool editing: false

    readonly property var th: Application.theme
    readonly property var nav: pane ? pane.navigation : null

    implicitHeight: th.rowHeight

    function startEditing() {
        editing = true
        editField.text = root.pane.location
        editField.forceFocus()
    }
    function stopEditing() {
        editing = false
        completions.close()
    }

    Rectangle {
        anchors.fill: parent
        radius: root.th.radiusSmall
        color: root.th.backgroundAlt
        border.color: root.editing ? root.th.accent : root.th.border
        border.width: 1
    }

    // ------------------------------------------------------- breadcrumbs
    Flickable {
        id: crumbFlick
        visible: !root.editing
        anchors.fill: parent
        anchors.leftMargin: root.th.spacingSmall
        anchors.rightMargin: root.th.spacingSmall
        contentWidth: crumbRow.implicitWidth
        contentHeight: height
        flickableDirection: Flickable.HorizontalFlick
        clip: true

        Row {
            id: crumbRow
            height: crumbFlick.height

            Repeater {
                model: root.nav ? root.nav.breadcrumbs : []
                delegate: Row {
                    id: crumb
                    required property var modelData
                    required property int index
                    height: crumbRow.height

                    Rectangle {
                        width: crumbLabel.implicitWidth + root.th.spacingMedium
                        height: parent.height - 6
                        anchors.verticalCenter: parent.verticalCenter
                        radius: root.th.radiusSmall
                        color: crumbMouse.containsMouse ? root.th.surfaceHover : "transparent"

                        Text {
                            id: crumbLabel
                            anchors.centerIn: parent
                            text: crumb.modelData.label
                            color: root.th.foreground
                            font.family: root.th.fontFamily
                            font.pointSize: root.th.fontSizeNormal
                        }
                        MouseArea {
                            id: crumbMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.nav.goTo(crumb.modelData.url)
                        }
                    }
                    // Sibling dropdown on the separator
                    Rectangle {
                        width: 14
                        height: parent.height - 6
                        anchors.verticalCenter: parent.verticalCenter
                        radius: root.th.radiusSmall
                        color: sepMouse.containsMouse ? root.th.surfaceHover : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "›"
                            color: root.th.foregroundMuted
                            font.pointSize: root.th.fontSizeNormal
                        }
                        MouseArea {
                            id: sepMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                siblingMenu.baseUrl = crumb.modelData.url
                                siblingMenu.dirs = root.nav.childDirs(crumb.modelData.url)
                                siblingMenu.popupBelow(parent)
                            }
                        }
                    }
                }
            }
        }

        // Click on empty area -> edit mode
        MouseArea {
            anchors.fill: parent
            z: -1
            onClicked: root.startEditing()
        }
    }

    C.Menu {
        id: siblingMenu
        property string baseUrl: "/"
        property var dirs: []
        Repeater {
            model: siblingMenu.dirs
            delegate: C.MenuItem {
                required property string modelData
                text: modelData
                icon: "folder"
                onTriggered: root.nav.goTo(
                    (siblingMenu.baseUrl === "/" ? "" : siblingMenu.baseUrl) + "/" + modelData)
            }
        }
    }

    // --------------------------------------------------------- edit mode
    C.TextField {
        id: editField
        visible: root.editing
        anchors.fill: parent
        onAccepted: {
            root.nav.commitEdit(text)
            root.stopEditing()
        }
        onEscapePressed: root.stopEditing()
        input.onActiveFocusChanged: {
            if (!input.activeFocus)
                root.stopEditing()
        }
        input.onTextEdited: {
            const t = text
            const slash = t.lastIndexOf("/")
            if (slash < 0) {
                completions.close()
                return
            }
            const dir = slash === 0 ? "/" : t.substring(0, slash)
            const prefix = t.substring(slash + 1).toLowerCase()
            const subs = root.nav.childDirs(dir)
            let matches = []
            for (let i = 0; i < subs.length && matches.length < 8; ++i) {
                if (prefix === "" || subs[i].toLowerCase().startsWith(prefix))
                    matches.push((dir === "/" ? "/" : dir + "/") + subs[i])
            }
            completions.paths = matches
            if (matches.length > 0) {
                completions.parent = root
                completions.x = 0
                completions.y = root.height + 2
                completions.width = root.width
                if (!completions.opened)
                    completions.open()
                completions.relayout()
            } else {
                completions.close()
            }
        }
        Keys.onTabPressed: {
            if (completions.paths.length > 0) {
                text = completions.paths[0]
                input.cursorPosition = text.length
            }
        }
    }

    C.Menu {
        id: completions
        property var paths: []
        closePolicy: QQC.Popup.NoAutoClose // stays while typing; closed programmatically
        modal: false
        Repeater {
            model: completions.paths
            delegate: C.MenuItem {
                required property string modelData
                text: modelData
                icon: "folder"
                onTriggered: {
                    editField.text = modelData
                    root.nav.commitEdit(modelData)
                    root.stopEditing()
                }
            }
        }
    }
}
