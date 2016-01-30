import QtQuick 2.5
import QtQuick.Controls 1.4

Menu {

    function getChildren() {
        var result = [];
        for (var i = 0; i < items.length; ++i) {
            result.push(items[i]);
        }
        return result;
    }
}

