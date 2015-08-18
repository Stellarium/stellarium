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
import org.stellarium 1.0

GridLayout {
    id: root
    columns: 1

    anchors {
        fill: parent
        margins: 10
    }

    StelGroup {
        Layout.fillWidth: true
        title: "Selected object information"
        columns: 2
        useRepeater: true

        Repeater {
            model: [
                ["All available", "Display all information available"],
                ["Short", "Display less information"],
                ["None", "Display no information"],
                ["Customized", "Display user settings information"],
            ]
            delegate: StelItem {
                text: modelData[0]
                check: "x"
                type: "radio"
            }
        }
    }
    StelGroup {
        Layout.fillWidth: true
        // Layout.fillHeight: true
        title: "Displayed fields"
        columns: 2
        useRepeater: true

        Repeater {
            model: [
                ["Name", "", StelGui.Name],
                ["Catalog number(s)", "", StelGui.CatalogNumber],
                ["Right ascension/Declination (J2000)", "Geocentric equatorial coordinates, equinox of J2000.0", StelGui.RaDecJ2000],
                ["Right ascension/Declination (of date)", "Geocentric equatorial coordinates, equinox of date", StelGui.RaDecOfDate],
                ["Ecliptic coordinates", "Ecliptic coordinates, equinox of date and J2000 (only for Earth)", StelGui.EclipticCoord],
                ["Size", "Angular or physical size", StelGui.Size],
                ["Type", "The type of the object (star, planet, etc.)", StelGui.ObjectType],
                ["Visual magnitude", "", StelGui.Magnitude],
                ["Absolute magnitude", "", StelGui.AbsoluteMagnitude],
                ["Galactic coordinates", "", StelGui.GalacticCoord],
                ["Hour angle/Declination", "", StelGui.HourAngle],
                ["Altitude/Azimuth", "", StelGui.AltAzi],
                ["Distance", "", StelGui.Distance],
                ["Additional information", "Spectral class, nebula type, etc.", StelGui.Extra]
            ]

            delegate: StelItem {
                text: modelData[0]
                checked: stelGui.infoTextFilters & modelData[2]
                onClicked: {
                    if (checked)
                        stelGui.infoTextFilters &= ~modelData[2];
                    else
                        stelGui.infoTextFilters |= modelData[2];
                }
            }
        }
    }
    Item { Layout.fillHeight: true }
}
