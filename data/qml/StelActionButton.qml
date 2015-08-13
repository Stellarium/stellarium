import QtQuick 2.5
import QtGraphicalEffects 1.0
import org.stellarium 1.0

Item {
    id: root
    property string image
    property string imageOff
    property string action
    property bool hovered: mouseArea.containsMouse
    property string tooltip: actionObj.text
    property string shortcut: actionObj.shortcut

    property StelAction actionObj: stelActionMgr.findAction(action)

    width: imageItem.width
    height: imageItem.height

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            actionObj.trigger();
        }
    }

    Image {
        id: imageItem
        source: root.image
        visible: !imageOffItem.visible
    }

    Image {
        id: imageOffItem
        source: root.imageOff
        visible: actionObj.checkable && !actionObj.checked
    }

    Image {
        anchors.fill: parent
        source: "qrc:///graphicGui/glow.png"
        visible: mouseArea.containsMouse
    }
}
