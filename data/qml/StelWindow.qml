/*
 * Stellarium
 * Copyright (C) 2015 Guillaume Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

import QtQuick 2.4
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0

Item {
    id: root
    property string title: qsTr("Stellarium")
    property var target
    property string property
    property int minimumWidth: 0
    property int minimumHeight: 0

    visible: root.target[root.property]
    // implicitWidth: childrenRect.width
    // height: childrenRect.height
    default property alias contents: contentItem.children

    MouseArea {
        width: 16
        height: 16
        property var startPos
        property var startSize
        anchors {right: contentItem.right; bottom: contentItem.bottom}
        cursorShape: Qt.SizeFDiagCursor
        onPressed: {
            startPos = [x + mouse.x, y + mouse.y];
            startSize = [root.width, root.height]
        }
        onPositionChanged: {
            root.width = Math.max(startSize[0] + x + mouse.x - startPos[0], root.minimumWidth);
            root.height = Math.max(startSize[1] + y + mouse.y - startPos[1], root.minimumHeight);
        }
    }

    StelGradient {
        id: header
        height: 30
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        c0: "#292f32"
        c1: "#1f2124"
        Text {
            anchors.centerIn: parent
            text: root.title
                color: "white"
        }
        MouseArea {
            anchors.fill: parent
            drag.target: root
        }

        Image {
            id: test
            source: "qrc:///graphicGui/closeButton.png"
            smooth: true
            visible: true
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
                margins: 5
            }
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {root.target[root.property] = false}
            }
        }
    }

    StelGradient {
        id: contentItem
        c0: "#56575a"
        c1: "#303134"
        anchors {
            top: header.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        // height: childrenRect.height
        // width: childrenRect.width
    }
}
