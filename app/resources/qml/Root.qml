import Hifi 1.0
import QtQuick 2.5
import QtQuick.Controls 1.4

// This is our primary 'window' object to which all dialogs and controls will 
// be childed. 
Root {
    id: root
    anchors.fill: parent
    
    RootMenu {}
    
    Row {
        spacing: 8
        Text { 
            text: "File"
            font.pointSize: 24
        }
        Text { 
            text: "Edit" 
            font.pointSize: 24
        }
    }
    
    Rectangle {
        anchors.fill: parent
        color: "#7fff0000"
    }
    
    Rectangle {
        width: 50
        height: 50
        y: 20
        color: "red"
        ColorAnimation on color {
            id: animation
            from: "red" 
            to: "yellow"; 
            duration: 1000
            loops: Animation.Infinite
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                animation.from = "blue"
                console.log("Click")
            }
        }
    }

    Rectangle {
        width: 50
        height: 50
        x: parent.width - 50
        y: parent.height - 50
        color: "red"
        ColorAnimation on color { 
            from: "red" 
            to: "yellow"; 
            duration: 1000
            loops: Animation.Infinite
        }
    }

    onParentChanged: {
        forceActiveFocus();
    }
}