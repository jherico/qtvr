import QtQuick 2.5
import QtQuick.Controls 1.4

import "."

Rectangle {
    id: root
    width: 640
    height: image.y + image.height + 12

    color: "white"
    border { width: 2; color: "black";  }
    radius: 4

    property var shaderId;
    property var shaderInfo;

    Timer {
        id: refreshTimer
        interval: Math.random() * 100 + 50;
        repeat: false
        running: false
        onTriggered: shadertoy.api.fetchShaderInfo(shaderId, function(result){
            if (!root || !shaderId) { return; }
            shaderInfo = result;
        });
    }

    Component.onCompleted: refreshTimer.start()


    Text {
        id: label
        anchors { left: parent.left; top: parent.top; margins: 4; leftMargin: 8}
        font.pointSize: 12
        font.bold: true
        text: shaderInfo ? shaderInfo.name : shaderId
    }

    Image {
        id: image
        visible: shaderInfo ? true : false
        anchors { left: parent.left; top: label.bottom; margins: 8 }
        width: 256; height: width / 16 * 9
        fillMode: Image.PreserveAspectFit
        //source: "https://www.shadertoy.com/media/shaders/" + shaderId + ".jpg"
        source: "../../shadertoys/" + shaderId + ".jpg"
    }


    ShaderInfo {
        id: shaderInfoBox
        shaderInfo: root.shaderInfo
        visible: root.shaderInfo ? true : false;
        anchors { top: image.top; bottom: image.bottom; left: image.right; leftMargin: 4; right: parent.right; rightMargin: 8 }
    }
    BusyIndicator {
        anchors.centerIn: shaderInfoBox
        visible: shaderInfo ? false : true
    }
}
