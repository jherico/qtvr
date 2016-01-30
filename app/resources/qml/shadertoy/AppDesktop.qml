import QtQuick 2.5

import "../desktop"
import "."
import "../controls"

Desktop {
    id: desktop
    rootMenu: AppMenu { }

    MouseArea {
        z: 10000
        id: menuActivator;
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.RightButton
        onClicked: desktop.toggleMenu(Qt.vector2d(mouseX, mouseY));
        property real lastX
        property real lastY
        onMouseXChanged: cursor.x = mouseX
        onMouseYChanged: cursor.y = mouseY
    }

    FontAwesome {
        z: 10000
        id: cursor
        text: "\uf245"
        size: 32

    }

    Rectangle {
        id: focusDebugger;
        z: 9999; visible: true; color: "red"
        width: 100
        height: 100
        ColorAnimation on color { from: "#7fffff00"; to: "#7f0000ff"; duration: 1000; loops: 9999 }
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


    function findMenu(items, targetName) {
        for (var i = 0; i < items.length; ++i) {
            if (items[i].title && items[i].title === targetName) {
                return items[i];
            }
        }
    }
}
