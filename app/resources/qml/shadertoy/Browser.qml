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

    property var currentResults: [];

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

                Button { text: "Fetch All";  onClicked: refreshAll(); }
            }

            ScrollView {
                clip: true;
                anchors { top: searchField.bottom; topMargin: 8; bottom: parent.bottom; left: parent.left; right: parent.right }
                id: flowContainer
                Flow { id: grid; spacing: 4; width: flowContainer.width * 0.95; }
            }
        }

        Component { id: shaderPreviewBuilder; ShaderPreview {} }
        //Component { id: flowBuilder;  }

        Timer {
            id: fetchTimer;
            running: false
            repeat: true
            interval: 500
            onTriggered: fetchSomeShaders();
        }
    }

    property var refreshAllList: [];

    function fetchSomeShaders() {
        if (refreshAllList.length === 0) {
            console.log("Fetching done");
            fetchTimer.stop();
            return;
        }

        for (var i = 0; i < 10; ++i) {
            if (refreshAllList.length) {
                fetchSingleShader(refreshAllList[0]);
                console.log(refreshAllList[0]);
                refreshAllList.shift();
            }
        }
        console.log(refreshAllList.length + " items remaining");
    }

    function fetchSingleShader(shaderId) {
        console.log("Requesting item " + shaderId);
        shadertoy.fetchShader(shaderId, function(newShaderData){
            console.log("Persisted shader " + shaderId);
        });
    }

    function refreshAll() {
        shadertoy.fetchShaderList(function(shaderList){
//            var newIds = shaderList.Results;
//            for (var i = 0; i < newIds.length; ++i) {
//                var shaderId = newIds[i];
//                refreshAllList.push(shaderId);
//            }
//            fetchTimer.start();
        })
    }

    function updateResults() {
        if (!ready) {
            return;
        }

        clearCurrentResults();
        var params = {};
        params["sort"] = sort;
        params["num"] = count;
        if (filter) { params["filter"] = filter }
        shadertoy.searchShaders(query, params, processResults);
        console.log("Update results");
    }

    function clearCurrentResults() {
        grid.children = [];
        while (currentResults.length) {
            var item = currentResults[0];
            item.parent = null;
            item.destroy();
            currentResults.pop();
        }
    }

    function processResults(json) {
        console.log("Results");
        clearCurrentResults();
        var newIds = json.Results;
        for (var i = 0; i < newIds.length; ++i) {
            var shaderId = newIds[i];
            var newPreview = shaderPreviewBuilder.createObject(grid, { shaderId: shaderId });
            currentResults.push(newPreview);
        }
    }
}
