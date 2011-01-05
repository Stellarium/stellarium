/*
 * Stellarium
 * Copyright (C) 2005 Fabien Chereau
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

// This file contains translations for all translatable strings stored within data files
// It is not meant to be compiled but just parsed by gettext

class Translations
{
	void Translations(){;}

	static translationList()
	{
		// Generate Gettext strings for traduction
		Q_ASSERT(0);
		
		// Planets and satellites from ssystem.ini
		N_("Sun");
		N_("Mercury");
		N_("Venus");
		N_("Earth");
		N_("Moon");
		N_("Mars");
		N_("Deimos");
		N_("Phobos");
		N_("Ceres");
		N_("Pallas");
		N_("Juno");
		N_("Vesta");
		N_("Jupiter");
		N_("Io");
		N_("Europa");
		N_("Ganymede");
		N_("Callisto");
		N_("Saturn");
		N_("Mimas");
		N_("Enceladus");
		N_("Tethys");
		N_("Dione");
		N_("Rhea");
		N_("Titan");
		N_("Hyperion");
		N_("Iapetus");
		N_("Phoebe");
		N_("Neptune");
		N_("Uranus");
		N_("Miranda");
		N_("Ariel");
		N_("Umbriel");
		N_("Titania");
		N_("Oberon");
		N_("Pluto");
		N_("Charon");
		N_("Eris");
		N_("Solar System Observer");
		//Asteroids that are not in the default ssystem.ini
		// TRANSLATORS: Asteroid (5) Astraea
		N_("Astraea");
		// TRANSLATORS: Asteroid (6) Hebe
		N_("Hebe");
		// TRANSLATORS: Asteroid (7) Iris
		N_("Iris");
		// TRANSLATORS: Asteroid (8) Flora
		N_("Flora");
		// TRANSLATORS: Asteroid (9) Metis
		N_("Metis");
		// TRANSLATORS: Asteroid (10) Hygiea
		N_("Hygiea");
		// TRANSLATORS: Asteroid (1221) Amor
		N_("Amor");
		// TRANSLATORS: Asteroid (99942) Apophis
		N_("Apophis");
		// TRANSLATORS: Asteroid (2060) Chiron
		N_("Chiron");
		// TRANSLATORS: Asteroid (433) Eros
		N_("Eros");
		// TRANSLATORS: Asteroid (624) Hektor
		N_("Hektor");
		
		
		// Cardinals names
		N_("N");	// North
		N_("S");	// South
		N_("E");	// East
		N_("W");	// West

		// Constellation cultures
		N_("Aztec");
		N_("Chinese");
		N_("Egyptian");
		N_("Inuit");
		N_("Korean");
		N_("Lakota");
		N_("Maori");
		N_("Navajo");
		N_("Norse");
		N_("Polynesian");
		N_("Sami");
		N_("Tupi-Guarani");
		N_("Western");

		///////////////////////////////////////////////////////
		// Text UI, should be moved somewhere else at somepoint
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
};
