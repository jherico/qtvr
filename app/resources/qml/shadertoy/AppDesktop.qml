import QtQuick 2.5
import QtQuick.Controls 1.4

import "../desktop"
import "."

Desktop {
    id: desktop

    signal updateCode(string code);
    signal selectedShader(var shader);

    property alias browser: browser;
    property alias splash: splash
    property var currentShader;
    property var shaderModel: []

    Browser {
        id: browser;
        visible: false;
        onSelectedShader: desktop.loadShaderId(shaderId);
    }
    
    Shadertoy { id: shadertoy }

    Splash { id: splash }

    onBrowserChanged: { console.log("Browser: " + browser) }

    Button {
        onClicked: toggleBrowser();
        text: "Browser"
    }
    
    function loadShaderId(shaderId) {
        shadertoy.fetchShader(shaderId, loadShader)
    }

    function loadShader(shader) {
        console.log("Got shader data " + shader )
        currentShader = shader;
        selectedShader(shader);
        //renderer.setShader(shader);
        //renderer.build();
    }


    function toggleBrowser() {
        console.log("browser: " + browser);
        browser.visible = !browser.visible;
    }

    Action {
        id:  toggleBrowserAction
        shortcut: "Ctrl+B"
        onTriggered: toggleBrowser();
    }

    /*
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        property point cursor: Qt.point(0, 0);
        onMouseXChanged: {
            cursor.x = mouseX;
            renderer.mouseMoved(cursor);
        }
        onMouseYChanged: {
            cursor.y = mouseY
            renderer.mouseMoved(cursor);
        }
        onPressed: {
            cursor = Qt.point(mouseX, mouseY);
            renderer.mousePressed(cursor);
        }
        onReleased: {
            cursor = Qt.point(0, 0);
            renderer.mouseReleased();
        }
    }
    Keys.onPressed: {
        console.log("Pressed " + event.key);
        renderer.keyPressed(event.key);
        event.accepted = false;
    }

    Keys.onReleased: {
        console.log("Released " + event.key);
        renderer.keyReleased(event.key);
        event.accepted = false;
    }
    */


    focus: true
}
