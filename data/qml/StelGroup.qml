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
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4

Rectangle {
    id: root
    property string title
    property int columns: 1
    property real rowSpacing: 2
    property bool useRepeater: false

    default property alias contents: contentItem.children
    Layout.preferredHeight: contentItem.implicitHeight + 20

    property ExclusiveGroup exclusiveGroup: ExclusiveGroup { }

    color: "#33959698"
    radius: 10

    GridLayout {
        id: contentItem
        columns: root.columns
        flow: GridLayout.TopToBottom
        implicitHeight: childrenRect.height
        rows: {
            if (root.columns == 1) return -1;
            var nbCells = contentItem.children.length;
            if (root.title) nbCells++;
            if (root.useRepeater) nbCells--;
            if (nbCells % 2) nbCells++;
            return nbCells / 2;
        }
        anchors {
            fill: parent
            margins: 10
        }
        rowSpacing: root.rowSpacing
        columnSpacing: 20

        StelItem {
            visible: root.title
            Layout.columnSpan: root.columns
            text: root.title
            bold: true
        }
    }
}
