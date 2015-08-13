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

    Component.onCompleted: {
        if (actionObj == null) console.warn("Cannot find action '%1'".arg(action));
    }

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
