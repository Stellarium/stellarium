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
    columns: 1

    anchors {
        fill: parent
        margins: 10
    }
    StelGroup {
        Layout.fillWidth: true
        title: qsTr("Language settings")
        StelItem {
            text: "Program Language"
            target: starsMgr
            choices: stelGui.languages
            currentIndex: choices.indexOf(stelGui.language)
            onCurrentIndexChanged: {
                stelGui.language = choices[currentIndex]
                // XXX: reload gui at this point
            }
        }
        StelItem {
            text: "Sky Culuture Language"
            target: starsMgr
            choices: stelGui.languages
            currentIndex: choices.indexOf(stelGui.skyLanguage)
            onCurrentIndexChanged: {
                stelGui.skyLanguage = choices[currentIndex]
            }
        }
    }
    StelGroup {
        Layout.fillWidth: true
        title: "Default options"
        rowSpacing: 20
        RowLayout {
            Button {Layout.fillWidth: true; text: "Save settings"}
            Button {Layout.fillWidth: true; text: "Restore Defaults"}
        }
        Text {
            Layout.fillWidth: true
            text: "Restoring default settings requires a restart of Stellarium. Saving all the current options includes the current FOV and direction of view for use at next startup."
            wrapMode: Text.Wrap
        }
        Text {
            Layout.fillWidth: true
            text: "Startup FOV: XX"
        }
        Text {
            Layout.fillWidth: true
            text: "Startup direction of view: xxxx"
        }
    }
    Item { Layout.fillHeight: true }
}
