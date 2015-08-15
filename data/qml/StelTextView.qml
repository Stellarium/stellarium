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

ScrollView {
    id: root
    property string text

    width: parent.width
    height: parent.height
    Text {
        width: parent.parent.width
        text: root.text
        wrapMode: Text.Wrap
    }

    style : ScrollViewStyle {
        scrollBarBackground: Rectangle {
            implicitWidth: 16
            LinearGradient {
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(16, 0)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#515151" }
                    GradientStop { position: 1.0; color: "#747474" }
                }
            }
        }

        handle: Rectangle {
            implicitWidth: 16
            LinearGradient {
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(16, 0)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#b4b4b4" }
                    GradientStop { position: 1.0; color: "#8f8f8f" }
                }
            }
        }

        padding {right: 0; left: 0; top: 0; bottom: 0}

        incrementControl: StelGradient {
            implicitHeight: 20
            implicitWidth: 16
            c0: "#363639"
            c1: "#222223"
            Image {
                anchors.centerIn: parent
                source: "qrc:///graphicGui/spindown.png";
            }
        }

        decrementControl: StelGradient {
            implicitHeight: 20
            implicitWidth: 16
            c0: "#363639"
            c1: "#222223"
            Image {
                anchors.centerIn: parent
                source: "qrc:///graphicGui/spinup.png";
            }
        }
    }
}
