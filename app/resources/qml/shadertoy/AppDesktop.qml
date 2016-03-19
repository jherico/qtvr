import QtQuick 2.5
import QtQuick.Controls 1.4

import "../desktop"
import "."
import "../controls"
import "../windows"

Desktop {
    id: desktop
    rootMenu: AppMenu { }
    property alias editor: editor;
    property alias browser: browser;
    property vector2d cursorPosition: Qt.vector2d(0, 0);
    property var currentShader;

    signal updateCode(string code);

    onCursorPositionChanged: {
        cursorImage.x = cursorPosition.x;
        cursorImage.y = cursorPosition.y;
    }

    Shadertoy { id: shadertoy }

    MouseArea {
        z: 10000
        id: menuActivator;
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.RightButton
        onClicked: desktop.toggleMenu(Qt.vector2d(mouseX, mouseY));
    }

    FontAwesome {
        z: 10000
        id: cursorImage
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

    Editor {
        id: editor;
        visible: false;
    }

    function toggleEditor() {
        editor.visible = !editor.visible;
    }


    function loadShaderId(shaderId) {
        console.log("Shader load request for editor " + shaderId)
        shadertoy.api.fetchShader(shaderId, loadShader)
        //root.visible = true;
    }

    function loadShader(shader) {
        console.log("Got shader data " + shader )
        currentShader = shader;
        renderer.setShader(shader);
        renderer.build();
    }

    Stats {
        id: stats;
        visible: false;
    }

    function toggleStats() {
        stats.visible = !stats.visible;
    }

    Browser {
        id: browser;
        visible: false;
        onSelectedShader: desktop.loadShaderId(shaderId);
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
        TexturePicker { onSelected: editor.setInput(channel, input) }
    }

    function pickTexture(channel) {
        texturePickerBuilder.createObject(desktop, { channel: channel });
    }

    Action {
        id:  toggleBrowserAction
        shortcut: "Ctrl+B"
        onTriggered: toggleBrowser();
    }

    Action {
        id:  toggleEditorAction
        shortcut: "Ctrl+E"
        onTriggered: toggleEditor();
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        property point cursor: Qt.point(0, 0);
        onMouseXChanged: {
            cursor.x = mouseX;
            renderer.mouseMoved(cursor);
        }
        onMouseYChanged: {
            cursor.y = mouseY
            renderer.mouseMoved(cursor);
        }
        onPressed: {
            cursor = Qt.point(mouseX, mouseY);
            renderer.mousePressed(cursor);
        }
        onReleased: {
            cursor = Qt.point(0, 0);
            renderer.mouseReleased();
        }

    }

    Keys.onPressed: {
        renderer.keyPressed(event.key);
        event.accepted = false;
    }

    Keys.onReleased: {
        renderer.keyReleased(event.key);
        event.accepted = false;
    }

    focus: true

//    onKeyEvent: {
//        console.log("App desktop got key" + event.key);
//    }
}
