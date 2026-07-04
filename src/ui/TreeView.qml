import QtQuick as QQ
import QtQuick
import Fang
import "controls" as C

// Hierarchical lazy-expanding view with the same columns as Details
// (docs/05). Clicking selects; the arrow (or double-click/Enter) expands or
// navigates.
Item {
    id: root
    required property var fileView
    required property PaneController pane

    readonly property var th: Application.theme
    readonly property int columnsPerRow: 1

    function ensureVisible(row) {
        // Best-effort: view rows match top-level rows until nodes expand.
        if (row >= 0 && row < tree.rows)
            tree.positionViewAtRow(row, QQ.TableView.Contain)
    }

    function widthFor(key) {
        if (key === "name") {
            let others = 0
            for (let i = 0; i < pane.columns.length; ++i) {
                const k = pane.columns[i]
                if (k !== "name")
                    others += (pane.columnWidths[k] || 120)
            }
            return Math.max(pane.columnWidths["name"] || 320, width - others - 16)
        }
        return pane.columnWidths[key] || 120
    }

    MouseArea {
        anchors.fill: tree
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(ev) {
            if (ev.button === Qt.RightButton) {
                const p = mapToItem(null, ev.x, ev.y)
                root.fileView.backgroundRightClick(p.x, p.y)
            } else {
                root.fileView.backgroundClick()
            }
        }
    }

    QQ.TreeView {
        id: tree
        anchors.fill: parent
        anchors.rightMargin: 12
        clip: true
        model: root.pane.model
        boundsBehavior: QQ.Flickable.StopAtBounds
        columnWidthProvider: function(column) { return tree.width }

        delegate: Item {
            id: treeDele
            required property var treeView
            required property bool isTreeNode
            required property bool expanded
            required property bool hasChildren
            required property int depth
            required property int row
            required property var model

            readonly property bool isTopLevel: depth === 0
            readonly property bool isSelected:
                root.pane.selection.count >= 0 && model.selected

            implicitWidth: tree.width
            implicitHeight: root.th.rowHeight

            Rectangle {
                anchors.fill: parent
                radius: root.th.radiusSmall
                color: treeDele.isSelected ? root.th.selectionBackground
                     : rowMouse.containsMouse ? root.th.surfaceHover
                     : "transparent"
            }

            Row {
                anchors.fill: parent
                anchors.leftMargin: root.th.spacingMedium + treeDele.depth * 18

                // Expand arrow
                Item {
                    width: 16
                    height: parent.height
                    Text {
                        anchors.centerIn: parent
                        visible: treeDele.model.isDir
                        text: treeDele.expanded ? "▾" : "▸"
                        color: root.th.foregroundMuted
                        font.pointSize: root.th.fontSizeNormal
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (treeDele.model.isDir)
                                tree.toggleExpanded(treeDele.row)
                        }
                    }
                }

                Image {
                    width: root.th.iconSizeSmall
                    height: root.th.iconSizeSmall
                    anchors.verticalCenter: parent.verticalCenter
                    asynchronous: true
                    source: "image://fang-icon/" + treeDele.model.iconName
                    sourceSize: Qt.size(width, height)
                }
                Item { width: root.th.spacingMedium; height: 1 }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.widthFor("name") - treeDele.depth * 18 - 48
                    text: treeDele.model.name
                    color: treeDele.isSelected ? root.th.selectionForeground
                                               : root.th.foreground
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeNormal
                    font.italic: treeDele.model.isSymlink
                    elide: Text.ElideRight
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.widthFor("size")
                    text: treeDele.model.sizeText
                    color: root.th.foregroundMuted
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeNormal
                    elide: Text.ElideRight
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.widthFor("modified")
                    text: Qt.formatDateTime(treeDele.model.modified, "yyyy-MM-dd hh:mm")
                    color: root.th.foregroundMuted
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeNormal
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: rowMouse
                anchors.fill: parent
                anchors.leftMargin: root.th.spacingMedium + treeDele.depth * 18 + 16
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function(ev) {
                    if (ev.button === Qt.RightButton) {
                        root.pane.selection.selectPath(treeDele.model.path)
                        const p = mapToItem(null, ev.x, ev.y)
                        if (treeDele.isTopLevel)
                            root.fileView.rightClickRow(treeDele.row, p.x, p.y)
                        return
                    }
                    root.pane.selection.selectPath(treeDele.model.path)
                    root.fileView.forceActiveFocus()
                }
                onDoubleClicked: function(ev) {
                    if (ev.button !== Qt.LeftButton)
                        return
                    root.pane.activatePath(treeDele.model.path)
                }
            }
        }
    }

    C.ScrollBar {
        target: tree
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }
}
