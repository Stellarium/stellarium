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

StelWindow {
    id: root
    title: "DateTime"
    width: 500
    height: 120
    property var date: stelGui.jdToDate(core.JD)

    function change(i, v) {
        var d = date;
        d[i] = v;
        core.JD = stelGui.jdFromDate(d[0], d[1], d[2], d[3], d[4], d[5]);
    }

    StelTabView {
        width: root.width
        height: root.height
        StelTab {
            title: "Date and Time"
            RowLayout {
                StelGradient {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    c0: "#ebebf9"
                    c1: "#d2d6e0"
                    RowLayout {
                        anchors.fill: parent
                        StelVSpinBox {
                            value: root.date[0]
                            onNewValue: root.change(0, newValue)
                        }
                        Text {
                            text: "/"
                        }
                        StelVSpinBox {
                            value: root.date[1]
                            onNewValue: root.change(1, newValue)
                        }
                        Text {
                            text: "/"
                        }
                        StelVSpinBox {
                            value: root.date[2]
                            onNewValue: root.change(2, newValue)
                        }
                    }
                }
                StelGradient {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    c0: "#ebebf9"
                    c1: "#d2d6e0"
                    RowLayout {
                        anchors.fill: parent
                        StelVSpinBox {
                            value: root.date[3]
                            onNewValue: root.change(3, newValue)
                        }
                        Text {
                            text: ":"
                        }
                        StelVSpinBox {
                            value: root.date[4]
                            onNewValue: root.change(4, newValue)
                        }
                        Text {
                            text: ":"
                        }
                        StelVSpinBox {
                            value: root.date[5]
                            onNewValue: root.change(5, newValue)
                        }
                    }
                }
            }
        }

        StelTab {
            title: "Julian Date"
            RowLayout {
                StelGradient {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    c0: "#ebebf9"
                    c1: "#d2d6e0"
                    RowLayout {
                        anchors.fill: parent
                        Text {
                            Layout.fillWidth: true
                            text: "JD:"
                            horizontalAlignment: Text.AlignRight
                        }
                        StelVSpinBox {
                            value: core.JD
                            function getRepr(v) {return v.toFixed(5)}
                            onNewValue: core.JD = newValue
                        }
                    }
                }
                StelGradient {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    c0: "#ebebf9"
                    c1: "#d2d6e0"
                    RowLayout {
                        anchors.fill: parent
                        Text {
                            Layout.fillWidth: true
                            text: "MJD:"
                            horizontalAlignment: Text.AlignRight
                        }
                        StelVSpinBox {
                            function getRepr(v) {return v.toFixed(5)}
                            value: core.JD - 2400000.5
                            onNewValue: core.JD = newValue + 2400000.5
                        }
                    }
                }
            }
        }
    }
}
