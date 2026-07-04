import QtQuick
import QtQuick.Controls.Basic as QQC
import Fang
import "controls" as C

// Modal conflict resolution (docs/05): shows both files' details, offers
// Overwrite / Skip / Keep Both / Rename / Merge (dirs) plus apply-to-all.
// Conflicts from concurrent operations queue up; Escape means Skip.
C.Popup {
    id: root

    property var operation: null
    property string sourceUrl: ""
    property string destUrl: ""
    property var info: ({})
    property bool resolved: false
    property var queue: []

    readonly property bool bothDirs:
        info && info.sourceIsDir === true && info.destIsDir === true

    modal: true
    anchors.centerIn: parent
    closePolicy: QQC.Popup.CloseOnEscape
    contentWidth: 420
    contentHeight: col.implicitHeight

    function enqueue(op, src, dst, conflictInfo) {
        queue.push({ op: op, src: src, dst: dst, info: conflictInfo })
        if (!opened)
            showNext()
    }

    function showNext() {
        const c = queue.shift()
        operation = c.op
        sourceUrl = c.src
        destUrl = c.dst
        info = c.info
        resolved = false
        open()
    }

    function resolve(mode) {
        resolved = true
        if (operation)
            operation.resolveConflict(mode, applyAll.checked)
        close()
    }

    onClosed: {
        if (!resolved && operation)
            operation.resolveConflict(Fang.ConflictMode.Skip, false)
        operation = null
        if (queue.length > 0)
            Qt.callLater(showNext)
        else
            applyAll.checked = false
    }

    function detailLine(prefix) {
        if (!info)
            return ""
        const size = info[prefix + "Size"]
        const mtime = info[prefix + "Modified"]
        const isDir = info[prefix + "IsDir"] === true
        let parts = []
        parts.push(isDir ? "Folder" : Application.formatSize(size !== undefined ? size : 0))
        if (mtime)
            parts.push("modified " + Qt.formatDateTime(mtime, "yyyy-MM-dd hh:mm"))
        return parts.join(" · ")
    }

    Column {
        id: col
        width: parent.width
        spacing: root.th.spacingMedium

        Text {
            width: parent.width
            text: root.bothDirs
                  ? "Merge folder “" + (root.info ? root.info.destName : "") + "”?"
                  : "Replace “" + (root.info ? root.info.destName : "") + "”?"
            color: root.th.foreground
            font.family: root.th.fontFamily
            font.pointSize: root.th.fontSizeLarge
            font.bold: true
            elide: Text.ElideMiddle
        }

        Text {
            width: parent.width
            text: "The destination already contains an item with this name."
            color: root.th.foregroundMuted
            font.family: root.th.fontFamily
            font.pointSize: root.th.fontSizeNormal
            wrapMode: Text.WordWrap
        }

        // Both files' details, source over destination.
        Column {
            width: parent.width
            spacing: root.th.spacingSmall

            Repeater {
                model: [
                    { label: "Source:", prefix: "source" },
                    { label: "Existing:", prefix: "dest" },
                ]
                delegate: Row {
                    required property var modelData
                    spacing: root.th.spacingMedium
                    Text {
                        width: 70
                        text: modelData.label
                        color: root.th.foregroundMuted
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeNormal
                    }
                    Text {
                        text: root.detailLine(modelData.prefix)
                        color: root.th.foreground
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeNormal
                    }
                }
            }
        }

        C.Checkbox {
            id: applyAll
            text: "Apply to all remaining conflicts"
        }

        Flow {
            width: parent.width
            spacing: root.th.spacingSmall
            layoutDirection: Qt.RightToLeft

            C.Button {
                text: root.bothDirs ? "Merge" : "Overwrite"
                accent: true
                onClicked: root.resolve(root.bothDirs ? Fang.ConflictMode.Merge
                                                      : Fang.ConflictMode.Overwrite)
            }
            C.Button {
                visible: root.bothDirs
                text: "Overwrite"
                onClicked: root.resolve(Fang.ConflictMode.Overwrite)
            }
            C.Button {
                text: "Keep Both"
                onClicked: root.resolve(Fang.ConflictMode.KeepBoth)
            }
            C.Button {
                text: "Rename"
                onClicked: root.resolve(Fang.ConflictMode.Rename)
            }
            C.Button {
                text: "Skip"
                onClicked: root.resolve(Fang.ConflictMode.Skip)
            }
        }
    }
}
