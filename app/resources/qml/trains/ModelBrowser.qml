import QtQuick 2.5
import QtQuick.Controls 1.4

import "../windows"
import "../controls/crossbar"

CenteredWindow {
    width: 720
    height: 480

    Rectangle {
        color: "white"
        anchors.fill: parent


        CrossBar {
            height: 32
            anchors { top: parent.top; left: parent.left; right: parent.right; margins: 16 }
            model: ListModel {
                ListElement { name: "Tracks" }
                ListElement { name: "Buildings" }
                ListElement { name: "Houses" }
            }
            focus: true
        }

        Item {
            anchors.fill: parent
            anchors.margins: 16
            anchors.topMargin: 48

            Rectangle {
                id: modelPreview
                color: "red"
                width: 540 - 32
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
            }

            Rectangle {
                color: "blue"
                anchors { left: modelPreview.right; leftMargin: 16; right: parent.right; top: parent.top; bottom: parent.bottom }
            }
        }
    }
}
