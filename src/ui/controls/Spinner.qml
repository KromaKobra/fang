import QtQuick
import Fang

Item {
    id: root
    property bool running: visible
    implicitWidth: 18
    implicitHeight: 18

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = Application.theme.accent
            ctx.lineWidth = 2.5
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.arc(width / 2, height / 2, width / 2 - 2, 0, Math.PI * 1.4)
            ctx.stroke()
        }
        Component.onCompleted: requestPaint()
    }

    RotationAnimation on rotation {
        running: root.running
        from: 0
        to: 360
        duration: 900
        loops: Animation.Infinite
    }
}
