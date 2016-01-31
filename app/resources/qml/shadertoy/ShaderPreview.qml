import QtQuick 2.5
import QtQuick.Controls 1.4

import "."

Rectangle {
    id: root
    width: 256 + 16
    height: 256

    color: "white"
    border { width: 2; color: "black";  }
    radius: 4

    property var shaderId;
    property var shaderData;

    Component.onCompleted: {
        shadertoy.fetchShader(shaderId, function(newShaderData){
            if (!shaderId) {
                return;
            }
            root.shaderData = newShaderData;
        });
    }

    Image {
        id: image
        visible: root.shaderData ? true : false
        width: 256; height: 144
        anchors { top: parent.top; topMargin: 8; horizontalCenter: parent.horizontalCenter }
        fillMode: Image.PreserveAspectFit
        //source: "https://www.shadertoy.com/media/shaders/" + shaderId + ".jpg"
    }

    Item {
        anchors { top: image.bottom; bottom: parent.bottom; left: parent.left; right: parent.right; margins: 8; topMargin: 4 }
        BusyIndicator {
            anchors.centerIn: parent
            visible: shaderData ? false : true
        }

        Item {
            anchors.fill: parent
            visible: shaderData ? true : false

            Column {
                id: labelsColumn
                width: 48
                spacing: 2
                Text { text: "Name" }
                Text { text: "Author" }
            }
            Column {
                anchors { left: labelsColumn.right; right: parent.right; margins: 4 }
                spacing: 2
                Text { text: root.shaderData ? root.shaderData.Shader.info.name : "" }
                Text { text: root.shaderData ? root.shaderData.Shader.info.username : "" }
                TextArea {
                    anchors { left: parent.left; right: parent.right; }
                    height: 64; readOnly: true;  text: root.shaderData ? root.shaderData.Shader.info.description : ""
                }
            }
        }
    }
}
