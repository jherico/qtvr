import QtQuick 2.5
import ".."

FocusScope {
    id: root
    anchors.fill: parent
    property var crossBar: null
    property var delegate;
    property var currentItem;
    property var name;
    property var labelDelegate: Component {
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: currentItem.y - height - 8
            font.bold: true
            font.family: mainFont.name
            font.pointSize: 24
            text: name
        }
    }

    Component.onCompleted: {
        if (delegate) {
            currentItem = delegate.createObject(closeArea)
            if (currentItem && name) {
                labelDelegate.createObject(root)
            }
        }
    }

    MouseArea {
        id: closeArea
        anchors.fill: parent
        onClicked: root.closeChild()
    }

    function closeChild() {
        crossBar.restore()
    }


    Keys.onLeftPressed: {
        crossBar.decrementCurrentIndex();
        root.closeChild();
    }

    Keys.onRightPressed: {
        crossBar.incrementCurrentIndex();
        root.closeChild();
    }

    Keys.onPressed: {
        switch(event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Backspace:
                closeChild();
                event.accepted = true
                break
        }
    }

    FocusMarker{}
}
