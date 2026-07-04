import QtQuick
import Fang
import "controls" as C

// New-folder / new-file name prompt (docs/05). "From template" joins when
// templates ship (Phase 2).
C.Popup {
    id: root
    property string mode: "folder" // "folder" | "file"
    signal accepted(string mode, string name)

    modal: true
    anchors.centerIn: parent
    contentWidth: 280
    contentHeight: col.implicitHeight

    function openFor(mode) {
        root.mode = mode
        nameField.text = mode === "folder" ? "New Folder" : "New File"
        open()
        nameField.forceFocus()
    }

    function commit() {
        const name = nameField.text.trim()
        if (name === "")
            return
        root.accepted(root.mode, name)
        root.close()
    }

    Column {
        id: col
        width: parent.width
        spacing: root.th.spacingMedium

        Text {
            text: root.mode === "folder" ? "New folder" : "New file"
            color: root.th.foreground
            font.family: root.th.fontFamily
            font.pointSize: root.th.fontSizeNormal
            font.bold: true
        }
        C.TextField {
            id: nameField
            width: parent.width
            selectOnFocus: true
            onAccepted: root.commit()
            onEscapePressed: root.close()
        }
        Row {
            anchors.right: parent.right
            spacing: root.th.spacingSmall
            C.Button {
                text: "Cancel"
                onClicked: root.close()
            }
            C.Button {
                text: "Create"
                accent: true
                onClicked: root.commit()
            }
        }
    }
}
