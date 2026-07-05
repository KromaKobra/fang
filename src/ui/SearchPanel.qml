import QtQuick as QQ
import QtQuick
import Fang
import "controls" as C

// Query builder + results list bound to Application.search (docs/05).
// Activating a result opens its containing directory and selects the file.
Rectangle {
    id: root
    property PaneController pane: null
    signal closed()

    readonly property var th: Application.theme
    readonly property var search: Application.search

    color: th.backgroundAlt
    implicitHeight: queryRow.height + optionsRow.height + resultsArea.height
                    + th.spacingMedium * 4

    function openPanel() {
        nameField.forceFocus()
    }

    function runSearch() {
        const q = search.query
        q.rootPath = pane ? pane.location : ""
        q.namePattern = nameField.text
        q.contentSearch = contentCheck.checked && contentField.text !== ""
        q.contentQuery = contentField.text
        q.caseSensitive = caseCheck.checked
        q.regex = regexCheck.checked
        q.includeHidden = hiddenCheck.checked
        // Walk limits come from the [search] config group (docs/06).
        q.followSymlinks = Application.settings.searchFollowSymlinks
        q.maxDepth = Application.settings.searchMaxDepth
        q.ignorePaths = Application.settings.searchIgnorePaths
        search.start(q)
    }

    function activateResult(row) {
        const model = search.results
        const dir = model.parentDirAt(row)
        const path = model.pathAt(row)
        if (model.isDirAt(row)) {
            root.pane.open(path)
        } else if (dir !== "") {
            root.pane.open(dir)
            root.pane.selection.selectPath(path)
        }
    }

    Row {
        id: queryRow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: root.th.spacingMedium
        height: 30
        spacing: root.th.spacingMedium

        C.TextField {
            id: nameField
            width: parent.width * 0.35
            placeholder: "Name contains / glob…"
            onAccepted: root.runSearch()
            onEscapePressed: root.closed()
        }
        C.TextField {
            id: contentField
            visible: contentCheck.checked
            width: parent.width * 0.3
            placeholder: "File content…"
            onAccepted: root.runSearch()
            onEscapePressed: root.closed()
        }
        C.Button {
            text: root.search.running ? "Cancel" : "Search"
            accent: !root.search.running
            onClicked: root.search.running ? root.search.cancel() : root.runSearch()
        }
        C.IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "window-close"
            tooltip: "Close (Esc)"
            onClicked: root.closed()
        }
    }

    Row {
        id: optionsRow
        anchors.top: queryRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: root.th.spacingMedium
        anchors.topMargin: root.th.spacingSmall
        height: 22
        spacing: root.th.spacingLarge

        C.Checkbox {
            id: contentCheck
            text: "Search contents"
            checked: Application.settings.contentSearchDefault
        }
        C.Checkbox { id: caseCheck; text: "Case sensitive" }
        C.Checkbox { id: regexCheck; text: "Regex" }
        C.Checkbox {
            id: hiddenCheck
            text: "Hidden files"
            checked: !Application.settings.searchIgnoreHidden
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: {
                const s = root.search
                if (s.running)
                    return s.matchCount + " matches · scanning (" + s.dirsScanned + " dirs)…"
                if (s.results.count > 0 || s.dirsScanned > 0)
                    return s.matchCount + " matches · " + s.dirsScanned + " dirs scanned"
                return ""
            }
            color: root.th.foregroundMuted
            font.family: root.th.fontFamily
            font.pointSize: root.th.fontSizeSmall
        }

        C.Spinner {
            visible: root.search.running
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Item {
        id: resultsArea
        anchors.top: optionsRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: root.th.spacingMedium
        anchors.topMargin: root.th.spacingSmall
        height: root.search.results.count > 0 ? 260 : 0

        QQ.ListView {
            id: resultsList
            anchors.fill: parent
            anchors.rightMargin: 12
            clip: true
            model: root.search.results
            boundsBehavior: QQ.Flickable.StopAtBounds

            delegate: Rectangle {
                required property var model
                required property int index
                width: resultsList.width
                height: root.th.rowHeight
                radius: root.th.radiusSmall
                color: resultMouse.containsMouse ? root.th.surfaceHover : "transparent"

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: root.th.spacingMedium
                    spacing: root.th.spacingMedium

                    Image {
                        width: root.th.iconSizeSmall
                        height: root.th.iconSizeSmall
                        anchors.verticalCenter: parent.verticalCenter
                        asynchronous: true
                        source: "image://fang-icon/" + model.iconName
                        sourceSize: Qt.size(width, height)
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth, resultsList.width * 0.3)
                        text: model.name
                        color: root.th.foreground
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeNormal
                        elide: Text.ElideMiddle
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth, resultsList.width * 0.35)
                        text: model.parentDir
                        color: root.th.foregroundMuted
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeSmall
                        elide: Text.ElideMiddle
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: model.matchContext !== ""
                        width: Math.max(0, resultsList.width - x - root.th.spacingMedium)
                        text: model.matchContext
                        color: root.th.foregroundMuted
                        font.family: root.th.fontFamilyMono
                        font.pointSize: root.th.fontSizeSmall
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: resultMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.activateResult(parent.index)
                }
            }
        }

        C.ScrollBar {
            target: resultsList
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
        }
    }

    C.Separator { anchors.bottom: parent.bottom; width: parent.width }
}
