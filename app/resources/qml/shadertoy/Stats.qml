import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0
import QtWebEngine 1.0

import "../windows"
import "../controls"
import "."

Window {
    id: root
    width: 320
    height: 240
    destroyOnCloseButton: false
    destroyOnInvisible: false
    objectName: "Stats"

    Settings {
        category: "StatsWindow"
        property alias x: root.x
        property alias y: root.y
    }

    Rectangle {
        anchors.fill: parent;
        color: "white"

        Item {
            id: content
            anchors { fill: parent; margins: 8 }

            Column {
                id: col
                property int valuesX: 128
                anchors.fill: parent
                Item {
                    Rectangle {
                        anchors.fill: parent
                        color: "#7fff0000"
                    }
                    height: scaleLabel.height
                    width: parent.width
                    Text {
                        id: scaleLabel
                        text: "Scale: "
                    }
                    Text {
                        x: col.valuesX
                        text: renderer.scale.toFixed(2);
                    }
                }
                Item {
                    height: scaleLabel.height
                    width: parent.width
                    Text { text: "FPS: " }
                    Text { x: col.valuesX; text: renderer.fps.toFixed(1); }
                }
                Item {
                    height: scaleLabel.height
                    width: parent.width
                    Text { text: "Pixels: " }
                    Text { x: col.valuesX; text: renderer.pixels + " mps"; }
                }

                Item {
                    height: scaleLabel.height
                    width: parent.width
                    Text { text: "GPU time: " }
                    Text { x: col.valuesX; text: renderer.gpuFrameTime + " µSecs"; }
                }

            }
        }
    }
}
