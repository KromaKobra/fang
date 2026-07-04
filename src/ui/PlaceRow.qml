import QtQuick
import Fang
import "controls" as C

// One sidebar entry: icon, label, capacity bar (devices), badge (trash
// count), eject button (docs/05). Reads Places roles via `model` because
// the "id" role clashes with the QML id keyword.
Rectangle {
    id: root
    property var model: null
    property bool current: false
    property bool dropHover: false
    signal clicked()
    signal middleClicked()
    signal rightClicked(real sceneX, real sceneY)

    readonly property var th: Application.theme
    readonly property bool showCapacity:
        model && model.totalBytes > 0 && model.section === Fang.PlaceSection.Devices

    implicitHeight: th.rowHeight
    radius: th.radiusSmall
    color: current ? th.surfaceActive
         : (mouse.containsMouse || dropHover) ? th.surfaceHover
         : "transparent"
    border.color: th.accent
    border.width: dropHover ? 1 : 0

    Image {
        id: icon
        anchors.left: parent.left
        anchors.leftMargin: root.th.spacingMedium
        anchors.verticalCenter: parent.verticalCenter
        width: root.th.iconSizeSmall
        height: root.th.iconSizeSmall
        source: root.model ? "image://fang-icon/" + root.model.iconName : ""
        sourceSize: Qt.size(width, height)
        opacity: root.model && !root.model.isMounted ? 0.5 : 1.0
    }

    Text {
        id: label
        anchors.left: icon.right
        anchors.leftMargin: root.th.spacingMedium
        anchors.right: badge.visible ? badge.left : (eject.visible ? eject.left : parent.right)
        anchors.rightMargin: root.th.spacingSmall
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: root.showCapacity ? -4 : 0
        text: root.model ? root.model.label : ""
        elide: Text.ElideRight
        color: root.th.foreground
        font.family: root.th.fontFamily
        font.pointSize: root.th.fontSizeNormal
    }

    // Capacity bar for mounted devices.
    Rectangle {
        visible: root.showCapacity
        anchors.left: label.left
        anchors.right: eject.visible ? eject.left : parent.right
        anchors.rightMargin: root.th.spacingMedium
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        height: 3
        radius: 1.5
        color: root.th.surfaceActive

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            radius: parent.radius
            width: parent.width * Math.min(1, root.model.usedBytes / root.model.totalBytes)
            color: root.th.accent
        }
    }

    C.Badge {
        id: badge
        visible: root.model && root.model.itemCount > 0
        anchors.right: parent.right
        anchors.rightMargin: root.th.spacingMedium
        anchors.verticalCenter: parent.verticalCenter
        text: root.model && root.model.itemCount > 0 ? String(root.model.itemCount) : ""
    }

    C.IconButton {
        id: eject
        visible: root.model ? root.model.isEjectable : false
        anchors.right: parent.right
        anchors.rightMargin: root.th.spacingSmall
        anchors.verticalCenter: parent.verticalCenter
        iconSize: 12
        icon: "media-eject"
        tooltip: "Eject"
        onClicked: Application.places.eject(root.model.id)
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        anchors.rightMargin: eject.visible ? eject.width + root.th.spacingSmall : 0
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        onClicked: function(ev) {
            if (ev.button === Qt.RightButton) {
                const p = mapToItem(null, ev.x, ev.y)
                root.rightClicked(p.x, p.y)
            } else if (ev.button === Qt.MiddleButton) {
                root.middleClicked()
            } else {
                root.clicked()
            }
        }
    }
}
