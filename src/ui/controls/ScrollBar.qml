import QtQuick
import Fang

// Custom scrollbar for a Flickable/ListView/GridView (vertical by default).
Item {
    id: root
    property Flickable target: null
    property bool horizontal: false
    readonly property var th: Application.theme
    readonly property bool needed: target && (horizontal
        ? target.contentWidth > target.width + 1
        : target.contentHeight > target.height + 1)

    width: horizontal ? (parent ? parent.width : 100) : 10
    height: horizontal ? 10 : (parent ? parent.height : 100)
    visible: needed

    readonly property real span: horizontal ? width : height
    readonly property real viewSpan: target ? (horizontal ? target.width : target.height) : 1
    readonly property real contentSpan: target
        ? (horizontal ? target.contentWidth : target.contentHeight) : 1
    readonly property real pos: target ? (horizontal ? target.contentX : target.contentY) : 0

    Rectangle {
        id: handle
        x: root.horizontal
            ? (root.span - width) * Math.min(1, Math.max(0,
                root.pos / Math.max(1, root.contentSpan - root.viewSpan)))
            : 1
        y: root.horizontal
            ? 1
            : (root.span - height) * Math.min(1, Math.max(0,
                root.pos / Math.max(1, root.contentSpan - root.viewSpan)))
        width: root.horizontal
            ? Math.max(24, root.span * root.viewSpan / Math.max(1, root.contentSpan)) : 8
        height: root.horizontal
            ? 8 : Math.max(24, root.span * root.viewSpan / Math.max(1, root.contentSpan))
        radius: 4
        color: dragArea.pressed || dragArea.containsMouse ? root.th.scrollbarHover
                                                          : root.th.scrollbar
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true
        function apply(px) {
            if (!root.needed)
                return
            const handleSpan = root.horizontal ? handle.width : handle.height
            const frac = Math.min(1, Math.max(0,
                (px - handleSpan / 2) / Math.max(1, root.span - handleSpan)))
            const target = frac * (root.contentSpan - root.viewSpan)
            if (root.horizontal)
                root.target.contentX = target
            else
                root.target.contentY = target
        }
        onPressed: function(ev) { apply(root.horizontal ? ev.x : ev.y) }
        onPositionChanged: function(ev) { if (pressed) apply(root.horizontal ? ev.x : ev.y) }
    }
}
