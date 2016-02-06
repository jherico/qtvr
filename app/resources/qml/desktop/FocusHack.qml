import QtQuick 2.5

FocusScope {
    id: root

    TextInput {
        id: textInput;
        focus: true
        width: 10; height: 10
        onActiveFocusChanged: {
            console.log("Got focus");
            root.destroy()
        }
    }

    Timer {
        id: focusTimer
        running: false
        interval: 100
        onTriggered: {
            console.log("FocusHack forcing focus to input");
            textInput.forceActiveFocus()
        }
    }

    function start() {
        focusTimer.running = true;
    }
}



