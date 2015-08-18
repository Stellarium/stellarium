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
    columns: 1

    anchors {
        fill: parent
        margins: 10
    }

    StelGroup {
        Layout.fillWidth: true
        title: "Planetarium options"
        columns: 2
        useRepeater: true

        Repeater {
            model: [
                "Spheric mirror distortion",
                "Select single constellation",
                "Show nebula background button",
                "Auto-enabling for the atmosphere",
                "Auto zoom out returns to initial direction of view",
                "Disc viewport",
                "Gravity labels",
                "Show flip buttons",
                "Use decimal degrees",
            ]
            delegate: StelItem {
                text: modelData
                check: "x"
            }
        }
    }

    StelGroup {
        Layout.fillWidth: true
        title: "Screenshots"
        Row {
            Text {text: "Screenshot Directory"}
            Button {
                height: 24
                text: "test"
                onClicked: screenshotDirFileDialog.visible = true
            }
        }
        StelItem {
            text: "Invert colors"
            target: starsMgr
            check: "x"
        }
    }

    StelGroup {
        Layout.fillWidth: true
        Layout.fillHeight: true
        title: "Star catalog updates"
    }

    FileDialog {
        id: screenshotDirFileDialog
        title: "Please choose a file"
        folder: shortcuts.home
        selectFolder: true
        onAccepted: {
            console.log("You chose: " + screenshotDirFileDialog.fileUrls)
        }
        onRejected: {
            console.log("Canceled")
        }
    }
}
