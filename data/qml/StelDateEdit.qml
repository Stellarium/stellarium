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

StelSpinBox {
    id: root
    property string type: "datetime"
    implicitWidth: 200

    function getValue(s) {
        var reg = /(\d+):(\d+) ((AM)|(PM))"/;
        var m = reg.exec(s);
        var hour = m[1];
        var min  = m[2];
        var ampm = m[3] == "AM" ? 0 : 1;
        return (ampm * 12 + hour) * 3600 + min * 60;
    }

    function getRepr(v) {
        var sec = v % 1;
        v = (v - sec) / 60;
        var min = v % 60;
        v = (v - min) / 60;
        var hour = v % 12;
        var ampm = v < 12 ? "AM" : "PM";
        hour = "" + hour;
        if (hour.length == 1) hour = "0" + hour;
        min = "" + min;
        if (min.length == 1) min = "0" + min;
        return "%1:%2 %3".arg(hour).arg(min).arg(ampm);
    }

    function getDelta(s) {
        if (s.indexOf(" ") != -1)
            return 12 * 60 * 60;
        if (s.indexOf(":") != -1)
            return 60;
        return 60 * 60;
    }
}
