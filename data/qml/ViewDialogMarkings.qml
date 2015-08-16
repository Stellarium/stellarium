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
    StelGroup {
        Layout.fillWidth: true
        // Layout.fillHeight: true
        title: "Celestial Sphere"
        columns: 2
        useRepeater: true

        Repeater {
            model: [
                ["Equatorial grid (J2000)", "Equatorial coordinates of J2000.0.", "actionShow_Equatorial_J2000_Grid"],
                ["Equatorial grid (of date)", "Equatorial coordinates of current date and planet.", "actionShow_Equatorial_Grid"],
                ["Ecliptic grid (J2000)", "Equatorial coordinates of J2000.0.", "actionShow_Ecliptic_J2000_Grid"],
                ["Ecliptic grid (of date)", "Ecliptical coordinates for current date. Displayed on Earth only.", "actionShow_Ecliptic_Grid"],
                ["Azimuthal grid", "Altitudes and azimuth (counted from North towards East).", "actionShow_Azimuthal_Grid"],
                ["Galactic grid", "Galactic Coordinates, System II (IAU 1958).", "actionShow_Galactic_Grid"],
                ["Cardinal points", "Labels for the Cardinal directions.", "actionShow_Cardinal_Points"],
                ["Precession circles", "Instantaneous circles of earth's axis on its motion around ecliptical poles. Displayed on Earth only.", "actionShow_Precession_Circles"],
                ["Equator (J2000)", "Show celestial equator of J2000.0.", "actionShow_Equator_J2000_Line"],
                ["Equator (of date)", "Show celestial equator of current planet and date.", "actionShow_Equator_Line"],
                ["Ecliptic (J2000)", "Show ecliptic line of J2000.0 (VSOP87A fundamental plane).", "actionShow_Ecliptic_J2000_Line"],
                ["Ecliptic (of date)", "Show ecliptic line of current date.", "actionShow_Ecliptic_Line"],
                ["Horizon", "Show horizon line.", "actionShow_Horizon_Line"],
                ["Galactic equator", "Show Galactic equator line.", "actionShow_Galactic_Equator_Line"],
                ["Meridian", "Show meridian line.", "actionShow_Meridian_Line"],
                ["O./C. longitude", "Opposition/conjunction longitude line - the line of ecliptic longitude which passes through both ecliptic poles, the Sun and opposition point.", "actionShow_Longitude_Line"]
            ]

            delegate: StelItem {
                text: modelData[0]
                target: stelActionMgr.findAction(modelData[2])
                check: "checked"
            }
        }
    }
    StelGroup {
        Layout.fillWidth: true
        // Layout.fillHeight: true
        title: "Constellations"
        StelItem {
            text: "Show lines"
            target: ConstellationMgr
            check: "linesDisplayed"
        }
        StelItem {
            text: "Show labels"
            target: ConstellationMgr
            check: "namesDisplayed"
        }
        StelItem {
            text: "Show boundaries"
            target: ConstellationMgr
            check: "boundariesDisplayed"
        }
        StelItem {
            text: "Show art"
            target: ConstellationMgr
            check: "artDisplayed"
        }
        StelItem {
            text: "Art brightness:"
            target: ConstellationMgr
            spin: "artIntensity"
        }
        StelItem {
            text: "Thickness of lines:"
            target: ConstellationMgr
            spin: "twinkleAmount"
        }
    }

    StelGroup {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.columnSpan: 2
        columns: 2
        title: "Projection"

        StelTableView {
            id: table
            Layout.fillWidth: true
            Layout.fillHeight: true
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
    }
}
