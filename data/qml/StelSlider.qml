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

Slider {

    onValueChanged: {
    }

    style : SliderStyle {
        groove: StelGradient {
            implicitWidth: 200
            implicitHeight: 4
            c0: "#000000"    
            c1: "#363636"
        }
        handle: StelGradient {
            c0: "#b7b8b9"
            c1: "#6f7172"
            anchors.centerIn: parent
            radius: 2
            color: control.pressed ? "white" : "lightgray"
            border.color: "black"
            border.width: 1
            implicitWidth: 32
            implicitHeight: 14
        }
    }
}
