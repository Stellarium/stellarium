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
    property string type
    implicitWidth: 200

    function getValue(s) {
        if (type == "latitude" || type == "longitude") {
            var reg = /([SNWE]) (\d+)° (\d+)' (\d+)"/;
            var m = reg.exec(s);
            var sign = (m[1] == 'S' || m[0] == 'W') ? -1 : +1;
            var deg = +m[2];
            var min = +m[3];
            var sec = +m[4];
            return sign * (deg + min / 60. + sec / 3600.);
        }
        if (type == "ra") {
            var reg = /(\d+)h (\d+)m (\d+\.\d+)s/;
            var m = reg.exec(s);
            var hour = m[1];
            var min  = m[2];
            var sec  = m[3];
            return (+hour + min / 60. + sec / 3600.) * 15;
        }
        if (type == "de") {
            var reg = /([+-])(\d+)° (\d+)' (\d+\.\d+)"/;
            var sign = m[1] == '+' ? +1 : -1;
            var deg  = +m[2];
            var min  = +m[3];
            var sec  = +m[4];
            return sign * (deg + min / 60. + sec / 3600.);
        }
    }

    function getRepr(v) {
        if (type == "latitude" || type == "longitude") {
            var signs = root.type == "latitude" ? ["S", "N"] : ["W", "E"];
            var sign = signs[v >= 0 ? 1 : 0];
            v = Math.abs(v);
            var degree = Math.floor(v);
            v = (v - degree) * 60;
            var minutes = Math.floor(v);
            v = (v - minutes) * 60;
            var seconds = Math.floor(v);
            return "%4 %1° %2' %3\"".arg(degree).arg(minutes).arg(seconds).arg(sign);
        }
        if (type == "ra") {
            v = v / 15;
            var hour = Math.floor(v);
            v = (v - hour) * 60;
            var minutes = Math.floor(v);
            v = (v - minutes) * 60;
            var seconds = v.toFixed(2);
            return "%1h %2m %3\s".arg(hour).arg(minutes).arg(seconds);
        }
        if (type == "de") {
            var sign = v >= 0 ? '+' : '-';
            v = Math.abs(v);
            var deg = Math.floor(v);
            v = (v - deg) * 60;
            var min = Math.floor(v);
            v = (v - min) * 60;
            var sec = v.toFixed(2);
            return "%1%2° %3' %4\"".arg(sign).arg(deg).arg(min).arg(sec);
        }
        return ""
    }

    function getDelta(s) {
        if (type == "latitude" || type == "longitude") {
            if (s.indexOf("'") != -1) return 1.0 / 60.0 / 60.0;
            if (s.indexOf("°") != -1) return 1.0 / 60.0;
            if (s.length <= 1) return -1;
            return 1;
        }
        if (type == "ra") {
            if (s.indexOf("m") != -1) return 15.0 / 60.0 / 60.0;
            if (s.indexOf("h") != -1) return 15.0 / 60.0;
            return 15;
        }
        if (type == "de") {
            if (s.indexOf("'") != -1) return 1.0 / 60.0 / 60.0;
            if (s.indexOf("°") != -1) return 1.0 / 60.0;
            return 1;
        }
    }
}
