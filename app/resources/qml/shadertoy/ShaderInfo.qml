import QtQuick 2.5
import QtQuick.Controls 1.4

import "."

Item {
    id: root
    property string shaderId;
    property var shaderInfo;
    visible: shaderInfo ? true : false
    property string tags: "";

    onShaderInfoChanged: {
        if (shaderInfo) {
            for (var tag in shaderInfo.tags) {
                if (tags) {
                    tags += ", "
                }
                tags += shaderInfo.tags[tag];
            }
        }
    }

    //            "id": "MddGzf",
    //            "date": "1451884800",
    //            "viewed": 10566,
    //            "name": "Bricks Game",
    //            "username": "iq",
    //            "description": "Use arrow keys or the mouse to move the paddle. Use space to restart. This shader uses the new Multipass system",
    //            "likes": 111,

    Column {
        id: labelsColumn
        width: 48
        spacing: 4
        Text { text: "Author" }
        Text { text: "Date" }
        Text { text: "Likes" }
        Text { text: "Views" }
    }
    Column {
        id: valuesColumn
        anchors { left: labelsColumn.right; right: description.left;  margins: 4 }
        spacing: labelsColumn.spacing
        Text {  text: shaderInfo ? shaderInfo.username : "" }
        Text {  text: shaderInfo ? shaderInfo.date : "" }
        Text {  text: shaderInfo ? shaderInfo.likes : "" }
        Text {  text: shaderInfo ? shaderInfo.viewed : "" }
    }

    Text {
        anchors.top: labelsColumn.bottom; anchors.topMargin:  labelsColumn.spacing;
        anchors.left: labelsColumn.left;
        anchors.right: labelsColumn.right;
        text: "Tags"
    }

    TextArea {
        anchors.top: labelsColumn.bottom; anchors.topMargin:  labelsColumn.spacing;
        anchors.left: valuesColumn.left;
        anchors.right: valuesColumn.right;
        anchors.bottom: parent.bottom;
        id: tagsValue;
        text: tags;
        readOnly: true
    }

    TextArea {
        id: description
        anchors { right: parent.right; leftMargin: 4; rightMargin: 8; top:  parent.top; bottom: parent.bottom; }
        width: 256
        readOnly: true;  text: shaderInfo ? shaderInfo.description : ""
    }
}
