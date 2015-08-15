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
    columns: 2
    columnSpacing: 20
    rowSpacing: 20
    anchors {
        fill: parent
        margins: 10
    }

    StelTableView {
        Layout.fillHeight: true
        Layout.rowSpan: 3
        model: ["Perspective", "Equal Area", "Fish-Eye", "Hammer-Aitoff", "Cylinder", "Mercator", "Orthographic", "Sinusoidal"]
        TableViewColumn { }
    }

    Column {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 10

        StelTextView {
            width: parent.width
            height: parent.height
            text: {
                  return "<h2>Western</h2>" +
                  "<p>Western sky culture is used internationally by modern astronomers, and is the official scheme of The International Astronomical Union.  It has historical roots in Ancient Greek astronomy, with influences from Islamic astronomy.</p>" +
                  "<h3>Constellations</h3>" +
                  "<p>The Western culture divides the celestial sphere into 88 areas of various sizes called <i>constellations</i>, each with precise boundary, issued by the International Astronomical Union. These constellations have become the standard way to describe the sky, replacing similar sets in other sky cultures exhaustively in daily usage.</p>" +
                  "<h3>Star names</h3>" +
                  "<p>Most of traditional western star names came from Arabic. In astronomy, Bayer/Flamsteed designations and other star catalogues are widely used instead of traditional names except few cases where the traditional names are more famous than the designations.</p>" +
                  "<h3>Alternative asterism files for Stellarium</h3>" +
                  "<p><a href=\"http://wackymorningdj.users.sourceforge.net/ha_rey_stellarium.zip\" class='external text' rel=\"nofollow\">Asterisms by H.A. Rey</a>, from his book \"The Stars: A New Way To See Them\", by <a class='external text' href=\"http://sourceforge.net/users/wackymorningdj/' rel=\"nofollow\">Mike Richards</a>.</p>" +
                  "<h3>External links</h3>" +
                  "<ul><li> <a href=\"http://en.wikipedia.org/wiki/Constellation\" class='external text' title=\"http://en.wikipedia.org/wiki/Constellation\" rel=\"nofollow\">Constellation</a> article at Wikipedia" +
                  "</li><li> <a href=\"http://en.wikipedia.org/wiki/Star_catalogue\" class='external text' title=\"http://en.wikipedia.org/wiki/Star catalogue\" rel=\"nofollow\">Star Catalogue</a> article at Wikipedia" +
                  "</li><li> <a href=\"http://hubblesource.stsci.edu/sources/illustrations/constellations/\" class='external text' title=\"http://hubblesource.stsci.edu/sources/illustrations/constellations/\" rel=\"nofollow\">Constellation image library</a> of the U.S. Naval Observatory and the Space Telescope Science Institute. Johannes Hevelius Engravings." +
                  "</li></ul>";
            }
        }
    }

    StelGroup {
        Layout.fillWidth: true
        title: "Options"
        StelItem {
            text: "Use this sky culture as default"
            target: starsMgr
            check: "x"
        }
        StelItem {
            text: "Use native names for planets from current culture"
            target: starsMgr
            check: "x"
        }
        StelItem {
            text: "Show names:"
            target: starsMgr
            choices: ["Abbreviated", "Native", "Translated"]
        }
    }

    Button {
        Layout.fillWidth: true
        text: "Add/Remove landscapes"
    }
}
