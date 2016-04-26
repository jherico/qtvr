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

    function selectShader(shaderId) {
        selectedShader(shaderId);
        visible = false;
    }

    property bool queryActive: false
    property bool ready: false
    property string query: ""
    property string filter: "vr"
    property string sort: "newest"
    property string count: "Unlimited"

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
            searchField.text = root.query;

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
    onVisibleChanged: {
        if (visible) {
            if ((!desktop.shaderModel || desktop.shaderModel.length == 0)) {
                updateResults();
            }
            flow.focus = true;
            flow.forceActiveFocus();
        }
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
                    model: [ "5", "25", "100", "Unlimited" ]
                    KeyNavigation.tab: searchField
                    KeyNavigation.right: searchField
                    KeyNavigation.left: filterSelection
                    KeyNavigation.down: flow
                }
            }

            Item {
                id: flowContainer
                clip: true
                anchors { top: searchField.bottom; topMargin: 8; bottom: parent.bottom; left: parent.left; right: parent.right }

                ListView {
                    id: flow
                    anchors.fill: parent
                    model: desktop.shaderModel
                    highlightMoveDuration: 10
                    onCurrentIndexChanged: flow.positionViewAtIndex(currentIndex, ListView.Contain)
                    delegate: ShaderPreview {
                        id: preview
                        width: flow.width
                        shaderId: desktop.shaderModel[index]
                        focus: flow.currentIndex === index
                        Component.onCompleted: console.log("Shader preview for id " + shaderId + " " + index);
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                preview.focus = true;
                                preview.forceActiveFocus();
                                flow.currentIndex = index;
                            }
                            onDoubleClicked: selectShader(parent.shaderId);
                        }
                    }
                    Keys.onReturnPressed: selectShader(flow.currentItem.shaderId);
                    Keys.onEscapePressed: root.visible = false;
                    KeyNavigation.right: countSelection
                    KeyNavigation.left: searchField
                    Keys.onPressed: {
                        switch (event.key) {
                            case Qt.Key_End: flow.currentIndex = desktop.shaderModel.length - 1; break;
                            case Qt.Key_Home: flow.currentIndex = 0; break;
                            case Qt.Key_PageDown: Math.min(desktop.shaderModel.length - 1, flow.currentIndex += 5); break;
                            case Qt.Key_PageUp: Math.max(0, flow.currentIndex -= 5); break;
                            default: break;
                        }
                    }
                }
            }

            //NumberAnimation { id: anim; target: flow; property: "contentY"; duration: 250 }
        }
    }

    function updateResults() {
        if (!ready || queryActive) {
            return;
        }
        console.log("Update results start");
        var params = {};
        params["sort"] = sort;
        if (count != "Unlimited") {
            params["num"] = count;
        }
        if (filter) { params["filter"] = filter }

        shadertoy.queryShaders(query, params, function(shaderIds){
            queryActive = false
            desktop.shaderModel = shaderIds || [];
        });
    }
}
