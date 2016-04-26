import QtQuick 2.5

import ".."

PathView {
    id: root
    highlightRangeMode: PathView.StrictlyEnforceRange
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    property var subItems: []
    property var currentChild
    property var targetParent
    property real overdraw: 0.0

    signal childOpened()
    signal restore()
    signal pressed(var index)

    path: Path {
        startX: 0 - (overdraw * root.width); startY: root.height / 2.0
        PathAttribute { name: "selectedValue"; value: 0.0 }
        PathLine { x: root.width * 0.4; y: root.height / 2.0; }
        PathAttribute { name: "selectedValue"; value: 0.0 }
        PathLine { x: root.width * 0.5; y: root.height / 2.0; }
        PathAttribute { name: "selectedValue"; value: 1.0 }
        PathLine { x: root.width * 0.6; y: root.height / 2.0; }
        PathAttribute { name: "selectedValue"; value: 0.0 }
        PathLine { x: root.width + (overdraw * root.width); y: root.height / 2.0; }
        PathAttribute { name: "selectedValue"; value: 0.0 }
    }

    delegate: CrossBarItem {
        //text: name
        //icon: icon
        onPressed: {
            if (root.currentIndex === index) {
                root.pressed(index)
            } else {
                root.gotoIndex(index);
            }
        }
    }

    highlight: Rectangle {
        color: "lightsteelblue"
        smooth: true
        radius: 5
        visible: root.activeFocus
        width: root.currentItem ? root.currentItem.width * 1.2 : 0
        height: root.currentItem ? root.currentItem.height * 1.2 : 0
    }

    Keys.onLeftPressed: decrementCurrentIndex()
    Keys.onRightPressed: incrementCurrentIndex()
    Keys.onReturnPressed: root.pressed(root.currentIndex)
    Keys.onPressed: {
        switch (event.key) {
            case Qt.Key_A:
                decrementCurrentIndex();
                event.accepted = true;
                break;
            case Qt.Key_D:
                incrementCurrentIndex();
                event.accepted = true;
                break;
        }
    }

    SequentialAnimation {
        id: anim
        running: false
        NumberAnimation { id: firstAnim; target: root; property: "offset"; duration: 300 }
        NumberAnimation { id: secondAnim; target: root; property: "offset"; duration: 300 }
    }

    // Force animation when using the mouse to click to an item
    function gotoIndex(idx) {
        anim.running = false;
        var cur = root.offset;
        positionViewAtIndex(idx, PathView.Center);
        var dest = root.offset;
        var dist = Math.abs(cur - dest);
        var count = count + 0.0;
        var halfCount = count / 2.0;
        if (true) {
            console.log("One Step Animation " + cur + " -> " + dest)
            firstAnim.from = cur;
            firstAnim.to = dest;
            firstAnim.duration = 300;
            secondAnim.from = dest;
            secondAnim.to = dest;
            secondAnim.duration = 0;
            anim.running = true;
        } else {
            console.log("Two Step Animation " + cur + " -> " + dest)
            firstAnim.from = cur;
            firstAnim.to = cur < halfCount ? -0.5 : count + 0.5;
            firstAnim.duration = 150;
            secondAnim.from = cur < halfCount ? count + 0.5 : -0.5;
            secondAnim.to = dest;
            secondAnim.duration = 150;
            anim.running = true;
        }
    }

    onPressed: {
        if (subItems.length > index) {
            currentChild = subItems[index].createObject(targetParent);
            currentChild.crossBar = root
            currentChild.visible = true
            currentChild.forceActiveFocus()
            childOpened()
        }
    }

    onRestore: {
        if (currentChild) {
            currentChild.destroy()
            currentChild = null
        }

        if (currentItem) {
            currentItem.forceActiveFocus()
        }
    }

    onCountChanged: {
        gotoIndex(0)
    }

    FocusMarker {}
}
