import QtQuick
import Fang

Rectangle {
    property bool vertical: false
    implicitWidth: vertical ? 1 : parent ? parent.width : 100
    implicitHeight: vertical ? (parent ? parent.height : 100) : 1
    color: Application.theme.border
}
