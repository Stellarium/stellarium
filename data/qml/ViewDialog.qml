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
    title: "View"
    width: 800
    height: 600

    StelTabView {
        width: parent.width
        height: parent.height
        StelTab {
            title: "Sky"
            icon: "qrc:///graphicGui/tabicon-sky.png"
            width: 500
            height: 500
            Item {
                ViewDialogSky { }
            }
        }
        StelTab {
            title: "Markings"
            icon: "qrc:///graphicGui/tabicon-markings.png"
            width: 500
            height: 500
            Item {
                ViewDialogMarkings { }
            }
        }
        StelTab {
            title: "Landscape"
            icon: "qrc:///graphicGui/tabicon-landscape.png"
            width: 500
            height: 500
            Item {
                ViewDialogLandscape { }
            }
        }
        StelTab {
            title: "Starlore"
            icon: "qrc:///graphicGui/tabicon-starlore.png"
            width: 500
            height: 500
            Item {
                ViewDialogStarlore { }
            }
        }
    }
}
