import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import ".."

Item {
    id: root
    width: imageLabel.visible ? imageLabel.width * 1.2 : textLabel.width
    height: imageLabel.visible ? imageLabel.height : textLabel.height
    property real size: (1.0 + 0.6 * PathView.selectedValue)
    property bool isCurrentItem: PathView.isCurrentItem

    signal pressed(var index)

    Text {
        id: textLabel
        text: name ? name : ""
        visible: name ? true : false
        font.family: mainFont.name
        font.pointSize: 16 * root.size
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        color: isCurrentItem ? "black" : "grey"
    }

    Image {
        id: imageLabel
        visible: icon ? true : false
        anchors.centerIn: parent
        source: icon ? icon : ""
        height: 64 * root.size
        width: 64 * root.size
        sourceSize.width: width
        sourceSize.height: height
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.pressed(index)
    }

}

