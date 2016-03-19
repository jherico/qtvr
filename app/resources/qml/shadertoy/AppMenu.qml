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

    MenuItem {
        id: editorItem
        action: Action {
            checkable: true
            checked: desktop.editor.visible
            text: "Editor"
            shortcut: "`"
            onTriggered: desktop.toggleEditor()
        }
    }

    MenuItem {
        id: browserItem
        action: Action {
            checkable: true
            checked: desktop.browser.visible
            text: "Browser"
            onTriggered: desktop.toggleBrowser()
        }
    }

    MenuItem {
        id: statsItem
        action: Action {
            checkable: true
            checked: desktop.stats.visible
            text: "Stats"
            onTriggered: desktop.toggleStats()
        }
    }

    Menu {
        id: displayMenu
        title: "Display"
        ExclusiveGroup {
            id: displayPluginsGroup
            onCurrentChanged: root.activateDisplayPlugin(displayPluginsGroup.current.text);
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
