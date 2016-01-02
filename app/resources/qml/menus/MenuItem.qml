import QtQuick 2.5
import QtQuick.Controls 1.4

MenuItem {
    visible: action.advanced ? rootMenu.advancedMenusEnabled : 
        action.developer ? rootMenu.developerMenusEnabled : true
}

