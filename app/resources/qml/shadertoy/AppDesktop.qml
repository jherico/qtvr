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

    onFocusItemChanged: if (item === desktop) { restScopeTimer.start(); }

    Browser {
        id: browser;
        visible: false;
        onSelectedShader: desktop.loadShaderId(shaderId);
    }
    
    Shadertoy { id: shadertoy }

    Splash { id: splash }

    onBrowserChanged: { console.log("Browser: " + browser) }

    /*
    Button {
        onClicked: toggleBrowser();
        text: "Browser"
    }
    */
    
    function loadShaderId(shaderId) {
        shadertoy.fetchShader(shaderId, loadShader)
    }

    function loadShader(shader) {
        currentShader = shader;
        selectedShader(shader);
        // Fixme, use a signal instead
        if (renderer) {
            renderer.setShader(shader);
            renderer.build();
        }
    }


    FocusScope {
        id: restScope
        focus: true

        // Works in VR and in Qt Creator
        Keys.onPressed: {
            switch (event.key) {
                case Qt.Key_Escape:
                case Qt.Key_Return:
                case Qt.Key_Enter:
                    desktop.toggleBrowser();
                    break;
                default: break;
            }
        }

        Timer {
            id: restScopeTimer
            interval: 100
            repeat: false
            onTriggered: { restScope.focus = true; restScope.forceActiveFocus(); }
        }
    }


    function toggleBrowser() {
        console.log("browser: " + browser);
        browser.visible = !browser.visible;
    }
}
