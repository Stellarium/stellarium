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

#include "StelGui.hpp"

StelGui::StelGui()
{
	// Register all default buttons.
	// Maybe this should be done in the different modules directly.
	addButton("qrc:///graphicGui/btConstellationLines-on.png",
			  "qrc:///graphicGui/btConstellationLines-off.png",
			  "actionShow_Constellation_Lines",
			  "010-constellationsGroup");

	addButton("qrc:///graphicGui/btConstellationLabels-on.png",
			  "qrc:///graphicGui/btConstellationLabels-off.png",
			  "actionShow_Constellation_Labels",
			  "010-constellationsGroup");

	addButton("qrc:///graphicGui/btConstellationArt-on.png",
			  "qrc:///graphicGui/btConstellationArt-off.png",
			  "actionShow_Constellation_Art",
			  "010-constellationsGroup");
}

void StelGui::addButton(QString pixOn, QString pixOff,
                        QString action, QString groupName,
                        QString beforeActionName)
{
	QVariantMap button;
	button["pixOn"]     = pixOn;
	button["pixOff"]    = pixOff;
	button["action"]    = action;
	button["groupName"] = groupName;
	button["beforeActionName"] = beforeActionName;
	buttons << button;
	emit buttonsChanged();
}

