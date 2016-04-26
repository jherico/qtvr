import QtQuick 2.0

Rectangle {
    anchors.fill: parent
    //visible: parent ? parent.focus || parent.activeFocus : false
    visible: false
    color: parent ? (parent.activeFocus ? "#7fff0000" : "#7fffff00") : "black"
}

