import QtQuick 2.5
import QtQuick.Controls 1.4

import "."

Item {
    id: root
    anchors.fill: parent

    MouseArea {
        id: menuRoot;
        anchors.fill: parent
        enabled: d.topMenu !== null
        onClicked: {
            d.clearMenus();
        }
    }

    QtObject {
        id: d
        property var menuStack: []
        property var topMenu: null;
        property var modelMaker: Component { ListModel { } }
        property var menuViewMaker: Component {
            VrMenuView {
                id: subMenu
                onSelected: d.handleSelection(subMenu, currentItem, item)
            }
        }

        function toModel(items) {
            var result = modelMaker.createObject(desktop);
            for (var i = 0; i < items.length; ++i) {
                var item = items[i];
                if (!item.visible) continue;
                switch (item.type) {
                case MenuItemType.Menu:
                    result.append({"name": item.title, "item": item})
                    break;
                case MenuItemType.Item:
                    result.append({"name": item.text, "item": item})
                    break;
                case MenuItemType.Separator:
                    result.append({"name": "", "item": item})
                    break;
                }
            }
            return result;
        }

        function popMenu() {
            if (menuStack.length) {
                menuStack.pop().destroy();
            }
            if (menuStack.length) {
                topMenu = menuStack[menuStack.length - 1];
                topMenu.focus = true;
            } else {
                topMenu = null;
                offscreenFlags.navigationFocused = false;
                menuRoot.enabled = false;
            }
        }

        function pushMenu(newMenu) {
            menuStack.push(newMenu);
            topMenu = newMenu;
            topMenu.focus = true;
            offscreenFlags.navigationFocused = true;
        }

        function clearMenus() {
            while (menuStack.length) {
                popMenu()
            }
        }

        function clampMenuPosition(menu) {
            var margins = 0;
            if (menu.x < margins) {
                menu.x = margins
            } else if ((menu.x + menu.width + margins) > root.width) {
                menu.x = root.width - (menu.width + margins);
            }

            if (menu.y < 0) {
                menu.y = margins
            } else if ((menu.y + menu.height + margins) > root.height) {
                menu.y = root.height - (menu.height + margins);
            }
        }

        function buildMenu(items, targetPosition) {
            var model = toModel(items);
            // Menu's must be childed to desktop for Z-ordering
            var newMenu = menuViewMaker.createObject(desktop, { model: model, z: topMenu ? topMenu.z + 1 : desktop.zLevels.menu });
            if (targetPosition) {
                newMenu.x = targetPosition.x
                newMenu.y = targetPosition.y - newMenu.height / 3 * 1
            }
            clampMenuPosition(newMenu);
            pushMenu(newMenu);
            return newMenu;
        }

        function handleSelection(parentMenu, selectedItem, item) {
            while (topMenu && topMenu !== parentMenu) {
                popMenu();
            }

            switch (item.type) {
                case MenuItemType.Menu:
                    var target = Qt.vector2d(topMenu.x, topMenu.y).plus(Qt.vector2d(selectedItem.x + 96, selectedItem.y));
                    buildMenu(item.items, target).objectName = item.title;
                    break;

                case MenuItemType.Item:
                    console.log("Triggering " + item.text)
                    item.trigger();
                    clearMenus();
                    break;
                }
        }

    }

    function popup(parent, items, point) {
        d.clearMenus();
        menuRoot.enabled = true;
        d.buildMenu(items, point);
    }

    function toggle(parent, items, point) {
        if (d.topMenu) {
            d.clearMenus();
            return;
        }
        popup(parent, items, point);
    }

    function closeLastMenu() {
        if (d.menuStack.length) {
            d.popMenu();
            return true;
        }
        return false;
    }

}
