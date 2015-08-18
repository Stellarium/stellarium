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
import QtQuick.Layouts 1.1

ColumnLayout {
    id: root
    property double value
    signal newValue(double newValue)
    function getRepr(v) {return v}

    MouseArea {
        height: 20
        Layout.fillWidth: true
        Image {
            anchors.centerIn: parent
            source: "qrc:///graphicGui/spinup.png"
        }
        onClicked: {root.newValue(value + 1)}
    }
    
    Item {
        Layout.fillWidth: true
        implicitWidth: 200
        implicitHeight: 20

        TextInput {
            anchors.fill: parent
            text: root.getRepr(root.value)
            horizontalAlignment: TextInput.AlignHCenter
        }
    }

    MouseArea {
        height: 20
        Layout.fillWidth: true
        Image {
            anchors.centerIn: parent
            source: "qrc:///graphicGui/spindown.png"
        }
        onClicked: {root.newValue(value - 1)}
    }
}
