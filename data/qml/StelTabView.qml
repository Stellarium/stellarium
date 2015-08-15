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
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0

TabView {
    id: root

    height: childrenRect.height

    style: TabViewStyle {
        frameOverlap: 1
        tabBar: Rectangle { color: "#313131" }

        tab: Rectangle {
            property bool hasIcon: root.getTab(styleData.index).icon
            color: styleData.selected ? "#56575a" : "#313131"
            implicitWidth: Math.max(text.width + 20, 80)
            height: hasIcon ? 100 : 40
            radius: 10
            Rectangle {
                anchors.bottom: parent.bottom
                color: parent.color
                height: parent.radius
                width: parent.width

            }

            Image {
                visible: root.getTab(styleData.index).icon
                source: root.getTab(styleData.index).icon
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                    margins: 10
                }
            }

            Text {
                id: text
                color: styleData.selected ? "#dcdfd6" : "#aaada4"
                anchors {
                    bottom: parent.bottom
                    horizontalCenter: parent.horizontalCenter
                    margins: 10
                }
                text: styleData.title
            }
        }
        frame: StelGradient {
            c0: "#56575a"
            c1: "#303134"
        }
    }
}
