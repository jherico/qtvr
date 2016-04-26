import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0
import QtWebEngine 1.0

import "../windows"
import "../controls"
import "."

CenteredWindow {
    id: root
    width: 900
    height: 640
    objectName: "Browser"
    closable: false

    signal selectedShader(string shaderId);

    property bool queryActive: false
    property bool ready: false
    property string query: ""
    property string filter: ""
    property string sort: "popular"
    property string count: "25"

    Settings {
        category: "BrowserWindow"
        property alias count: root.count
        property alias filter: root.filter
        property alias sort: root.sort
        property alias query: root.query

        Component.onCompleted: {
            filterSelection.currentIndex = filterSelection.model.indexOf(filter);
            sortSelection.currentIndex = sortSelection.model.indexOf(sort);
            countSelection.currentIndex = countSelection.model.indexOf(count);
            root.searchField.text = root.query;

            root.filter = Qt.binding(function() { return filterSelection.currentText });
            root.sort = Qt.binding(function() { return sortSelection.currentText });
            root.count = Qt.binding(function() { return countSelection.currentText });
            root.query = Qt.binding(function() { return searchField.text });
            root.ready = true;
        }
    }

    onCountChanged: updateResults();
    onSortChanged: updateResults();
    onFilterChanged: updateResults();

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
                onTextChanged: { refreshTimer.interval = 1500;  refreshTimer.restart(); }
                onAccepted: { refreshTimer.interval = 100;  refreshTimer.restart(); }
                Timer {
                    id: refreshTimer
                    interval: 1500; repeat: false; running: false
                    onTriggered: updateResults();
                }

                KeyNavigation.tab: sortSelection
                KeyNavigation.right: sortSelection
                KeyNavigation.left: countSelection
                KeyNavigation.down: flow
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

                    KeyNavigation.tab: filterSelection
                    KeyNavigation.right: filterSelection
                    KeyNavigation.left: searchField
                    KeyNavigation.down: flow
                }

                Item { height: 1; width: 8 }

                Text { text: "Filter"; anchors.verticalCenter: parent.verticalCenter }
                ComboBox {
                    id: filterSelection
                    width: 100
                    model: shadertoy.filterFields
                    KeyNavigation.tab: countSelection
                    KeyNavigation.right: countSelection
                    KeyNavigation.left: sortSelection
                    KeyNavigation.down: flow
                }


                Item { height: 1; width: 8 }

                Text { text: "Results"; anchors.verticalCenter: parent.verticalCenter }
                ComboBox {
                    id: countSelection
                    width: 48
                    model: [ "5", "25", "50", "100" ]
                    KeyNavigation.tab: searchField
                    KeyNavigation.right: searchField
                    KeyNavigation.left: filterSelection
                    KeyNavigation.down: flow
                }
            }

            ScrollView {
                clip: true;
                anchors { top: searchField.bottom; topMargin: 8; bottom: parent.bottom; left: parent.left; right: parent.right }
                id: flowContainer
                ListView {
                    id: flow
                    model: desktop.shaderModel
                    delegate: ShaderPreview {
                        width: flow.width
                        shaderId: desktop.shaderModel[index]
                        Component.onCompleted: console.log("Shader preview for id " + shaderId);
                        MouseArea {
                            anchors.fill: parent;
                            onDoubleClicked: {
                                root.selectedShader(parent.shaderId);
                                root.visible = false;
                            }
                        }
                    }

                    KeyNavigation.right: countSelection
                    KeyNavigation.left: searchField
                }
            }
        }
    }

    function updateResults() {
        if (!ready || queryActive) {
            return;
        }
        console.log("Update results start");
        var params = {};
        params["sort"] = sort;
        params["num"] = count;
        if (filter) { params["filter"] = filter }
        shadertoy.queryShaders(query, params, function(shaderIds){
            queryActive = false
            desktop.shaderModel = shaderIds || [];
        });
    }
}
