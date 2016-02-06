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
    property var shader;

    function updateShader() {
        if (shaderId && !shader) {
            console.debug("Fetching shader information for " + shaderId);
            shadertoy.api.fetchShader(root.shaderId, function(shader) {
                console.log("Shader fetched for " + shaderId);
                root.shader = shader;
            });
        }
    }

    Component.onCompleted: updateShader();
    onShaderIdChanged: updateShader();

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
        //source: "https://www.shadertoy.com/media/shaders/" + root.shaderId + ".jpg"
        source: "file:///c:/Users/bdavis/git/shadertoys/" + shaderId + ".jpg"

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
