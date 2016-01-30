import QtQuick 2.5
import QtQuick.Controls 1.4 as Original

import "."
import ".."

Original.MenuBar {
    id: root
    property var actionBuilder: Component { Action { } }
    property var itemBuilder: Component { MenuItem { } }
    property var menuBuilder: Component { Original.Menu { } }

    function findMatchingElement(collection, predicate) {
        for (var i = 0; i < collection.length; ++i) {
            var item = collection[i];
            if (predicate(item)) {
                return item;
            }
        }
    }

    function findMatchingItem(collection, name) {
        return findMatchingElement(collection, function(item) {
            return (item.type === Original.MenuItemType.Item) &&
                    ((item.text === name) || (item.action.text === name));
        });
    }

    function findMatchingMenu(collection, name) {
        return findMatchingElement(collection, function(item) {
            return item.type === Original.MenuItemType.Menu && item.title === name;
        });
    }

    function findMenu(path) {
        path = path.split(">");
        var items = menus;
        var menu;
        for (var i = 0; i < path.length; ++i) {
            var pathElement = path[i];
            menu = findMatchingMenu(items, pathElement);
            if (!menu) { return; }
            items = menu.items;
        }
        return menu;
    }

    function findItem(path, itemName) {
        var menu = findMenu(path);
        if (!menu) { return; }
        var item = findMatchingItem(menu.items, itemName);
        if (!item) {
            return;
        }
        return item.action;
    }

    function findItemByNameRecursive(collection, itemName) {
        var result = findMatchingItem(collection, itemName);
        if (result) { return result.action; }

        for (var i = 0; i < collection.length; ++i) {
            var menuChild = collection[i];
            if (menuChild.type === Original.MenuItemType.Menu) {
                result = findItemByNameRecursive(menuChild.items, itemName);
                if (result) { return result; }
            }
        }
    }

    function findItemByName(itemName) {
        return findItemByNameRecursive(menus, itemName);
    }

    function itemExists(path, itemName) {
        var item = findItem(path, itemName);
        if (!item) {
            return false;
        }
        return true;
    }

    function createItem(parent, properties) {
        var action = actionBuilder.createObject(parent, properties)
        var menuItem = itemBuilder.createObject(parent, { action: action });
        parent.insertItem(parent.items.length, menuItem);
        return action;
    }

    function getChildren() {
        var result = [];
        for (var i = 0; i < menus.length; ++i) {
            result.push(menus[i]);
        }
        return result;
    }
}

