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
        top: parent.top
        left: parent.left
        right: parent.right
        margins: 10
    }
    StelGroup {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Stars"
        StelItem {
            text: "Absolute scale:"
            target: StelSkyDrawer
            spin: "starAbsoluteScale"
            spinMin: 0
            spinMax: 1.5
            spinStep: 0.1
        }
        StelItem {
            text: "Relative Scale:"
            target: StelSkyDrawer
            spin: "starRelativeScale"
        }
        StelItem {
            text: "Milky Way brightness:"
            target: MilkyWay
            spin: "intensity"
        }
        StelItem {
            text: "Zodiacal Light brightness:"
            target: ZodiacalLight
            spin: "intensity"
        }
        StelItem {
            text: "Twinkle"
            target: StelSkyDrawer
            check: "flagTwinkle"
            spin: "twinkleAmount"
            spinMin: 0
            spinMax: 1.5
            spinStep: 0.1
        }
        StelItem {
            text: "Dynamic Eye adaptation"
            target: StelSkyDrawer
            check: "flagLuminanceAdaptation"
        }
    }
    StelGroup {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Planets and satellites"
        StelItem {
            text: "Show planets"
            target: SolarSystem
            check: "planetsDisplayed"
        }
        StelItem {
            text: "Show planet markers"
            target: SolarSystem
            check: "planetsDisplayed"
        }
        StelItem {
            text: "Show planet orbits"
            target: SolarSystem
            check: "orbitsDisplayed"
        }
        StelItem {
            text: "Simulate light speed"
            target: SolarSystem
            check: "simulateLightSpeed"
        }
        StelItem {
            text: "Scale Moon"
            target: SolarSystem
            check: "scaleMoon"
            spin: "moonScale"
        }
        StelItem {
            text: "Auto select landscapes"
            target: LandscapeMgr
            check: "autoSelect"
        }
    }
    StelGroup {
        Layout.fillWidth: true
        Layout.fillHeight: true
        StelItem {
            text: "Atmosphere"
            bold: true
            target: LandscapeMgr
            check: "atmosphereDisplayed"
        }
        StelItem {
            text: "Light pollution data from locations database"
            target: LandscapeMgr
            check: "databaseUsage"
        }
        StelItem {
            text: "Light pollution:"
            target: LandscapeMgr
            spin: "atmosphereBortleLightPollution"
        }
        // Button
    }
    StelGroup {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Labels and Markers"
        StelItem {
            text: "Stars"
            target: StarMgr
            check: "flagLabelsDisplayed"
            spin: "labelsAmount"
            spinMin: 0.0
            spinMax: 10.0
            type: "slider"
        }
        StelItem {
            text: "DSOs"
            target: NebulaMgr
            check: "flagHintDisplayed"
            spin: "labelsAmount"
            spinMin: 0.0
            spinMax: 10.0
            type: "slider"
        }
        StelItem {
            text: "Planets"
            target: SolarSystem
            check: "labelsDisplayed"
            spin: "labelsAmount"
            spinMin: 0.0
            spinMax: 10.0
            type: "slider"
        }
    }
    StelGroup {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Shooting Stars"
        StelItem {
            id: zhrItem
            text: "ZHR:"
            target: SporadicMeteorMgr
            spin: "zhr"
        }
        Text {
            text: {
                switch (SporadicMeteorMgr.zhr) {
                    case 0: return "No shooting stars";
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                    case 10: return "Normal rate";
                    case 25: return "Standard Orionids rate";
                    case 100: return "Standard Perseids rate";
                    case 120: return "Standard Geminids rate";
                    case 200: return "Exceptional Perseid rate";
                    case 1000: return "Meteor storm rate";
                    case 6000: return "Exceptional Draconid rate";
                    case 10000: return "Exceptional Leonid rate";
                    case 144000: return "Very high rate (1966 Leonids)";
                    case 240000: return "Highest rate ever (1833 Leonids)";
                    default: return "";
                }
            }
            font.pixelSize: 12
        }
    }
    StelGroup {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Limit Magnitudes"
        StelItem {
            text: "Stars"
            target: StelSkyDrawer
            check: "flagStarMagnitudeLimit"
            spin: "customStarMagLimit"
        }
        StelItem {
            text: "DSOs"
            target: StelSkyDrawer
            check: "flagNebulaMagnitudeLimit"
            spin: "customNebulaMagLimit"
        }
    }
}
