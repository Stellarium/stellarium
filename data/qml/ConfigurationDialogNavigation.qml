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
        title: "Control"
        columns: 2

        StelItem {
            text: "Enable keyboard navigation"
            target: starsMgr
            check: "x"
        }

        StelItem {
            text: "Enable mouse navigation"
            target: starsMgr
            check: "x"
        }

        Button {
            text: "Edit keyboard shortcuts..."
        }

        StelItem {
            text: "Mouse cursor timeout:"
            target: starsMgr
            check: "x"
            spin: "x"
        }
    }

    StelGroup {
        Layout.fillWidth: true
        title: "Startup date and time"
        columns: 2

        StelItem {
            text: "System date and time"
            target: starsMgr
            check: "x"
            type: "radio"
        }

        StelItem {
            text: "Other"
            target: starsMgr
            check: "x"
            type: "radio"
            spinType: "datetime"
        }

        StelItem {
            text: "System date at:"
            target: starsMgr
            check: "x"
            type: "radio"
            spinType: "time"
        }

        Button {
            text: "use current"
        }
    }

    StelGroup {
        Layout.fillWidth: true
        title: "Time correction"

        StelItem {
            text: "Algorithm of Δt"
            target: starsMgr
            check: "x"
            choices: ["XXX", "YYY"]
        }
    }
    Item { Layout.fillHeight: true }
}
