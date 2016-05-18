import QtQuick 2.5
import QtQuick.Controls 1.4

import "."

FocusScope {
    id: root
    width: 640
    height: image.y + image.height + 12

    property var shaderId;
    property var shader;

    Rectangle  {
        anchors.fill: parent
        color: root.activeFocus ? "white" : "grey"
        border { width: 2; color: "black";  }
        radius: 4
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.focus = true
        }
    }

    function updateShader() {
        if (shaderId && !shader) {
            console.debug("Fetching shader information for " + shaderId);
            shadertoy.fetchShader(shaderId, function(shader) {
                console.log("Shader fetched for " + shaderId);
                root.shader = shader;
            });
        }
    }

    Component.onCompleted: updateShader();
    onShaderIdChanged: updateShader();
    onShaderChanged: {
        console.log("Shader is now " + shader);
    }

    Text {
        id: label
        anchors { left: parent.left; top: parent.top; margins: 4; leftMargin: 8}
        font.pointSize: 12
        font.bold: true
        text: shader ? shader.info.name : "Unknown"
    }

    Image {
        id: image
        anchors { left: parent.left; top: label.bottom; margins: 8 }
        width: 256; height: width / 16 * 9
        fillMode: Image.PreserveAspectFit
        source: "https://www.shadertoy.com/media/shaders/" + root.shaderId + ".jpg"

        BusyIndicator {
            anchors.centerIn: parent
            visible: image.status != Image.Ready
        }
    }


    ShaderInfo {
        id: shaderInfoBox
        shaderInfo: root.shader ? root.shader.info : null
        anchors { top: image.top; bottom: image.bottom; left: image.right; leftMargin: 4; right: parent.right; rightMargin: 8 }
    }

}
