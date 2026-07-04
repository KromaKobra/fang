import QtQuick as QQ
import QtQuick
import Fang
import "controls" as C

// Multi-column list with reorderable/resizable/sortable headers (docs/05).
Item {
    id: root
    required property var fileView
    required property PaneController pane

    readonly property var th: Application.theme
    readonly property int columnsPerRow: 1

    readonly property var sortMap: ({
        name: Fang.SortRole.ByName,
        size: Fang.SortRole.BySize,
        type: Fang.SortRole.ByType,
        modified: Fang.SortRole.ByModified,
        created: Fang.SortRole.ByCreated,
        accessed: Fang.SortRole.ByAccessed,
        extension: Fang.SortRole.ByExtension,
        owner: Fang.SortRole.ByOwner,
    })
    readonly property var columnLabels: ({
        name: "Name", size: "Size", type: "Type", modified: "Modified",
        created: "Created", accessed: "Accessed", permissions: "Permissions",
        owner: "Owner", group: "Group", extension: "Extension",
    })
    readonly property var allColumnKeys:
        ["name", "size", "type", "modified", "created", "accessed",
         "permissions", "owner", "group", "extension"]

    function widthFor(key) {
        if (key === "name") {
            let others = 0
            for (let i = 0; i < pane.columns.length; ++i) {
                const k = pane.columns[i]
                if (k !== "name")
                    others += (pane.columnWidths[k] || 120)
            }
            const configured = pane.columnWidths["name"] || 320
            return Math.max(configured, width - others - 16)
        }
        return pane.columnWidths[key] || 120
    }

    function cellText(model, key) {
        switch (key) {
        case "name": return model.name
        case "size": return model.sizeText
        case "type": return model.typeText
        case "modified": return Qt.formatDateTime(model.modified, "yyyy-MM-dd hh:mm")
        case "created": return Qt.formatDateTime(model.created, "yyyy-MM-dd hh:mm")
        case "accessed": return Qt.formatDateTime(model.accessed, "yyyy-MM-dd hh:mm")
        case "permissions": return model.permissions
        case "owner": return model.owner
        case "group": return model.group
        case "extension": return model.extension
        default: return ""
        }
    }

    function toggleSort(key) {
        if (!(key in sortMap))
            return
        const role = sortMap[key]
        pane.setSort(role, pane.sortRole === role ? !pane.sortDescending : false)
    }

    function headerDropIndex(x) {
        let acc = 0
        for (let i = 0; i < pane.columns.length; ++i) {
            const w = widthFor(pane.columns[i])
            if (x < acc + w / 2)
                return i
            acc += w
        }
        return pane.columns.length - 1
    }

    function openColumnsMenu(sceneX, sceneY) {
        columnsMenu.popup(sceneX, sceneY)
    }

    // ------------------------------------------------------------ header

    Rectangle {
        id: header
        width: parent.width
        height: root.th.rowHeight
        color: root.th.backgroundAlt

        Row {
            anchors.fill: parent
            Repeater {
                model: root.pane.columns
                delegate: ColumnHeader {
                    required property string modelData
                    required property int index
                    key: modelData
                    columnIndex: index
                    label: root.columnLabels[modelData] || modelData
                    detailsView: root
                    pane: root.pane
                    width: root.widthFor(modelData)
                    height: header.height
                }
            }
        }
        C.Separator { anchors.bottom: parent.bottom }
    }

    // -------------------------------------------------------------- rows

    MouseArea {
        anchors.fill: list
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(ev) {
            if (ev.button === Qt.RightButton) {
                const p = mapToItem(null, ev.x, ev.y)
                root.fileView.backgroundRightClick(p.x, p.y)
            } else {
                root.fileView.backgroundClick()
            }
        }
        onPressed: function(ev) {
            if (ev.button === Qt.LeftButton)
                rubber.begin(ev.x, ev.y)
        }
        onPositionChanged: function(ev) {
            if (pressed && (ev.buttons & Qt.LeftButton))
                rubber.update(ev.x, ev.y)
        }
        onReleased: rubber.end()
    }

    QQ.ListView {
        id: list
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 12
        clip: true
        model: root.pane.model
        boundsBehavior: QQ.Flickable.StopAtBounds
        reuseItems: true

        delegate: FileDelegate {
            id: dele
            required property var model
            required property int index

            fileView: root.fileView
            row: index
            filePath: model.path
            fileName: model.name
            fileUrl: model.url
            isDir: model.isDir
            selected: model.selected
            width: list.width
            height: root.th.rowHeight

            Row {
                anchors.fill: parent
                Repeater {
                    model: root.pane.columns
                    delegate: Item {
                        required property string modelData
                        width: root.widthFor(modelData)
                        height: root.th.rowHeight

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: root.th.spacingMedium
                            spacing: root.th.spacingMedium

                            Item {
                                visible: modelData === "name"
                                width: root.th.iconSizeSmall
                                height: root.th.iconSizeSmall
                                anchors.verticalCenter: parent.verticalCenter

                                Image {
                                    anchors.fill: parent
                                    asynchronous: true
                                    visible: rowThumb.status !== Image.Ready
                                    source: parent.visible
                                        ? "image://fang-icon/" + dele.model.iconName : ""
                                    sourceSize: Qt.size(width, height)
                                }
                                Image {
                                    id: rowThumb
                                    anchors.fill: parent
                                    asynchronous: true
                                    visible: status === Image.Ready
                                    source: parent.visible ? dele.model.thumbnail : ""
                                    sourceSize: Qt.size(width, height)
                                }
                            }
                            Text {
                                width: parent.width - (modelData === "name"
                                    ? root.th.iconSizeSmall + root.th.spacingMedium * 2.5
                                    : root.th.spacingMedium * 1.5)
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.cellText(dele.model, modelData)
                                color: modelData === "name"
                                    ? (dele.selected ? root.th.selectionForeground
                                                     : root.th.foreground)
                                    : root.th.foregroundMuted
                                font.family: root.th.fontFamily
                                font.pointSize: root.th.fontSizeNormal
                                font.italic: modelData === "name" && dele.model.isSymlink
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: rubber
        property real startX: 0
        property real startY: 0
        property bool active: false
        visible: active && (width > 4 || height > 4)
        color: Qt.alpha(root.th.accent, 0.15)
        border.color: root.th.accent
        border.width: 1
        z: 10

        function begin(x0, y0) {
            startX = x0; startY = y0 + header.height
            x = x0; y = startY
            width = 0; height = 0
            active = true
        }
        function update(x1, y1) {
            if (!active)
                return
            const ya = y1 + header.height
            x = Math.min(startX, x1)
            y = Math.min(startY, ya)
            width = Math.abs(x1 - startX)
            height = Math.abs(ya - startY)
            const top = y - header.height + list.contentY
            const bottom = top + height
            const from = list.indexAt(10, top + 1)
            const to = list.indexAt(10, bottom - 1)
            const sel = root.pane.selection
            const lo = from >= 0 ? from : 0
            const hi = to >= 0 ? to : root.pane.itemCount - 1
            if (hi >= lo && root.pane.itemCount > 0)
                sel.range(lo, hi)
        }
        function end() { active = false }
    }

    C.ScrollBar {
        target: list
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
    }

    // Column chooser (right-click on header)
    C.Menu {
        id: columnsMenu
        Repeater {
            model: root.allColumnKeys
            delegate: C.MenuItem {
                required property string modelData
                text: root.columnLabels[modelData]
                showChecked: true
                checked: root.pane.columns.indexOf(modelData) >= 0
                enabled: modelData !== "name" // name is always present
                onTriggered: {
                    let cols = root.pane.columns.slice()
                    const idx = cols.indexOf(modelData)
                    if (idx >= 0)
                        cols.splice(idx, 1)
                    else
                        cols.push(modelData)
                    root.pane.setColumns(cols)
                }
            }
        }
    }
}
