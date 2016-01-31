import QtQuick 2.5

import "../desktop"
import "."
import "../controls"
import "../windows"

Desktop {
    id: desktop
    rootMenu: AppMenu { }
    property var editor;

    Shadertoy {
        id: shadertoy
    }

    MouseArea {
        z: 10000
        id: menuActivator;
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.RightButton
        onClicked: desktop.toggleMenu(Qt.vector2d(mouseX, mouseY));
        property real lastX
        property real lastY
        onMouseXChanged: cursor.x = mouseX;
        onMouseYChanged: cursor.y = mouseY
    }

    FontAwesome {
        z: 10000
        id: cursor
        text: "\uf245"
        size: 24
        FontAwesome {
            text: "\uf245"
            color: "white"
            size: parent.size * 0.8
            x: parent.size * 0.1
            y: parent.size * 0.1
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

    function findMenu(items, targetName) {
        for (var i = 0; i < items.length; ++i) {
            if (items[i].title && items[i].title === targetName) {
                return items[i];
            }
        }
    }
}
