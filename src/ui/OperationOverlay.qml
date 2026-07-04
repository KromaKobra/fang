import QtQuick
import Fang
import "controls" as C

// Stacked bottom-right progress cards for active operations (docs/05):
// type, progress bar, current file, speed/ETA, pause/resume/cancel.
// Terminal cards linger briefly, then collapse.
Column {
    id: root
    property var fileOps: null

    spacing: th.spacingSmall
    readonly property var th: Application.theme

    function opTitle(op) {
        switch (op.type) {
        case Fang.OpType.Copy: return "Copying"
        case Fang.OpType.Move: return "Moving"
        case Fang.OpType.Delete: return "Deleting"
        case Fang.OpType.Trash: return "Moving to trash"
        case Fang.OpType.Restore: return "Restoring"
        case Fang.OpType.Rename: return "Renaming"
        case Fang.OpType.CreateFolder: return "Creating"
        }
        return "Working"
    }

    function stateNote(op) {
        switch (op.state) {
        case Fang.OpState.Paused: return "Paused"
        case Fang.OpState.WaitingConflict: return "Waiting for conflict resolution"
        case Fang.OpState.Completed: return "Done"
        case Fang.OpState.Failed: return "Failed"
        case Fang.OpState.Cancelled: return "Cancelled"
        }
        return ""
    }

    function etaText(seconds) {
        if (seconds < 0)
            return ""
        const m = Math.floor(seconds / 60)
        const s = seconds % 60
        return (m > 0 ? m + "m " : "") + s + "s left"
    }

    function baseName(pathOrUrl) {
        const s = String(pathOrUrl)
        return s.substring(s.lastIndexOf("/") + 1)
    }

    Repeater {
        model: root.fileOps ? root.fileOps.operations : []

        delegate: Rectangle {
            id: card
            required property var modelData
            readonly property var op: modelData
            readonly property bool terminal:
                op.state === Fang.OpState.Completed
                || op.state === Fang.OpState.Failed
                || op.state === Fang.OpState.Cancelled
            // Synchronous ops (rename/create) are born terminal; never show
            // a card for them, only for ops that actually ran.
            property bool sawRunning: false
            property bool expired: true

            onTerminalChanged: if (terminal && sawRunning) hideTimer.restart()
            Component.onCompleted: {
                if (!terminal) {
                    sawRunning = true
                    expired = false
                }
            }

            Timer {
                id: hideTimer
                interval: 2500
                onTriggered: card.expired = true
            }

            visible: !expired
            width: 300
            height: visible ? cardCol.implicitHeight + root.th.spacingMedium * 2 : 0
            radius: root.th.radiusMedium
            color: root.th.surface
            border.color: root.th.border
            border.width: 1

            Column {
                id: cardCol
                anchors.fill: parent
                anchors.margins: root.th.spacingMedium
                spacing: root.th.spacingSmall

                Item {
                    width: parent.width
                    height: root.th.iconSizeMedium

                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.opTitle(card.op)
                              + (card.op.filesTotal > 1
                                 ? " " + card.op.filesDone + " / " + card.op.filesTotal : "")
                        color: root.th.foreground
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeNormal
                        font.bold: true
                    }

                    Row {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        C.IconButton {
                            visible: !card.terminal
                            iconSize: 12
                            icon: card.op.state === Fang.OpState.Paused
                                  ? "media-playback-start" : "media-playback-pause"
                            tooltip: card.op.state === Fang.OpState.Paused ? "Resume" : "Pause"
                            onClicked: card.op.state === Fang.OpState.Paused
                                       ? card.op.resume() : card.op.pause()
                        }
                        C.IconButton {
                            visible: !card.terminal
                            iconSize: 12
                            icon: "process-stop"
                            tooltip: "Cancel"
                            onClicked: card.op.cancel()
                        }
                    }
                }

                // Progress bar
                Rectangle {
                    width: parent.width
                    height: 4
                    radius: 2
                    color: root.th.surfaceActive

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        radius: parent.radius
                        width: parent.width * Math.max(0, Math.min(1, card.op.progress))
                        color: card.op.state === Fang.OpState.Failed ? root.th.error
                             : card.op.state === Fang.OpState.Cancelled ? root.th.warning
                             : root.th.accent
                    }
                }

                Text {
                    width: parent.width
                    visible: text !== ""
                    text: card.terminal || card.op.state === Fang.OpState.Paused
                              || card.op.state === Fang.OpState.WaitingConflict
                          ? root.stateNote(card.op)
                          : root.baseName(card.op.currentSource)
                    color: root.th.foregroundMuted
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeSmall
                    elide: Text.ElideMiddle
                }

                Text {
                    width: parent.width
                    visible: !card.terminal && card.op.speedBytesPerSec > 0
                    text: Application.formatSize(card.op.speedBytesPerSec) + "/s"
                          + (card.op.etaSeconds >= 0
                             ? " · " + root.etaText(card.op.etaSeconds) : "")
                    color: root.th.foregroundMuted
                    font.family: root.th.fontFamily
                    font.pointSize: root.th.fontSizeSmall
                    elide: Text.ElideRight
                }
            }
        }
    }
}
