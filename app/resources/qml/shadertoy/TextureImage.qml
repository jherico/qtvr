import QtQuick 2.5
import QtQuick.Controls 1.4

import "."

Image {
    id: root
    width: 128; height: 128;
    property int channel;

    MouseArea {
        anchors.fill: parent;
        onClicked: desktop.pickTexture(channel);
    }

    Rectangle {
        z: -1
        anchors.fill: parent
        color: "#00000000"
        border.width: 4
        border.color: "black"
        radius: 8
    }
}
