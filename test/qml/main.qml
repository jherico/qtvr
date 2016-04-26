import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../../app/resources/qml"
import "../../app/resources/qml/windows"
import "../../app/resources/qml/dialogs"
import "../../app/resources/qml/shadertoy"

ApplicationWindow {
    id: offscreenWindow
    visible: true
    width: 1280
    height: 720
    title: qsTr("Scratch App")

    Settings {
        category: "MainTestWindow"
        property alias width: offscreenWindow.width
        property alias height: offscreenWindow.height
        property alias x: offscreenWindow.x
        property alias y: offscreenWindow.y
    }

    Component { id: listModelBuilder; ListModel{} }

    AppDesktop {
        id: desktop
        anchors.fill: parent
    }

    /*
    Rectangle {
        id: focusDebugger
        color: "#7500ffff"
    }

    onActiveFocusItemChanged: {
        var focusItem = offscreenWindow.activeFocusItem
        console.log("Focus item is " + focusItem);
        if (focusItem) {
            var r = desktop.mapFromItem(focusItem, 0, 0, focusItem.width, focusItem.height)
            focusDebugger.width = r.width
            focusDebugger.height = r.height
            focusDebugger.x = r.x
            focusDebugger.y = r.y
        }
    }
    */
}
