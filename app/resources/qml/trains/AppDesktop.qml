import QtQuick 2.5
import QtQuick.Controls 1.4

import "../desktop"
import "."

Desktop {
    id: desktop
    Button {
        onClicked: { console.log("Test") }
        text: "Browser"
    }

    ModelBrowser {

    }

    focus: true
}
