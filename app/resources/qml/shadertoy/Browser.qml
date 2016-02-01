import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0
import QtWebEngine 1.0

import "../windows"
import "../controls"
import "."

Window {
    id: root
    width: 900
    height: 640
    destroyOnCloseButton: false
    destroyOnInvisible: false
    resizable: true
    objectName: "Browser"

    property bool ready: false
    property string query: ""
    property string filter: ""
    property string sort: "popular"
    property string count: "25"

    Component.onCompleted: {
        filterSelection.currentIndex = filterSelection.model.indexOf(filter);
        filter = Qt.binding(function() { return filterSelection.currentText });
        sortSelection.currentIndex = sortSelection.model.indexOf(sort);
        sort = Qt.binding(function() { return sortSelection.currentText });
        countSelection.currentIndex = countSelection.model.indexOf(count);
        count = Qt.binding(function() { return countSelection.currentText });
        searchField.text = query;
        query = Qt.binding(function() { return searchField.text });
        ready = true;
        updateResults();
    }

    onCountChanged: updateResults();
    onSortChanged: updateResults();
    onFilterChanged: updateResults();

    Settings {
        category: "BrowserWindow"
        property alias x: root.x
        property alias y: root.y
        property alias count: root.count
        property alias filter: root.filter
        property alias sort: root.sort
        property alias query: root.query
    }

    Rectangle {
        anchors.fill: parent;
        color: "white"

        Item {
            id: content
            anchors { fill: parent; margins: 8 }

            TextField {
                id: searchField
                anchors { top: parent.top; left: parent.left; right: queryParams.left; rightMargin: 16 }
                placeholderText: "Search"
                onTextChanged: refreshTimer.restart();
                onAccepted: { refreshTimer.stop(); root.updateResults(); }
                Timer {
                    id: refreshTimer
                    interval: 500; repeat: false; running: false
                    onTriggered: updateResults();
                }
            }

            Row {
                anchors { top: parent.top; right: parent.right; }
                id: queryParams
                spacing: 4

                Text { text: "Sort"; anchors.verticalCenter: parent.verticalCenter }
                ComboBox {
                    id: sortSelection
                    width: 64
                    model: shadertoy.sortFields
                }

                Item { height: 1; width: 8 }

                Text { text: "Filter"; anchors.verticalCenter: parent.verticalCenter }
                ComboBox {
                    id: filterSelection
                    width: 100
                    model: shadertoy.filterFields
                }

                Item { height: 1; width: 8 }

                Text { text: "Results"; anchors.verticalCenter: parent.verticalCenter }
                ComboBox {
                    id: countSelection
                    width: 48
                    model: [ "5", "25", "50", "100" ]
                    currentIndex: 1
                }
            }

            ScrollView {
                clip: true;
                anchors { top: searchField.bottom; topMargin: 8; bottom: parent.bottom; left: parent.left; right: parent.right }
                id: flowContainer


                Column {
                    id: flow;
                    spacing: 4;
                    width: flowContainer.width * 0.98;
                    Component {
                        id: shaderPreviewBuilder;
                        ShaderPreview {
                            anchors { left: parent.left; right: parent.right }
                            MouseArea {
                                anchors.fill: parent;
                                onDoubleClicked: desktop.loadShader(shaderId);
                            }
                        }
                    }
                    property var shaderChildren: [];

                    function setShaders(shaderIds) {
                        clearShaders();
                        for (var i = 0; i < shaderIds.length; ++i) {
                            shaderChildren.push(shaderPreviewBuilder.createObject(flow, { shaderId: shaderIds[i] }));
                        }

                    }

                    function clearShaders() {
                        while (shaderChildren.length) {
                            var item = shaderChildren[0];
                            shaderChildren.shift();
                            item.parent = null;
                            item.destroy();
                        }
                    }
                }
            }
        }
    }

    function updateResults() {
        if (!ready) {
            return;
        }

        console.log("Attempting to update results")
        flow.clearShaders();
        var params = {};
        params["sort"] = sort;
        params["num"] = count;
        if (filter) { params["filter"] = filter }
        shadertoy.api.queryShaders(query, params, processResults);
        console.log("Update results");
    }

    function processResults(shaderIds) {
        flow.setShaders(shaderIds);
    }
}
