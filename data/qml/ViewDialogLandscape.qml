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
    id: root
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
        model: LandscapeMgr.getAllLandscapeNames()
        TableViewColumn { }
        Component.onCompleted: {
            var current = LandscapeMgr.getCurrentLandscapeName();
            selection.select(model.indexOf(current));
        }
        onCurrentRowChanged: {
            var value = model[currentRow];
            LandscapeMgr.setCurrentLandscapeName(value);
            landscapeDesc.text = LandscapeMgr.getCurrentLandscapeHtmlDescription();
        }
    }

    Column {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 10

        Text {
            id: landscapeDesc
            width: parent.width
            text: LandscapeMgr.getCurrentLandscapeHtmlDescription()
            wrapMode: Text.Wrap
        }
    }

    StelGroup {
        Layout.fillWidth: true
        title: "Options"
        StelItem {
            text: "Show ground"
            target: LandscapeMgr
            check: "landscapeDisplayed"
        }
        StelItem {
            text: "Show fog"
            target: LandscapeMgr
            check: "fogDisplayed"
        }
        StelItem {
            text: "Use associated planet and position"
            target: LandscapeMgr
            check: "landscapeSetsLocation"
        }
        StelItem {
            text: "Use this landscape as default"
            checked: LandscapeMgr.currentLandscapeID == LandscapeMgr.defaultLandscapeID
            enabled: LandscapeMgr.currentLandscapeID != LandscapeMgr.defaultLandscapeID
            onClicked: {
                LandscapeMgr.defaultLandscapeID = LandscapeMgr.currentLandscapeID;
            }
        }
        Row {
            spacing: 8
            StelItem {
                text: "Minimal brightness:"
                target: LandscapeMgr
                check: "useMinimalBrightness"
                spin: "defaultMinimalBrightness"
            }
            StelItem {
                text: "from landscape, if given"
                target: LandscapeMgr
                check: "setMinimalBrightness"
            }
        }
        StelItem {
            text: "Show illumination layer (bright windows, light pollution, etc.)"
            target: LandscapeMgr
            check: "illuminationDisplayed"
        }
        StelItem {
            text: "Show landscape labels"
            target: LandscapeMgr
            check: "labelsDisplayed"
        }
    }

    Button {
        Layout.fillWidth: true
        text: "Add/Remove landscapes"
    }
}
