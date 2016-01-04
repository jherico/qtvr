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
}
