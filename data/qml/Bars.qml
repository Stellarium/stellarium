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

import QtQuick 2.5
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0
import org.stellarium 1.0


Item {
    id: bars
    anchors.fill: parent
    opacity: 0.75
    layer.enabled: true

    property var bottomGroups: {
        var tmp = {};
        for (var i = 0; i < stelGui.buttons.length; i++) {
            var g = stelGui.buttons[i].groupName;
            if (g == "win-bar") continue;
            tmp[g] = true;
        }
        var ret = [];
        for (var g in tmp)
            ret.push(g);
        return ret;
    }

    function getButtons(group) {
        var ret = [];
        for (var i = 0; i < stelGui.buttons.length; i++) {
            var b = stelGui.buttons[i];
            if (b.groupName == group)
                ret.push(b);
        }
        return ret;
    }

    Canvas {
        id: canvas
        width: bottomBar.width
        height: winBar.height
        anchors {
            left: winBar.left
            bottom: winBar.bottom
        }

        onPaint: {
            var ctx = canvas.getContext('2d');
            var r = 10;
            ctx.save();
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.translate(0, 0);
            ctx.globalAlpha = 1;
            ctx.strokeStyle = "#7fb2b2b2";
            ctx.fillStyle = "#3338383a";
            ctx.lineWidth = 1;

            ctx.beginPath();
            ctx.moveTo(0,0);
            ctx.lineTo(winBar.width - r, 0);
            ctx.quadraticCurveTo(winBar.width, 0, winBar.width, r);
            ctx.lineTo(winBar.width, winBar.height - bottomBar.height);
            ctx.lineTo(bottomBar.width - r, winBar.height - bottomBar.height);
            ctx.quadraticCurveTo(bottomBar.width, winBar.height - bottomBar.height,
                                 bottomBar.width, winBar.height - bottomBar.height + r);
            ctx.lineTo(bottomBar.width, winBar.height);
            ctx.lineTo(0, winBar.height);

            ctx.closePath();
            ctx.fill();
            ctx.stroke();
            ctx.restore();
        }
    }

    /*
    MouseArea {
        anchors {
            bottom: parent.bottom
            left: parent.left
        }
        width: 100
        height: 100
        hoverEnabled: true


        onEntered: {bottomBar.hovered = true; winBar.hovered = true}
        onExited:  {bottomBar.hovered = false; winBar.hovered = false}
    }
    MouseArea {
        anchors.fill: winBar
        anchors.margins: -32
        anchors.bottomMargin: 100
        hoverEnabled: true
        onEntered: {winBar.hovered = true;}
        onExited:  {winBar.hovered = false;}
        onContainsMouseChanged: winBar.hovered = containsMouse
    }
    MouseArea {
        anchors.fill: bottomBar
        anchors.leftMargin: 100
        hoverEnabled: true
        onEntered: {bottomBar.hovered = true;}
        onExited:  {bottomBar.hovered = false;}
    }
    */
    HoverArea {
        anchors.fill: winBar
        anchors.margins: -32
        onHoveredChanged: winBar.hovered = hovered
    }

    HoverArea {
        anchors.fill: bottomBar
        onHoveredChanged: bottomBar.hovered = hovered
    }

    WinBar {
        id: winBar
        onWidthChanged: canvas.requestPaint()
        property bool hovered: false
        up: hovered || !stelActionMgr.findAction("actionAutoHideVerticalButtonBar").checked
        Behavior on width {
            NumberAnimation {
                duration: 100
            }
        }
    }

    BottomBar {
        id: bottomBar
        onHeightChanged: canvas.requestPaint()
        property bool hovered: false
        up: hovered || !stelActionMgr.findAction("actionAutoHideHorizontalButtonBar").checked

        anchors {
            bottom: parent.bottom
        }
        Behavior on height {
            NumberAnimation {
                duration: 100
            }
        }
    }

    StelActionButton {
        visible: bottomBar.up || winBar.up
        image: "qrc:///graphicGui/HorizontalAutoHideOn.png"
        imageOff: "qrc:///graphicGui/HorizontalAutoHideOff.png"
        action: "actionAutoHideHorizontalButtonBar"
        anchors {
            left: parent.left
            bottom: parent.bottom
            margins: 4
            leftMargin: 6
        }
    }
    StelActionButton {
        visible: bottomBar.up || winBar.up
        image: "qrc:///graphicGui/VerticalAutoHideOn.png"
        imageOff: "qrc:///graphicGui/VerticalAutoHideOff.png"
        action: "actionAutoHideVerticalButtonBar"
        anchors {
            left: parent.left
            bottom: parent.bottom
            margins: 4
            bottomMargin: 6
        }
    }
}
