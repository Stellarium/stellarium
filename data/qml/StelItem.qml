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

RowLayout {
    id: root
    property string text
    property bool bold: false
    property var target
    property string check
    property bool checked: root.check ? root.target[root.check] : undefined
    property var choices: null
    property alias currentIndex: comboBox.currentIndex
    property string type: "default"
    property string spinType: type

    property string spin
    property double spinMin
    property double spinMax
    property double spinStep: 1.0
    signal clicked()


    height: 30
    Layout.fillWidth: true

    StelCheckBox {
        id: checkBoxItem
        visible: root.checked !== undefined
        checked: root.checked
        onCheckedChanged: {
            if (root.check) {
                root.target[root.check] = checked;
                checked = Qt.binding(function(){return root.checked});
            }
        }
        onClicked: root.clicked()
    }

    StelRadioButton {
        id: radioItem
        visible: root.check && root.type == "radio"
        checked: visible && root.target[root.check]
        exclusiveGroup: root.parent.parent.exclusiveGroup
    }

    Text {
        text: root.text
        font.bold: root.bold
    }

    Item {
        Layout.fillWidth: true
    }

    StelSpinBox {
        visible: root.spin && root.spinType == "default"
        minimumValue: spinMin
        maximumValue: spinMax
        stepSize: spinStep
        value: visible ? root.target[root.spin] : 0
        onValueChanged: {
            root.target[root.spin] = value;
            value = Qt.binding(function(){return root.target[root.spin]});
        }
        Layout.alignment: Qt.AlignRight
    }

    StelLatLonEdit {
        visible: root.spinType == "latitude" || root.spinType == "longitude" || root.spinType == "ra" || root.spinType == "de"
        type: root.spinType
        value: visible ? root.target[root.spin] : 0

        onValueChanged: {
            root.target[root.spin] = value;
            value = Qt.binding(function(){return root.target[root.spin]});
        }
    }

    StelDateEdit {
        visible: root.spinType == "time" || root.spinType == "datetime"
        type: root.spinType
        value: visible ? root.target[root.spin] : 0

        onValueChanged: {
            root.target[root.spin] = value;
            value = Qt.binding(function(){return root.target[root.spin]});
        }
    }

    StelSlider {
        visible: root.spin && root.type == "slider"
        minimumValue: root.spinMin
        maximumValue: root.spinMax
        value: visible ? root.target[root.spin] : 0
        onValueChanged: {
            root.target[root.spin] = value;
            value = Qt.binding(function(){return root.target[root.spin]});
        }
        Layout.alignment: Qt.AlignRight
    }

    StelComboBox {
        id: comboBox
        visible: root.choices
        model: root.choices
    }
}
