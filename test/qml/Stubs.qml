import QtQuick 2.5
import QtQuick.Controls 1.4

// Stubs for the global service objects set by Interface.cpp when creating the UI
// This is useful for testing inside Qt creator where these services don't actually exist.
Item {

    Item {
        objectName: "Renderer"
        property string currentShader: "Hello world"
    }

    Item {
        objectName: "offscreenFlags"
        property bool navigationFocused: false
    }
}

