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
    width: 1920
    height: 1080
    title: qsTr("Scratch App")
    menuBar: desktop.rootMenu

    Component { id: listModelBuilder; ListModel{} }

    AppDesktop {
        id: desktop
        anchors.fill: parent

        Editor {

        }


    }
}
