import QtQuick
import Fang

// Transient toasts for Application.error and operation summaries (docs/05).
// Sits on top of the whole window; does not intercept clicks outside toasts.
Item {
    id: root

    readonly property var th: Application.theme
    property int nextId: 0

    function notify(title, detail) {
        toasts.append({ toastId: nextId++, title: String(title),
                        detail: String(detail || "") })
        // Keep the stack bounded.
        if (toasts.count > 5)
            toasts.remove(0)
    }

    function dismiss(toastId) {
        for (let i = 0; i < toasts.count; ++i) {
            if (toasts.get(i).toastId === toastId) {
                toasts.remove(i)
                return
            }
        }
    }

    ListModel { id: toasts }

    Connections {
        target: Application
        function onError(title, detail) { root.notify(title, detail) }
    }

    Component.onCompleted: {
        if (Application.overrideError !== "")
            notify("UI override failed — using bundled UI",
                   Application.overrideError)
    }

    Column {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: root.th.spacingLarge
        spacing: root.th.spacingSmall

        Repeater {
            model: toasts
            delegate: Rectangle {
                id: toast
                required property int toastId
                required property string title
                required property string detail

                width: Math.min(380, Math.max(titleText.implicitWidth,
                                              detailText.implicitWidth)
                                + root.th.spacingLarge * 2)
                height: col.implicitHeight + root.th.spacingMedium * 2
                radius: root.th.radiusMedium
                color: root.th.surface
                border.color: root.th.error
                border.width: 1

                Column {
                    id: col
                    anchors.fill: parent
                    anchors.margins: root.th.spacingMedium
                    spacing: 2

                    Text {
                        id: titleText
                        width: parent.width
                        text: toast.title
                        wrapMode: Text.Wrap
                        color: root.th.foreground
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeNormal
                        font.bold: true
                    }
                    Text {
                        id: detailText
                        width: parent.width
                        visible: toast.detail !== ""
                        text: toast.detail
                        wrapMode: Text.Wrap
                        maximumLineCount: 4
                        elide: Text.ElideRight
                        color: root.th.foregroundMuted
                        font.family: root.th.fontFamily
                        font.pointSize: root.th.fontSizeSmall
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.dismiss(toast.toastId)
                }

                Timer {
                    interval: 6000
                    running: true
                    onTriggered: root.dismiss(toast.toastId)
                }
            }
        }
    }
}
