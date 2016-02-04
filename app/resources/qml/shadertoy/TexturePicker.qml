import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0
import QtWebEngine 1.0

import "../windows"
import "../controls"
import "."

ModalWindow {
    id: root
    width: 660
    height: 480
    resizable: true
    property int channel;
    signal selected(int channel, var input)

    function pickTexture(channel) {
        currentChannel = channel;
        root.visible = true;
    }

    Rectangle {
        id: content
        anchors.fill: parent;

        Settings {
            category: "PickerWindow"
            property alias width: root.width
            property alias height: root.height
        }

        Component {
            id: textureSelectBuilder
            Item {
                implicitHeight: image.height
                implicitWidth: image.width
                Image {
                    id: image
                    source: "../.." + modelData.preview
                    MouseArea {
                        id: imageMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.selected(root.channel, modelData);
                            root.visible = false;
                        }
                    }
                }
            }
        }



        ScrollView {
            id: scrollView;
            clip: true
            anchors { top: parent.top; left: parent.left; right: parent.right; bottom: cancelButton.top; margins: 8 }

            Column {
                x: 16
                id: column
                width: scrollView.width * 0.9

                Repeater {
                    model: shadertoy.inputs
                    Item {
                        width: parent.width
                        height: label.height + flow.height + 32
                        anchors { left: parent.left; right: parent.right; }
                        Text { id: label; text: modelData.name }
                        Flow {
                            id: flow; spacing: 8
                            anchors { left: parent.left; right: parent.right; top: label.bottom; topMargin: 8 }
                            Repeater { model: modelData.textures; delegate: textureSelectBuilder }
                        }
                    }

                }
            }
        }

        Button {
            id: cancelButton
            anchors { bottom: parent.bottom; right: parent.right; margins: 8 }
            text: "Cancel";
            onClicked: root.destroy()
        }
    }
}
