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
        model: SkyCultureMgr.getSkyCultureListIDs()
        function getText(v) {return SkyCultureMgr.getSkyCultureI18(v)}
        TableViewColumn { }
        Component.onCompleted: {
            var current = SkyCultureMgr.currentSkyCultureID;
            selection.select(model.indexOf(current));
        }
        onCurrentRowChanged: {
            var value = model[currentRow];
            SkyCultureMgr.currentSkyCultureID = value;
        }
    }

    Column {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 10

        StelTextView {
            width: parent.width
            height: parent.height
            baseUrl: SkyCultureMgr.getSkyCultureBaseUrl(SkyCultureMgr.currentSkyCultureID)
            text: SkyCultureMgr.getSkyCultureDesc(SkyCultureMgr.currentSkyCultureID)
        }
    }

    StelGroup {
        Layout.fillWidth: true
        title: "Options"
        StelItem {
            text: "Use this sky culture as default"
            checked: SkyCultureMgr.currentSkyCultureID == SkyCulture.defaultSkyCultureID
            enabled: SkyCultureMgr.currentSkyCultureID != SkyCulture.defaultSkyCultureID
            onClicked: {
                SkyCulture.defaultSkyCultureID = SkyCultureMgr.currentSkyCultureID
            }
        }
        StelItem {
            text: "Use native names for planets from current culture"
            target: SolarSystem
            check: "useNativeNames"
        }
        StelItem {
            text: "Show names:"
            target: ConstellationMgr
            choices: ["Abbreviated", "Native", "Translated"]
            currentIndex: ConstellationMgr.constellationDisplayStyle
            onCurrentIndexChanged: ConstellationMgr.constellationDisplayStyle = currentIndex
        }
    }
}
