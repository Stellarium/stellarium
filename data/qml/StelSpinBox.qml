import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

StelGradient {
    id: root
    property real value: 0
    property real stepSize: 1.0
    property real minimumValue
    property real maximumValue

    implicitWidth: 100
    implicitHeight: 24
    c0: "#b4b4b7"
    c1: "#5d5f62"
    border.color: "black"

    function getRepr(v) {return "" + v.toFixed(1)}
    function getValue(s) {return +s}
    function getDelta(s) {return root.stepSize}

    function changeValue(d) {
        var s = textInputItem.text.substring(0, textInputItem.cursorPosition);
        var delta = getDelta(s);

        if (delta > 0)
            value += d * delta;
        else
            value *= delta;

        // Update selection to reflect the delta.
        var start = 0;
        while (getDelta(textInputItem.text.substring(0, start)) != delta) {
            start++;
        }
        var end = start;
        while (getDelta(textInputItem.text.substring(0, end)) == delta) {
            end++;
            var sub = textInputItem.text.substring(start, end);
            if (isNaN(sub + '0')) {
                end--;
                break;
            }
            if (end == textInputItem.text.length) break;
        }
        textInputItem.select(start, end);
        textInputItem.forceActiveFocus();
    }

    TextInput {
        id: textInputItem
        selectByMouse: true
        anchors.fill: parent
        anchors.rightMargin: 8 + 22
        text: root.getRepr(root.value)
        horizontalAlignment: TextEdit.AlignRight
        verticalAlignment: TextEdit.AlignVCenter
        onAccepted: {root.value = root.getValue(text)}
    }

    StelGradient {
        c0: "#61615c"
        c1: "#4a4a4b"
        height: 12
        width: 22
        anchors {top: parent.top; right: parent.right; margins: 1}
        Image {
            anchors.centerIn: parent
            source: "qrc:///graphicGui/spinup.png";
        }

        MouseArea {
            id: mouseUp
            anchors.fill: parent
            onClicked: root.changeValue(+1)
            property bool autoincrement: false;
            onReleased: autoincrement = false
            Timer { running: mouseUp.pressed; interval: 350 ; onTriggered: mouseUp.autoincrement = true }
            Timer { running: mouseUp.autoincrement; interval: 60 ; repeat: true ; onTriggered: root.changeValue(+1) }
        }
    }

    StelGradient {
        c0: "#444445"
        c1: "#282828"
        height: 12
        width: 22
        anchors {bottom: parent.bottom; right: parent.right; margins: 1}
        Image {
            anchors.centerIn: parent
            source: "qrc:///graphicGui/spindown.png";
        }

        MouseArea {
            id: mouseDown
            anchors.fill: parent
            onClicked: root.changeValue(-1)
            property bool autoincrement: false;
            onReleased: autoincrement = false
            Timer { running: mouseDown.pressed; interval: 350 ; onTriggered: mouseDown.autoincrement = true }
            Timer { running: mouseDown.autoincrement; interval: 60 ; repeat: true ; onTriggered: root.changeValue(-1) }
        }
    }

}
