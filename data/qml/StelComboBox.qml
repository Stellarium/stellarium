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

ComboBox {
    implicitWidth: 120
    style: ComboBoxStyle {
        textColor: "#1f1f1f"
        background: StelGradient {
            c0: "#b4b4b7"
            c1: "#5d5f62"
            implicitWidth: control.implicitWidth
            implicitHeight: 24
            border {
                width: 1
                color: "#1f1f1f"
            }
            
            StelGradient {
                width: 20
                height: 22
                c0: "#61615c"
                c1: "#282828"
                anchors {
                    right: parent.right
                    top: parent.top
                    margins: 1
                }
                Image {
                    anchors.centerIn: parent
                    source: "qrc:///graphicGui/spindown.png";
                }
            }
        }
    }
}
