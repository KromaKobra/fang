import QtQuick as QQ
import QtQuick
import Fang
import "controls" as C

// Places/bookmarks/devices/network/trash sidebar (docs/05): collapsible
// sections, click to open, middle-click to open in a new tab, drag to
// reorder bookmarks, right-click menu, eject buttons. Drop targets for
// copying files onto a place arrive with stage-4 DnD.
Rectangle {
    id: root
    property PaneController pane: null
    property WindowController win: null

    readonly property var th: Application.theme
    // section name -> true when collapsed; reassigned on toggle so
    // bindings refresh.
    property var collapsedSections: ({})

    color: th.backgroundAlt

    function toggleSection(name) {
        const next = Object.assign({}, collapsedSections)
        next[name] = !next[name]
        collapsedSections = next
    }

    function openPlace(model) {
        if (!model.isMounted) {
            Application.places.mount(model.id)
            return
        }
        root.pane.open(Application.places.activate(model.url))
    }

    QQ.ListView {
        id: list
        anchors.fill: parent
        anchors.topMargin: root.th.spacingSmall
        anchors.leftMargin: root.th.spacingSmall
        anchors.rightMargin: root.th.spacingSmall
        clip: true
        model: Application.places
        boundsBehavior: QQ.Flickable.StopAtBounds

        section.property: "sectionName"
        section.delegate: Item {
            required property string section
            width: list.width
            height: root.th.rowHeight

            Text {
                anchors.left: parent.left
                anchors.leftMargin: root.th.spacingSmall
                anchors.verticalCenter: parent.verticalCenter
                text: (root.collapsedSections[parent.section] ? "▸ " : "▾ ") + parent.section
                color: root.th.foregroundMuted
                font.family: root.th.fontFamily
                font.pointSize: root.th.fontSizeSmall
                font.bold: true
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.toggleSection(parent.section)
            }
        }

        delegate: Item {
            id: wrapper
            required property var model
            required property int index
            readonly property string sec: QQ.ListView.section
            readonly property bool collapsed: root.collapsedSections[sec] === true

            width: list.width
            height: collapsed ? 0 : root.th.rowHeight
            visible: !collapsed
            z: dragOffset !== 0 ? 2 : 0

            // Vertical drag-reorder for bookmarks (transform, then a single
            // moveBookmark on release — same pattern as TabButton).
            property real dragOffset: 0
            transform: Translate { y: wrapper.dragOffset }

            PlaceRow {
                id: placeRow
                anchors.fill: parent
                model: wrapper.model
                current: root.pane && root.pane.location === wrapper.model.url
                         && wrapper.model.url !== ""
                onClicked: root.openPlace(wrapper.model)
                onMiddleClicked: if (wrapper.model.isMounted)
                                     root.win.addTab(wrapper.model.url)
                onRightClicked: function(sceneX, sceneY) {
                    placeMenu.placeModel = wrapper.model
                    placeMenu.popup(sceneX, sceneY)
                }
            }

            MouseArea {
                // Bookmark reorder drag handle: overlays the row but only
                // reacts to press-and-drag; plain clicks fall through.
                anchors.fill: parent
                enabled: wrapper.model.isBookmark
                acceptedButtons: Qt.LeftButton
                preventStealing: true
                property real pressY: 0
                property bool dragging: false
                onPressed: function(ev) { pressY = ev.y; dragging = false }
                onPositionChanged: function(ev) {
                    if (!pressed)
                        return
                    if (!dragging && Math.abs(ev.y - pressY) > 8)
                        dragging = true
                    if (dragging)
                        wrapper.dragOffset = ev.y - pressY
                }
                onReleased: function(ev) {
                    if (!dragging) {
                        wrapper.dragOffset = 0
                        return
                    }
                    const shift = Math.round(wrapper.dragOffset / root.th.rowHeight)
                    wrapper.dragOffset = 0
                    dragging = false
                    if (shift !== 0) {
                        const from = Application.places.bookmarkIndex(wrapper.model.id)
                        if (from >= 0)
                            Application.places.moveBookmark(from, from + shift)
                    }
                }
                onClicked: root.openPlace(wrapper.model)
            }
        }
    }

    C.ScrollBar {
        target: list
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }

    C.Separator {
        vertical: true
        anchors.right: parent.right
    }

    // ------------------------------------------------------- context menu

    C.Menu {
        id: placeMenu
        property var placeModel: null
        readonly property bool isBookmark: placeModel ? placeModel.isBookmark : false
        readonly property bool isDevice:
            placeModel ? placeModel.section === Fang.PlaceSection.Devices : false

        C.MenuItem {
            text: "Open"
            icon: "document-open"
            onTriggered: root.openPlace(placeMenu.placeModel)
        }
        C.MenuItem {
            text: "Open in New Tab"
            icon: "tab-new"
            enabled: placeMenu.placeModel ? placeMenu.placeModel.isMounted : false
            onTriggered: root.win.addTab(placeMenu.placeModel.url)
        }
        C.Separator { visible: placeMenu.isBookmark }
        C.MenuItem {
            visible: placeMenu.isBookmark
            text: "Rename Bookmark…"
            icon: "edit-rename"
            onTriggered: {
                renamePopup.placeId = placeMenu.placeModel.id
                renameField.text = placeMenu.placeModel.label
                renamePopup.open()
                renameField.forceFocus()
            }
        }
        C.MenuItem {
            visible: placeMenu.isBookmark
            text: "Remove Bookmark"
            icon: "edit-delete"
            onTriggered: Application.places.removeBookmark(placeMenu.placeModel.id)
        }
        C.Separator { visible: placeMenu.isDevice }
        C.MenuItem {
            visible: placeMenu.isDevice
                     && (placeMenu.placeModel ? placeMenu.placeModel.isMounted : false)
            text: "Unmount"
            icon: "media-eject"
            onTriggered: Application.places.unmount(placeMenu.placeModel.id)
        }
        C.MenuItem {
            visible: placeMenu.isDevice
                     && (placeMenu.placeModel ? placeMenu.placeModel.isEjectable : false)
            text: "Eject"
            icon: "media-eject"
            onTriggered: Application.places.eject(placeMenu.placeModel.id)
        }
    }

    // -------------------------------------------------- bookmark renaming

    C.Popup {
        id: renamePopup
        property string placeId: ""
        modal: true
        anchors.centerIn: parent
        contentWidth: 260
        contentHeight: renameCol.implicitHeight

        Column {
            id: renameCol
            width: parent.width
            spacing: root.th.spacingMedium

            Text {
                text: "Rename bookmark"
                color: root.th.foreground
                font.family: root.th.fontFamily
                font.pointSize: root.th.fontSizeNormal
                font.bold: true
            }
            C.TextField {
                id: renameField
                width: parent.width
                selectOnFocus: true
                onAccepted: {
                    Application.places.renameBookmark(renamePopup.placeId, text)
                    renamePopup.close()
                }
                onEscapePressed: renamePopup.close()
            }
            Row {
                anchors.right: parent.right
                spacing: root.th.spacingSmall
                C.Button {
                    text: "Cancel"
                    onClicked: renamePopup.close()
                }
                C.Button {
                    text: "Rename"
                    accent: true
                    onClicked: {
                        Application.places.renameBookmark(renamePopup.placeId,
                                                          renameField.text)
                        renamePopup.close()
                    }
                }
            }
        }
    }
}
