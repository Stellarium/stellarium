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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

// This file contains translations for all translatable strings stored within data files
// It is not meant to be compiled but just parsed by gettext
// names of celestial objects for screen display (Sky language) have been taken to translations_skycultures.h

class Translations
{
	void Translations(){;}

	static translationList()
	{
		// Generate Gettext strings for traduction
		Q_ASSERT(0);

		// Cardinals names
		// TRANSLATORS: Cardinals names: North
		N_("N");
		// TRANSLATORS: Cardinals names: South
		N_("S");
		// TRANSLATORS: Cardinals names: East
		N_("E");
		// TRANSLATORS: Cardinals names: West
		N_("W");

		// =====================================================================
		// List of types solar system bodies
		// TRANSLATORS: Type of object
		N_("star");
		// TRANSLATORS: Type of object
		N_("planet");
		// TRANSLATORS: Type of object
		N_("comet");
		// TRANSLATORS: Type of object
		N_("asteroid");
		// TRANSLATORS: Type of object
		N_("moon");
		// TRANSLATORS: Type of object
		N_("plutino");
		// TRANSLATORS: Type of object
		N_("dwarf planet");
		// TRANSLATORS: Type of object
		N_("cubewano");
		// TRANSLATORS: Type of object
		N_("scattered disc object");
		// TRANSLATORS: Type of object
		N_("Oort cloud object");
		// TRANSLATORS: Type of object
		N_("sednoid");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Planets");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Comets");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Asteroids");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Moons");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Plutinos");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Dwarf planets");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Cubewanos");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Scattered disc objects");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Oort cloud objects");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Sednoids");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Constellations");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Custom Objects");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Asterisms");

		// =====================================================================
		// List of deep-sky objects types
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Bright galaxies");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Open star clusters");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Globular star clusters");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Nebulae");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Planetary nebulae");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Dark nebulae");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Clusters associated with nebulosity");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("HII regions");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Reflection nebulae");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Active galaxies");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Radio galaxies");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Interacting galaxies");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Bright quasars");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Star clusters");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Stellar associations");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Star clouds");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Bipolar nebulae");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Emission nebulae");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Supernova remnants");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Interstellar matter");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Emission objects");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("BL Lac objects");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Blazars");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Molecular Clouds");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Young Stellar Objects");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Possible Quasars");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Possible Planetary Nebulae");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Protoplanetary Nebulae");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Messier Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Caldwell Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Barnard Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Sharpless Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Van den Bergh Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("The Catalogue of Rodgers, Campbell, and Whiteoak");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Collinder Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Melotte Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("New General Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Index Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Lynds' Catalogue of Bright Nebulae");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Lynds' Catalogue of Dark Nebulae");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Cederblad Catalog");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("The Catalogue of Peculiar Galaxies");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("The Catalogue of Interacting Galaxies");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("The Catalogue of Galactic Planetary Nebulae");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Dwarf galaxies");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Herschel 400 Catalogue");
		
		// =====================================================================
		// List of stars types
		// TRANSLATORS: Type of stars (for "Lists" in the search tool)
		N_("Interesting double stars");
		// TRANSLATORS: Type of stars (for "Lists" in the search tool)
		N_("Interesting variable stars");
		// TRANSLATORS: Type of stars (for "Lists" in the search tool)
		N_("Bright double stars");
		// TRANSLATORS: Type of stars (for "Lists" in the search tool)
		N_("Bright variable stars");
		// TRANSLATORS: Type of stars (for "Lists" in the search tool)
		N_("Bright stars with high proper motion");
		
		// =====================================================================
		// Constellation cultures
		// TRANSLATORS: Name of the sky culture
		N_("Arabic");
		// TRANSLATORS: Name of the sky culture
		N_("Arabic Moon Stations");
		// TRANSLATORS: Name of the sky culture
		N_("Aztec");
		// TRANSLATORS: Name of the sky culture
		N_("Belarusian");
		// TRANSLATORS: Name of the sky culture
		N_("Boorong");
		// TRANSLATORS: Name of the sky culture
		N_("Chinese");
		// TRANSLATORS: Name of the sky culture
		N_("Egyptian");
		// TRANSLATORS: Name of the sky culture
		N_("Hawaiian Starlines");
		// TRANSLATORS: Name of the sky culture
		N_("Inuit");
		// TRANSLATORS: Name of the sky culture
		N_("Indian Vedic");
		// TRANSLATORS: Name of the sky culture
		N_("Japanese Moon Stations");
		// TRANSLATORS: Name of the sky culture
		N_("Kamilaroi/Euahlayi");
		// TRANSLATORS: Name of the sky culture
		N_("Korean");
		// TRANSLATORS: Name of the sky culture
		N_("Dakota/Lakota/Nakota");
		// TRANSLATORS: Name of the sky culture
		N_("Macedonian");
		// TRANSLATORS: Name of the sky culture
		N_("Maori");
		// TRANSLATORS: Name of the sky culture
		N_("Mongolian");
		// TRANSLATORS: Name of the sky culture
		N_("Navajo");
		// TRANSLATORS: Name of the sky culture
		N_("Norse");
		// TRANSLATORS: Name of the sky culture
		N_("Ojibwe");
		// TRANSLATORS: Name of the sky culture
		N_("Polynesian");
		// TRANSLATORS: Name of the sky culture
		N_("Romanian");
		// TRANSLATORS: Name of the sky culture
		N_("Sami");
		// TRANSLATORS: Name of the sky culture
		N_("Sardinian");
		// TRANSLATORS: Name of the sky culture
		N_("Siberian");
		// TRANSLATORS: Name of the sky culture
		N_("Tukano");
		// TRANSLATORS: Name of the sky culture
		N_("Tupi-Guarani");
		// TRANSLATORS: Name of the sky culture
		N_("Tongan");
		// TRANSLATORS: Name of the sky culture
		N_("Western");
		// TRANSLATORS: Name of the sky culture
		N_("Western (H.A.Rey)");

		
		// =====================================================================
		// Landscape names
		// TRANSLATORS: Name of landscape
		N_("Guereins");
		// TRANSLATORS: Name of landscape
		N_("Trees");
		// TRANSLATORS: Name of landscape
		N_("Moon");
		// TRANSLATORS: Landscape name: Hurricane Ridge
		N_("Hurricane");
		// TRANSLATORS: Name of landscape
		N_("Ocean");
		// TRANSLATORS: Landscape name: Garching bei Munchen
		N_("Garching");
		// TRANSLATORS: Name of landscape
		N_("Mars");
		// TRANSLATORS: Name of landscape
		N_("Jupiter");
		// TRANSLATORS: Name of landscape
		N_("Saturn");
		// TRANSLATORS: Name of landscape
		N_("Uranus");
		// TRANSLATORS: Name of landscape
		N_("Neptune");
		// TRANSLATORS: Name of landscape
		N_("Geneva");
		// TRANSLATORS: Name of landscape
		N_("Grossmugl");
		// TRANSLATORS: Name of landscape
		N_("Zero Horizon");

		// =====================================================================
		// 3D landscapes (scenes) names
		// TRANSLATORS: Name of 3D scene ("Sterngarten" is proper name)
		N_("Vienna Sterngarten");
		// TRANSLATORS: Name of 3D scene
		N_("Testscene");
		
		// =====================================================================
		// Script names
		// TRANSLATORS: Name of script
		N_("Landscape Tour");
		// TRANSLATORS: Name of script
		N_("Partial Lunar Eclipse");
		// TRANSLATORS: Name of script
		N_("Total Lunar Eclipse");
		// TRANSLATORS: Name of script
		N_("Screensaver");
		// TRANSLATORS: Name of script
		N_("Solar Eclipse 2009");
		// TRANSLATORS: Name of script
		N_("Startup Script");
		// TRANSLATORS: Name of script
		N_("Zodiac");
		// TRANSLATORS: Name of script
		N_("Mercury Triple Sunrise and Sunset");
		// TRANSLATORS: Name of script
		N_("Double eclipse from Deimos in 2017");
		// TRANSLATORS: Name of script
		N_("Double eclipse from Deimos in 2031");
		// TRANSLATORS: Name of script
		N_("Eclipse from Olympus Mons Jan 10 2068");
		// TRANSLATORS: Name of script
		N_("Occultation of Earth and Jupiter 2048");
		// TRANSLATORS: Name of script
		N_("3 Transits and 2 Eclipses from Deimos 2027");
		// TRANSLATORS: Name of script
		N_("Solar System Screensaver");
		// TRANSLATORS: Name of script
		N_("Constellations Tour");
		// TRANSLATORS: Name of script
		N_("Sun from different planets");
		// TRANSLATORS: Name of script
		N_("Earth best views from other bodies");
		// TRANSLATORS: Name of script
		N_("Transit of Venus");
		// TRANSLATORS: Name of script
		N_("Sky Culture Tour");
		// TRANSLATORS: Name and description of script
		N_("Earth Events from Mercury");
		// TRANSLATORS: Name and description of script
		N_("Earth Events from a floating city on Venus");
		// TRANSLATORS: Name and description of script
		N_("Earth Events from Mars");
		// TRANSLATORS: Name of script
		N_("Earth and other planet's Greatest Elongations and Oppositions from Mars");
		// TRANSLATORS: Name of script
		N_("Earth and Mars Greatest Elongations and Transits from Callisto");
		// TRANSLATORS: Name of script
		N_("Tycho's Supernova");
		// TRANSLATORS: Name of script
		N_("Earth and other Planets from Ceres");
		// TRANSLATORS: Name of script
		N_("Messier Objects Tour");
		// TRANSLATORS: Name of script
		N_("Binocular Highlights");
		// TRANSLATORS: Name of script
		N_("20 Fun Naked-Eye Double Stars");
		// TRANSLATORS: Name of script
		N_("List of largest known stars");
		// TRANSLATORS: Name of script
		N_("Herschel 400 Tour");
		// TRANSLATORS: Name of script
		N_("Binosky: Deep Sky Objects for Binoculars");
		// TRANSLATORS: Name of script
		N_("The Jack Bennett Catalog");
		// TRANSLATORS: Name of script
		N_("Best objects in the New General Catalog");
		
		// =====================================================================
		// Script descriptions
		
		// TRANSLATORS: Description of the landscape tour script.
		N_("Look around each installed landscape.");
		// TRANSLATORS: Description of the sky culture tour script.
		N_("Look at each installed sky culture.");
		N_("Script to demonstrate a partial lunar eclipse.");
		N_("Script to demonstrate a total lunar eclipse.");
		N_("A slow, infinite tour of the sky, looking at random objects.");
		N_("Script to demonstrate a total solar eclipse which has happened in 2009 (location=Rangpur, Bangladesh).");
		N_("Script which runs automatically at startup");
		N_("This script displays the constellations of the Zodiac. That means the constellations which lie along the line which the Sun traces across the celestial sphere over the course of a year.");
		N_("Due to the quirks in Mercury's orbit and rotation at certain spots the sun will rise & set 3 different times in one Mercury day.");
		N_("Just before Mars eclipses the sun, Phobos pops out from behind and eclipses it first. Takes place between Scorpio and Sagittarius on April 26, 2017.");
		N_("Just before Mars eclipses the sun, Phobos pops out from behind and eclipses it first. Takes place between Taurus and Gemini on July 23, 2031.");
		N_("Phobos eclipsing the Sun as seen from Olympus Mons on Jan 10, 2068.");
		N_("Phobos occultations of Earth are common, as are occultations of Jupiter. But occultations of both on the same day are very rare. Here's one that takes place 1/23/2048. In real speed.");
		N_("Phobos races ahead of Mars and transits the sun, passes through it and then retrogrades back towards the sun and just partially transits it again (only seen in the southern hemisphere of Deimos), then Mars totally eclipses the sun while Phobos transits in darkness between Mars and Deimos. When Phobos emerges from Mars it is still eclipsed and dimmed in Mars' shadow, only to light up later.");
		N_("Screensaver of various happenings in the Solar System. 287 events in all!");
		N_("A tour of the western constellations.");
		N_("Look at the Sun from big planets of Solar System and Pluto.");
		N_("Best views of Earth from other Solar System bodies in the 21st Century.");
		N_("Transit of Venus as seen from Sydney Australia, 6th June 2012.");
		N_("Flash of the supernova observed by Tycho Brahe in 1572. The Supernovae plugin has to be enabled.");
		N_("Earth and other planet's Greatest Elongations and Oppositions from Mars 2000-3000");
		N_("Earth Greatest Elongations and Transits from Callisto 2000-3000. Why Callisto? Well of the 4 Galilean Moons, Callisto is the only one outside of Jupiter's radiation belt. Therefore, if humans ever colonize Jupiter's moons, Callisto will be the one.");
		N_("Earth the other visible Planet's Greatest Elongations and Oppositions from Ceres 2000-3000");
		N_("A tour of Messier Objects");
		N_("Tours around interesting objects, which accessible to observation with binoculars. The data for the script are taken from the eponymous book by Gary Seronik.");
		N_("This script helps you make an excursion around 20 fun double stars. The list has been collected by Jerry Lodriguss and published in Sky & Telescope 09/2014. Data taken from his website, http://www.astropix.com/doubles/");
		N_("This script helps you make an excursion around largest known stars.");
		N_("A tour around objects from the Herschel 400 Catalogue");
		N_("Ben Crowell has created Binosky, an observing list of Deep Sky Objects for Binoculars. In the script we give a list of these 31 objects, ordered by Right Ascension (2000.0).");
		N_("The Jack Bennett Catalog of Southern Deep-Sky Objects (152 objects in all). The Bennett catalog was contributed by Auke Slotegraaf.");
		N_("This list of 111 objects by A.J. Crayon and Steve Coe is used by members of the Saguaro Astronomy Club of Phoenix, AZ, for the Best of the NGC achievement award.");
		
		// =====================================================================
		// List of GUI elements (Qt's dialogs)
		N_("&Undo");
		N_("&Redo");
		N_("Cu&t");
		N_("&Copy");
		N_("&Paste");
		N_("Delete");
		N_("Select All");
		N_("Look in:");
		N_("Directory:");
		N_("Folder");
		N_("&Choose");
		N_("Cancel");
		N_("&Cancel");
		N_("Files of type:");
		N_("Date Modified");
		N_("Directories");
		N_("Computer");
		N_("&Open");
		N_("File &name:");
		N_("Copy &Link Location");
		N_("Abort");
		N_("Ignore");
		N_("&Basic colors");
		N_("&Pick Screen Color");
		N_("&Custom colors");
		N_("&Add to Custom Colors");
		N_("Hu&e:");
		N_("&Sat:");
		N_("&Val:");
		N_("&Red:");
		N_("&Green:");
		N_("Bl&ue:");
		N_("Select Color");
		N_("Cursor at %1, %2 Press ESC to cancel");
	}
};
