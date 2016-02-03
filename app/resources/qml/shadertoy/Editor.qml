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
    property var currentShader;
    property var channelImages: [
        channel0image,
        channel1image,
        channel2image,
        channel3image
    ]

    function loadShaderId(shaderId) {
        console.log("Shader load request for editor " + shaderId)
        shadertoy.api.fetchShader(shaderId, loadShader)
    }

    function loadShader(shader) {
        console.log("Got shader data " + shader )
        if (shader.renderpass.length != 1) {
            console.warn("Multiple render passes not supported");
            return;
        }

        var pass = shader.renderpass[0];
        for (var i =0; i < pass.inputs.length; ++i) {
            var input = pass.inputs[i];
            if (input.ctype !== "texture" && input.ctype !== "cubemap") {
                console.warn("Unsupported input type " + input.ctype);
                return;
            }
        }

        console.log("Loading text and image");

        for (i = 0; i < 4; ++i) {
            channelImages[i].source = ""
        }

        editor.setText(pass.code);
        for (i =0; i < pass.inputs.length; ++i) {
            input = pass.inputs[i];
            channelImages[input.channel].source = "../../" + input.src;
        }
    }

    function runShader() {
        editor.getText(function(text){
            console.log("Setting shader source");
            renderer.updateShaderSource(text);
//            renderer.updateShaderInput(int channel, const QVariant& input);
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

        Settings {
            category: "EditorWindow"
            property alias x: root.x
            property alias y: root.y
            property alias width: root.width
            property alias height: root.height
            property alias textSize: editor.fontSize
            property alias theme: editor.theme
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

        Component {
            id: textureViewBuilder
            Item {
                width: 128; height: 128
            }
        }

        Component { id: pickerMaker; TexturePicker {} }

        Column {
            id: textureColumn
            anchors { right: parent.right; }
            spacing: 8

            Image {
                id: channel0image
                width: 128; height: 128;
                source: "../../presets/cube00_0.jpg"
                MouseArea {
                    anchors.fill: parent;
                    onClicked: pickerMaker.createObject(desktop);
                }
                Rectangle {
                    z: -1
                    anchors.fill: parent
                    color: "#00000000"
                    border.width: 4
                    border.color: "black"
                    radius: 8
                }
            }
            Image {
                id: channel1image
                width: 128; height: 128;
                source: "../../presets/tex00.jpg"
            }
            Image {
                id: channel2image
                width: 128; height: 128;
                source: "../../presets/tex10.png"
            }
            Image {
                id: channel3image
                width: 128; height: 128;
                source: "../../presets/cube01_0.png"
            }
        }
    }
}
