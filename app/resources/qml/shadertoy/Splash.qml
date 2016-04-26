import QtQuick 2.5
import QtQuick.Controls 1.4

import "../windows"

CenteredWindow {
    id: splash
    width: 640; height: 480
    closable: false
    readonly property int startupTime: 1000;

    Item {
        anchors.fill: parent

        Timer {
            id: browserTimer
            interval: splash.startupTime
            running: true
            repeat: false
            onTriggered: {
                splash.visible = false;
                desktop.toggleBrowser();
            }
        }
        

        Item {
            anchors { top: parent.top; left: parent.left; right: parent.right; bottom: statusText.top; }
            Text {
                id: titleText
                text: "ShadertoyVR"
                font.pointSize: 32
                anchors { centerIn: parent; }
            }
        }

        Text {
            id: statusText
            text: "Let's get busy..."
            font.pointSize: 24
            anchors { horizontalCenter: parent.horizontalCenter; bottom: progress.top; margins: 8; }
        }

        ProgressBar {
            id: progress
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 8; }
            value: 0.0
            Timer {
                property int count: 0
                interval: 100
                running: splash.visible
                repeat: true
                onTriggered: {
                    ++count;
                    console.log("Count " + count)
                    progress.value = (count * 100) / (splash.startupTime - 200);
                    console.log(progress.value);
                }
            }
        }

    }

}
