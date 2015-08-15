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
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.2

GridLayout {
    columns: 2
    columnSpacing: 20
    rowSpacing: 20
    anchors {
        fill: parent
        margins: 10
    }

    StelTableView {
        Layout.fillHeight: true
        Layout.rowSpan: 3
        model: ["Perspective", "Equal Area", "Fish-Eye", "Hammer-Aitoff", "Cylinder", "Mercator", "Orthographic", "Sinusoidal"]
        TableViewColumn { }
    }

    Column {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 10

        Text {
            text: "Stereographic"
            font.bold: true
        }

        Text {
            width: parent.width
            text: "In fish-eye projection, or <i>azimuthal equidistant projection</i>, straight lines become curves when they appear a large angular distance from the centre of the field of view (like the distortions seen with very wide angle camera lenses)."
            wrapMode: Text.Wrap
        }

        Text {
            text: "<b>" + "Maximum FOV:" + " </b>" + 10 + "°"
        }
    }

    StelGroup {
        Layout.fillWidth: true
        title: "Options"
        StelItem {
            text: "Show ground"
            target: starsMgr
            check: "x"
        }
        StelItem {
            text: "Show fog"
            target: starsMgr
            check: "x"
        }
        StelItem {
            text: "Use associated planet and position"
            target: starsMgr
            check: "x"
        }
        StelItem {
            text: "Use this landscape as default"
            target: starsMgr
            check: "x"
        }
        Row {
            spacing: 8
            StelItem {
                text: "Minimal brightness:"
                target: starsMgr
                check: "x"
                spin: "x"
            }
            StelItem {
                text: "from landscape, if given"
                target: starsMgr
                check: "x"
            }
        }
        StelItem {
            text: "Show illumination layer (bright windows, light pollution, etc.)"
            target: starsMgr
            check: "x"
        }
        StelItem {
            text: "Show landscape labels"
            target: starsMgr
            check: "x"
        }
    }

    Button {
        Layout.fillWidth: true
        text: "Add/Remove landscapes"
    }
}
