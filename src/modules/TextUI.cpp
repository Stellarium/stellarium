/*
 * Stellarium
 * This file Copyright (C) 2004-2008 Robert Spearman
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <functional>
#include <QSettings>

#include "StelProjector.hpp"
#include "StelNavigator.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "TextUI.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"


void TextUI::init()
{
	return;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double TextUI::getCallOrder(StelModuleActionName actionName) const
{
	// TODO should always draw last (on top)
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+100;
	return 0;
}

void TextUI::update(double deltaTime)
{
	return;
}


void TextUI::draw(StelCore* core)
{
	return;
}

// Stub to hold translation keys
void temporary_translation()
{
	// TUI categories
	N_("Set Location ");
	N_("Set Time ");
	N_("General ");
	N_("Stars ");
	N_("Colors ");
	N_("Effects ");
	N_("Scripts ");
	N_("Administration ");

	// Set location
	N_("Latitude: ");
	N_("Longitude: ");
	N_("Altitude (m): ");
	N_("Solar System Body: ");

	// Set time
	N_("Sky Time: ");
	N_("Set Time Zone: ");
	N_("Day keys: ");
	N_("Calendar");
	N_("Sidereal");
	N_("Preset Sky Time: ");
	N_("Sky Time At Start-up: ");
	N_("Actual Time");
	N_("Preset Time");
	N_("Time Display Format: ");
	N_("Date Display Format: ");

	// General
	N_("Sky Culture: ");
	N_("Sky Language: ");

	// Stars
	N_("Show: ");
	N_("Star Value Multiplier: ");
	N_("Magnitude Sizing Multiplier: ");
	N_("Maximum Magnitude to Label: ");
	N_("Twinkling: ");
	N_("Limiting Magnitude: ");

	// Colors
	N_("Constellation Lines");
	N_("Constellation Names");
	N_("Constellation Art Intensity");
	N_("Constellation Boundaries");
	N_("Cardinal Points");
	N_("Planet Names");
	N_("Planet Orbits");
	N_("Planet Trails");
	N_("Meridian Line");
	N_("Azimuthal Grid");
	N_("Equatorial Grid");
	N_("Equator Line");
	N_("Ecliptic Line");
	N_("Nebula Names");
	N_("Nebula Circles");

	// Effects
	N_("Light Pollution Luminance: ");
	N_("Landscape: ");
	N_("Manual zoom: ");
	N_("Object Sizing Rule: ");
	N_("Magnitude Scaling Multiplier: ");
	N_("Milky Way intensity: ");
	N_("Maximum Nebula Magnitude to Label: ");
	N_("Zoom Duration: ");
	N_("Cursor Timeout: ");
	N_("Correct for light travel time: ");

	// Scripts
	N_("Local Script: ");
	N_("CD/DVD Script: ");
	N_("USB Script: ");
	N_("Arrow down to load list.");
	N_("Select and exit to run.");

	// Administration
	N_("Load Default Configuration: ");
	N_("Save Current Configuration as Default: ");
	N_("Shut Down: ");
	N_("Update me via Internet: ");
	N_("Set UI Locale: ");
}
