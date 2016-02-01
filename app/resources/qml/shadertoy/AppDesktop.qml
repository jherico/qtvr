import QtQuick 2.5

import "../desktop"
import "."
import "../controls"
import "../windows"

Desktop {
    id: desktop
    rootMenu: AppMenu { }
    property var editor;
    property var browser;
    property var cursorPosition;
    property var shadertoyCache;

    onCursorPositionChanged: {
        cursor.x = cursorPosition.x;
        cursor.y = cursorPosition.y;
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
//        onMouseXChanged: cursor.x = mouseX;
//        onMouseYChanged: cursor.y = mouseY
    }

    FontAwesome {
        z: 10000
        id: cursor
        text: "\uf245"
        size: 24
        FontAwesome {
            text: "\uf245"
            color: "white"
            size: parent.size * 0.7
            x: 2
            y: 4
        }

    }

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

    Component { id: editorBuilder; Editor { } }
    function toggleEditor() {
        if (!editor) {
            editor = editorBuilder.createObject(desktop);
            editor.visible = true;
        } else {
            editor.visible = !editor.visible;
        }
    }

    Component { id: browserBuilder; Browser { } }
    function toggleBrowser() {
        if (!browser) {
            browser = browserBuilder.createObject(desktop);
            browser.visible = true;
        } else {
            browser.visible = !browser.visible;
        }
    }

    function findMenu(items, targetName) {
        for (var i = 0; i < items.length; ++i) {
            if (items[i].title && items[i].title === targetName) {
                return items[i];
            }
        }
    }


}
