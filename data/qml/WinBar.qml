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
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4

Item {
    id: winBar
    property bool up: true

    width: up ? 75 : 0
    height: column.height + 100

    anchors {
        bottom: parent.bottom
        left: parent.left
        leftMargin: up ? -10 : -10
    }

    Column {
        id: column
        height: childrenRect.height
        width: childrenRect.width
        spacing: 4
        anchors {
            right: parent.right
            top: parent.top
            margins: 8
        }

        Repeater {
            id: groupItem
            property var buttons: bars.getButtons("win-bar")
            model: buttons
            delegate: StelActionButton {
                image: groupItem.buttons[index].pixOn
                imageOff: groupItem.buttons[index].pixOff
                action: groupItem.buttons[index].action
                onHoveredChanged: {
                    winBar.setTooltip(hovered ? "%1 [%2]".arg(tooltip).arg(shortcut)
                                              : "", y + width / 2);
                }
            }
        }
    }

    function setTooltip(txt, y) {
        tooltipItem.y = y;
        tooltipItem.text = txt;
    }

    Text {
        id: tooltipItem
        color: "white"
        visible: text != ""
        x: parent.x + parent.width + 20
    }
}
