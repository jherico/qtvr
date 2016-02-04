import QtQuick 2.5

import "../desktop"
import "."
import "../controls"
import "../windows"

Desktop {
    id: desktop
    rootMenu: AppMenu { }
    property alias editor: editor;
    property alias browser: browser;
    property var cursorPosition;

    onCursorPositionChanged: {
//        cursor.x = cursorPosition.x;
//        cursor.y = cursorPosition.y;
    }

    signal loadShader(string shaderId);

    Shadertoy { id: shadertoy }

    MouseArea {
        z: 10000
        id: menuActivator;
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.RightButton
        onClicked: desktop.toggleMenu(Qt.vector2d(mouseX, mouseY));
    }

//    FontAwesome {
//        z: 10000
//        id: cursor
//        text: "\uf245"
//        size: 24
//        FontAwesome {
//            text: "\uf245"
//            color: "white"
//            size: parent.size * 0.7
//            x: 2
//            y: 4
//        }

//    }

    function setDisplayPlugins(plugins) {
        rootMenu.setDisplayPlugins(menu)
        for (var i = 0; i < plugins.length; ++i) {
            var menuItem = addMenuItem("Display", plugins[i]);
            console.log(menuItem);
        }
    }

    function addMenuItem(path, name) {
        var menu = getMenu(path);
        if (!menu) {
            return;
        }
        return menu.addItem(name);
    }

    function getMenu(path) {
        console.log("Requested menu for " + path);
        path = path.split(">");
        var items = rootMenu.menus;
        var result;
        for (var i = 0; i < path.length; ++i) {
            result = findMenu(items, path[i]);
            if (!result) {
                return;
            }
        }

        return result;
    }

    Editor {
        id: editor;
        visible: false;
    }

    function toggleEditor() {
        editor.visible = !editor.visible;
    }

    Browser {
        id: browser;
        visible: false;
        onSelectedShader: editor.loadShaderId(shaderId);
    }

    function toggleBrowser() {
        browser.visible = !browser.visible;
    }

    function findMenu(items, targetName) {
        for (var i = 0; i < items.length; ++i) {
            if (items[i].title && items[i].title === targetName) {
                return items[i];
            }
        }
    }

    Component {
        id: texturePickerBuilder;
        TexturePicker {
            onSelected: {
                console.log("Selected texture " + input + " for channel " + channel)
                console.log("Preview " + input.preview);
                editor.setInput(channel, input)

            }
        }
    }

    function pickTexture(channel) {
        texturePickerBuilder.createObject(desktop, { channel: channel });
    }


}
