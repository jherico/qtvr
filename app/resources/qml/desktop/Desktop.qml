import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs;

import "../dialogs"
import "../js/Utils.js" as Utils

// This is our primary 'desktop' object to which all VR dialogs and
// windows will be childed.
FocusScope {
    id: desktop
    anchors.fill: parent;
    objectName: "desktop"

    signal focusItemChanged(var item);

    readonly property int invalid_position: -9999;
    property rect recommendedRect: Qt.rect(0,0,0,0);
    property var expectedChildren;

    onHeightChanged: d.handleSizeChanged();
    
    onWidthChanged: d.handleSizeChanged();

    // Allows QML/JS to find the desktop through the parent chain
    property bool desktopRoot: true

    signal showDesktop();

    readonly property alias zLevels: zLevels
    QtObject {
        id: zLevels;
        readonly property real normal: 1
        readonly property real top: 2000
        readonly property real modal: 4000
        readonly property real menu: 8000
    }

    QtObject {
        id: d
        property var knownWindows: []

        Component.onCompleted: offscreenWindow.activeFocusItemChanged.connect(onWindowFocusChanged);

        function handleSizeChanged() {
            var oldRecommendedRect = recommendedRect;
            var newRecommendedRectJS = (typeof Controller === "undefined") ? Qt.rect(0,0,0,0) : Controller.getRecommendedOverlayRect();
            var newRecommendedRect = Qt.rect(newRecommendedRectJS.x, newRecommendedRectJS.y, 
                                    newRecommendedRectJS.width, 
                                    newRecommendedRectJS.height);

            var oldChildren = expectedChildren;
            var newChildren = d.getRepositionChildren();
            if (oldRecommendedRect != Qt.rect(0,0,0,0) 
                  && (oldRecommendedRect != newRecommendedRect
                      || oldChildren != newChildren)
                ) {
                expectedChildren = newChildren;
                d.repositionAll();
            }
            recommendedRect = newRecommendedRect;
        }

        function findChild(item, name) {
            for (var i = 0; i < item.children.length; ++i) {
                if (item.children[i].objectName === name) {
                    return item.children[i];
                }
            }
            return null;
        }

        function findParentMatching(item, predicate) {
            while (item) {
                if (predicate(item)) {
                    break;
                }
                item = item.parent;
            }
            return item;
        }

        function isTopLevelWindow(item) {
            return item.topLevelWindow;
        }

        function isAlwaysOnTopWindow(window) {
            return window.alwaysOnTop;
        }

        function isModalWindow(window) {
            return window.modality !== Qt.NonModal;
        }

        function getTopLevelWindows(predicate) {
            var currentWindows = [];
            if (!desktop) {
                console.log("Could not find desktop for " + item)
                return currentWindows;
            }

            for (var i = 0; i < desktop.children.length; ++i) {
                var child = desktop.children[i];
                if (isTopLevelWindow(child) && (!predicate || predicate(child))) {
                    currentWindows.push(child)
                }
            }
            return currentWindows;
        }

        function getDesktopWindow(item) {
            return findParentMatching(item, isTopLevelWindow)
        }

        function getTopLevelWindow(item) {
            return findParentMatching(item, isTopLevelWindow)
        }

        function fixupZOrder(windows, basis, topWindow) {
            windows.sort(function(a, b){ return a.z - b.z; });

            if ((topWindow.z >= basis)  &&  (windows[windows.length - 1] === topWindow)) {
                return;
            }

            var lastZ = -1;
            var lastTargetZ = basis - 1;
            for (var i = 0; i < windows.length; ++i) {
                var window = windows[i];
                if (!window.visible) {
                    continue
                }

                if (topWindow && (topWindow === window)) {
                    continue
                }

                if (window.z > lastZ) {
                    lastZ = window.z;
                    ++lastTargetZ;
                }
                if (DebugQML) {
                    console.log("Assigning z order " + lastTargetZ + " to " + window)
                }

                window.z = lastTargetZ;
            }
            if (topWindow) {
                ++lastTargetZ;
                if (DebugQML) {
                    console.log("Assigning z order " + lastTargetZ + " to " + topWindow)
                }
                topWindow.z = lastTargetZ;
            }

            return lastTargetZ;
        }

        function raiseWindow(targetWindow) {
            var predicate;
            var zBasis;
            if (isModalWindow(targetWindow)) {
                predicate = isModalWindow;
                zBasis = zLevels.modal
            } else if (isAlwaysOnTopWindow(targetWindow)) {
                predicate = function(window) {
                    return (isAlwaysOnTopWindow(window) && !isModalWindow(window));
                }
                zBasis = zLevels.top
            } else {
                predicate = function(window) {
                    return (!isAlwaysOnTopWindow(window) && !isModalWindow(window));
                }
                zBasis = zLevels.normal
            }

            var windows = getTopLevelWindows(predicate);
            fixupZOrder(windows, zBasis, targetWindow);
        }

        function removeWindow(targetWindow) {
            var index = knownWindows.indexOf(targetWindow);
            if (index === -1) {
                console.warn("Attempted to remove unknown window " + targetWindow);
                return;
            }
        }

        function onWindowFocusChanged() {
            focusItemChanged(offscreenWindow.activeFocusItem);
        }

        function getRepositionChildren(predicate) {
            var currentWindows = [];
            if (!desktop) {
                console.log("Could not find desktop");
                return currentWindows;
            }

            for (var i = 0; i < desktop.children.length; ++i) {
                var child = desktop.children[i];
                if (child.shouldReposition === true && (!predicate || predicate(child))) {
                    currentWindows.push(child)
                }
            }
            return currentWindows;
        }

        function repositionAll() {
            var oldRecommendedRect = recommendedRect;
            var oldRecommendedDimmensions = { x: oldRecommendedRect.width, y: oldRecommendedRect.height };
            var newRecommendedRect = Controller.getRecommendedOverlayRect();
            var newRecommendedDimmensions = { x: newRecommendedRect.width, y: newRecommendedRect.height };
            var windows = d.getTopLevelWindows();
            for (var i = 0; i < windows.length; ++i) {
                var targetWindow = windows[i];
                if (targetWindow.visible) {
                    repositionWindow(targetWindow, true, oldRecommendedRect, oldRecommendedDimmensions, newRecommendedRect, newRecommendedDimmensions);
                }
            }

            // also reposition the other children that aren't top level windows but want to be repositioned
            var otherChildren = d.getRepositionChildren();
            for (var i = 0; i < otherChildren.length; ++i) {
                var child = otherChildren[i];
                repositionWindow(child, true, oldRecommendedRect, oldRecommendedDimmensions, newRecommendedRect, newRecommendedDimmensions);
            }

        }
    }

    function addWindow(newWindow) {
        console.log("New window created " + newWindow);
        if (-1 !== d.knownWindows.indexOf(newWindow)) {
            console.warn("Window was already registered!")
            return;
        }

        d.knownWindows.push(newWindow);
        newWindow.windowDestroyed.connect(function(){
            d.removeWindow(newWindow);
        })
    }

    function raise(item) {
        var targetWindow = d.getTopLevelWindow(item);
        if (!targetWindow) {
            console.warn("Could not find top level window for " + item);
            return;
        }

        // Fix up the Z-order (takes into account if this is a modal window)
        d.raiseWindow(targetWindow);
        var setFocus = true;
        if (!d.isModalWindow(targetWindow)) {
            var modalWindows = d.getTopLevelWindows(d.isModalWindow);
            if (modalWindows.length) {
                setFocus = false;
            }
        }

        if (setFocus) {
            focus = true;
        }

        showDesktop();
    }

    function centerOnVisible(item) {
        var targetWindow = d.getDesktopWindow(item);
        if (!targetWindow) {
            console.warn("Could not find top level window for " + item);
            return;
        }

        if (typeof Controller === "undefined") {
            console.warn("Controller not yet available... can't center");
            return;
        }

        var newRecommendedRectJS = (typeof Controller === "undefined") ? Qt.rect(0,0,0,0) : Controller.getRecommendedOverlayRect();
        var newRecommendedRect = Qt.rect(newRecommendedRectJS.x, newRecommendedRectJS.y, 
                                newRecommendedRectJS.width, 
                                newRecommendedRectJS.height);
        var newRecommendedDimmensions = { x: newRecommendedRect.width, y: newRecommendedRect.height };
        var newX = newRecommendedRect.x + ((newRecommendedRect.width - targetWindow.width) / 2);
        var newY = newRecommendedRect.y + ((newRecommendedRect.height - targetWindow.height) / 2);
        targetWindow.x = newX;
        targetWindow.y = newY;

        // If we've noticed that our recommended desktop rect has changed, record that change here.
        if (recommendedRect != newRecommendedRect) {
            recommendedRect = newRecommendedRect;
        }

    }

    function repositionOnVisible(item) {
        var targetWindow = d.getDesktopWindow(item);
        if (!targetWindow) {
            console.warn("Could not find top level window for " + item);
            return;
        }

        if (typeof Controller === "undefined") {
            console.warn("Controller not yet available... can't reposition targetWindow:" + targetWindow);
            return;
        }


        var oldRecommendedRect = recommendedRect;
        var oldRecommendedDimmensions = { x: oldRecommendedRect.width, y: oldRecommendedRect.height };
        var newRecommendedRect = Controller.getRecommendedOverlayRect();
        var newRecommendedDimmensions = { x: newRecommendedRect.width, y: newRecommendedRect.height };
        repositionWindow(targetWindow, false, oldRecommendedRect, oldRecommendedDimmensions, newRecommendedRect, newRecommendedDimmensions);
    }

    function repositionWindow(targetWindow, forceReposition, 
                    oldRecommendedRect, oldRecommendedDimmensions, newRecommendedRect, newRecommendedDimmensions) {

        if (desktop.width === 0 || desktop.height === 0) {
            return;
        }

        if (!targetWindow) {
            console.warn("Could not find top level window for " + item);
            return;
        }

        var recommended = Controller.getRecommendedOverlayRect();
        var maxX = recommended.x + recommended.width;
        var maxY = recommended.y + recommended.height;
        var newPosition = Qt.vector2d(targetWindow.x, targetWindow.y);

        // if we asked to force reposition, or if the window is completely outside of the recommended rectangle, reposition it
        if (forceReposition || (targetWindow.x > maxX || (targetWindow.x + targetWindow.width) < recommended.x) ||
            (targetWindow.y > maxY || (targetWindow.y + targetWindow.height) < recommended.y))  {
            newPosition.x = -1
            newPosition.y = -1
        }


        if (newPosition.x === -1 && newPosition.y === -1) {
            var originRelativeX = (targetWindow.x - oldRecommendedRect.x);
            var originRelativeY = (targetWindow.y - oldRecommendedRect.y);
            if (isNaN(originRelativeX)) {
                originRelativeX = 0;
            }
            if (isNaN(originRelativeY)) {
                originRelativeY = 0;
            }
            var fractionX = Utils.clamp(originRelativeX / oldRecommendedDimmensions.x, 0, 1);
            var fractionY = Utils.clamp(originRelativeY / oldRecommendedDimmensions.y, 0, 1);
            var newX = (fractionX * newRecommendedDimmensions.x) + newRecommendedRect.x;
            var newY = (fractionY * newRecommendedDimmensions.y) + newRecommendedRect.y;
            newPosition = Qt.vector2d(newX, newY);
        }

        newPosition = Utils.clampVector(Qt.vector2d(targetWindow.x, targetWindow.y), minPosition, maxPosition);
        targetWindow.x = newPosition.x;
        targetWindow.y = newPosition.y;
    }


    Component { id: messageDialogBuilder; MessageDialog { } }
    function messageBox(properties) {
        return messageDialogBuilder.createObject(desktop, properties);
    }

    Component { id: queryDialogBuilder; QueryDialog { } }
    function queryBox(properties) {
        return queryDialogBuilder.createObject(desktop, properties);
    }

    function unfocusWindows() {
        var windows = d.getTopLevelWindows();
        for (var i = 0; i < windows.length; ++i) {
            windows[i].focus = false;
        }
        desktop.focus = true;
    }
}



