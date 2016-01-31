import QtQuick 2.4
import QtQuick.Controls 1.3

Menu {
    id: root
    objectName: "rootMenu"
    Menu {
        title: "File"
        MenuItem {
            text: "Quit"
            shortcut: "Ctrl+Q"
            onTriggered: Qt.quit()
        }
    }
    Menu {
        title: "Edit"

        MenuItem {
            text: "Cut"
            shortcut: "Ctrl+X"
            onTriggered: console.log("Cut")
        }

        MenuItem {
            text: "Copy"
            shortcut: "Ctrl+C"
            onTriggered: console.log("Copy")
        }

        MenuItem {
            text: "Paste"
            shortcut: "Ctrl+V"
            onTriggered: console.log("Paste")
        }

        MenuSeparator { }

        Menu {
            title: "More Stuff"

            MenuItem {
                text: "Do Nothing"
                onTriggered: console.log("Nothing")
            }
        }
    }

    MenuItem {
        id: editorItem
        action: Action {
            id: editorAction
            text: "Toggle Editor"
            shortcut: "`"
            onTriggered: desktop.toggleEditor()
        }
    }

    MenuItem {
        id: browserItem
        action: Action {
            text: "Browser"
            onTriggered: desktop.toggleBrowser()
        }
    }

    Menu {
        id: displayMenu
        title: "Display"
        ExclusiveGroup {
            id: displayPluginsGroup
            onCurrentChanged: {
                console.log("Current item " + displayPluginsGroup.current)
                console.log("Current item " + displayPluginsGroup.current.text)
                root.activateDisplayPlugin(displayPluginsGroup.current.text);
            }
        }
    }

    function buildDisplayPluginMenuItem(name, active) {
        var menuItem = displayMenu.addItem(name);
        menuItem.exclusiveGroup = displayPluginsGroup;
        menuItem.checkable = true;
        menuItem.checked = active
    }

    function setDisplayPlugins(plugins, activePlugin) {
        for (var i = 0; i < plugins.length; ++i) {
            console.log(plugins[i]);
            buildDisplayPluginMenuItem(plugins[i], plugins[i] === activePlugin);
        }
    }

    signal activateDisplayPlugin(string name);
}
