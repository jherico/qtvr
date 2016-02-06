import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0
import QtWebEngine 1.0

import "../windows"
import "../controls"
import "."

Window {
    id: root
    width: 800
    height: 600
    destroyOnCloseButton: false
    destroyOnInvisible: false
    resizable: true
    objectName: "Editor"
    visible: false

    readonly property string prefix: "../.."
    property var currentInputs: [ shadertoy.misc[0], shadertoy.misc[0], shadertoy.misc[0], shadertoy.misc[0] ]
    property var channelImages: [ textureImage0, textureImage1, textureImage2, textureImage3 ]

    Connections {
        target: desktop
        onCurrentShaderChanged: {
            console.log("Current shader changed, setting text");
            editor.setText(currentShader.renderpass[0].code)
        }
    }

    function setInput(channel, input) {
        console.log("Setting channel " + channel + " to input " + input.src)
        currentInputs[channel] = input;
        console.log("Channel " + channel + " source is now " + currentInputs[channel].src)
        console.log(channelImages[channel]);
        channelImages[channel].source = prefix + currentInputs[channel].src;
    }

    function runShader() {
        console.log("Running shader")
        editor.getText(function(text){
            desktop.updateCode(text);
            renderer.build();
        });
    }

    Item {
        anchors.fill: parent;
        QtObject {
            id: d
            property bool running: true
            readonly property var themes: [
                "ambiance",
                "chaos",
                "chrome",
                "clouds",
                "clouds_midnight",
                "cobalt",
                "crimson_editor",
                "dawn",
                "dreamweaver",
                "eclipse",
                "github",
                "idle_fingers",
                "iplastic",
                "katzenmilch",
                "kr_theme",
                "kuroir",
                "merbivore",
                "merbivore_soft",
                "mono_industrial",
                "monokai",
                "pastel_on_dark",
                "solarized_dark",
                "solarized_light",
                "sqlserver",
                "terminal",
                "textmate",
                "tomorrow",
                "tomorrow_night",
                "tomorrow_night_blue",
                "tomorrow_night_bright",
                "tomorrow_night_eighties",
                "twilight",
                "vibrant_ink",
                "xcode"
            ]
        }


        Timer {
            interval: 5000
            repeat: true
            onTriggered: editor.getText(function(text){ currentShaderText = text; });
        }

        Row {
            id: buttonRow
            anchors { top: parent.top; left: parent.left; margins: 8 }
            spacing: 8
            FontAwesome {
                size: 32
                text: "\uf04b"
                color: "black"
                MouseArea {
                    anchors.fill: parent;
                    onClicked: runShader();
                }
            }

            ComboBox {
                id: themeSelector
                height: parent.height
                width: 128
                model: d.themes
            }

            SpinBox {
                id: fontSizeSpinner
                height: parent.height
                minimumValue: 6
                maximumValue: 32
            }

        }

        Settings {
            category: "EditorWindow"
            property alias x: root.x
            property alias y: root.y
            property alias width: root.width
            property alias height: root.height
            property alias textSize: editor.fontSize
            property alias theme: editor.theme
            //property alias currentText: root.currentShaderText
        }

        Text {
            id: editor
            property real fontSize;
            property string theme;
            anchors { top: buttonRow.bottom; left: parent.left; right: textureColumn.left; bottom: parent.bottom; margins: 8 }
            function getText(f) {
                f(editor.text);
            }

            function setText(content) {
                editor.text = content;
            }
        }

        /*
        WebEngineView {
            id: editor
            anchors { top: buttonRow.bottom; left: parent.left; right: textureColumn.left; bottom: parent.bottom; margins: 8 }
            url: "Editor.html"
            property var content: Renderer.currentShader
            property string theme: d.themes[0];
            property real fontSize: 8;
            property bool ready: false

            Component.onCompleted: {
                themeSelector.currentIndex = d.themes.indexOf(theme);
                theme = Qt.binding(function() { return themeSelector.currentText });
                fontSizeSpinner.value = fontSize;
                fontSize = Qt.binding(function() { return fontSizeSpinner.value });
            }

            function getText(f) {
                editor.runJavaScript("editor.getValue()", f);
            }

            function setText(content) {
                var escapedShaderString = content.replace(/\n/g, "\\n")
                                                  .replace(/\'/g, "\\'")
                                                  .replace(/\"/g, '\\"')
                                                  .replace(/\&/g, "\\&")
                                                  .replace(/\r/g, "\\r")
                                                  .replace(/\t/g, "\\t")
                                                  .replace(/\f/g, "\\f");
                editor.runJavaScript("editor.setValue(\"" + escapedShaderString + "\")");
            }

            onJavaScriptConsoleMessage: {
                console.log("Console message from page: " + level + ", " + sourceID + ", " + lineNumber + ": " + message);
            }

            onContentChanged: setText();
            onFontSizeChanged: updateAce();
            onThemeChanged: updateAce();

            function updateAce() {
                if (!ready) {
                    return;
                }

                console.log("Updating editor state");
                editor.runJavaScript("editor.setTheme('ace/theme/" + theme + "')");
                editor.runJavaScript("editor.setFontSize(" + fontSize + " )");
            }

            onLoadingChanged: {
                console.log("Loading changed to " + loading);
                if (!loading) {
                    ready = true;
                    setText();
                    updateAce();
                }
            }

            Timer {
                id: loadTimer;
                running: false
                repeat: false
                interval: 50
                onTriggered: {
                    if (!editor.loading) {
                        editor.updateShaderEditor();
                        editor.setTheme(themeSelector.currentText);
                        editor.setFontSize(fontSizeSpinner.value);
                    } else {
                        console.log("Editor load status " + editor.loading )
                        console.log("Editor HTML not loaded, triggering timer again")
                        start();
                    }
                }
            }
        }
        */

        Column {
            id: textureColumn
            anchors { right: parent.right; }
            spacing: 8

            TextureImage {
                id: textureImage0
                source: currentInputs[0].preview
                channel: 0
            }
            TextureImage {
                id: textureImage1
                source: currentInputs[1].preview
                channel: 1
            }
            TextureImage {
                id: textureImage2
                source: currentInputs[2].preview
                channel: 2
            }
            TextureImage {
                id: textureImage3
                source: currentInputs[3].preview
                channel: 3
            }
        }
    }
}
