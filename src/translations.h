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
		N_("Amalthea");
		N_("Himalia");
		N_("Elara");
		N_("Pasiphae");
		N_("Sinope");
		N_("Lysithea");
		N_("Carme");
		N_("Ananke");
		N_("Leda");
		N_("Thebe");
		N_("Adrastea");
		N_("Metis");
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
		// TRANSLATORS: Moon of Pluto (II)
		N_("Nix");
		// TRANSLATORS: Moon of Pluto (III)
		N_("Hydra (moon)");
		N_("Eris");
		N_("Triton");
		N_("Nereid");
		N_("Naiad");
		N_("Thalassa");
		N_("Despina");
		N_("Galatea");
		N_("Larissa");
		N_("Proteus");
		N_("Halimede");
		N_("Psamathe");
		N_("Sao");
		N_("Laomedeia");
		N_("Neso");
		N_("Solar System Observer");
		//TNO's that are in the default ssystem.ini
		// TRANSLATORS: TNO/Asteroid (90377) Sedna
		N_("Sedna");
		// TRANSLATORS: TNO/Asteroid (50000) Quaoar
		N_("Quaoar");
		// TRANSLATORS: TNO/Asteroid (90482) Orcus
		N_("Orcus");
		// TRANSLATORS: TNO/Asteroid (136108) Haumea
		N_("Haumea");
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
		// TRANSLATORS: Asteroid (41) Daphne
		N_("Daphne");
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
		// TRANSLATORS: Asteroid (20000) Varuna
		N_("Varuna");
		// TRANSLATORS: Asteroid (136472) Makemake
		N_("Makemake");
		// TRANSLATORS: Asteroid (3200) Phaethon
		N_("Phaethon");
		// TRANSLATORS: Asteroid (1866) Sisyphus
		N_("Sisyphus");
		// TRANSLATORS: Asteroid (1864) Daedalus
		N_("Daedalus");
		// TRANSLATORS: Asteroid (1566) Icarus
		N_("Icarus");
		// TRANSLATORS: Asteroid (1809) Prometheus
		N_("Prometheus");
		N_("Prometheus (moon)");
		// TRANSLATORS: Asteroid (1810) Epimetheus
		N_("Epimetheus");
		N_("Epimetheus (moon)");
		// TRANSLATORS: Asteroid (951) Gaspra
		N_("Gaspra");
		// TRANSLATORS: Asteroid (243) Ida
		N_("Ida");
		// TRANSLATORS: Asteroid (218) Bianca
		N_("Bianca");
		N_("Bianca (moon)");
		
		// TRANSLATORS: Proper name of comet C/1680 V1
		N_("Great Comet of 1680 (C/1680 V1)");
		
		// TRANSLATORS: Name of supernova SN 1572A and script name
		N_("Tycho's Supernova");
		// TRANSLATORS: Name of supernova SN 1604A
		N_("Kepler's Supernova");
		
		// Cardinals names
		N_("N");	// North
		N_("S");	// South
		N_("E");	// East
		N_("W");	// West

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
		N_("plutoid");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Planets");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Comets");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Asteroids");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Moons");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Plutoids");

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
		N_("Irregular galaxies");
		// TRANSLATORS: Type of objects (for "Lists" in the search tool)
		N_("Clusters associated with nebulosity");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Messier Catalogue");
		// TRANSLATORS: Catalogue of objects (for "Lists" in the search tool)
		N_("Caldwell Catalogue");
		
		// =====================================================================
		// Constellation cultures
		N_("Arabic");
		N_("Aztec");
		N_("Boorong");
		N_("Chinese");
		N_("Egyptian");
		N_("Inuit");
		N_("Indian Vedic");
		N_("Korean");
		N_("Lakota");
		N_("Maori");
		N_("Mongolian");
		N_("Navajo");
		N_("Norse");
		N_("Polynesian");
		N_("Romanian");
		N_("Sami");
		N_("Tupi-Guarani");
		N_("Tongan");
		N_("Western");

		
		// =====================================================================
		// Landscape names
		// TRANSLATORS: Name of landscape
		N_("Guereins");
		// TRANSLATORS: Name of landscape
		N_("Trees");
		// TRANSLATORS: Name of landscape and Earth's satellite
		N_("Moon");
		// TRANSLATORS: Landscape name: Hurricane Ridge
		N_("Hurricane");
		// TRANSLATORS: Name of landscape
		N_("Ocean");
		// TRANSLATORS: Landscape name: Garching bei Munchen
		N_("Garching");
		// TRANSLATORS: Name of landscape and planet
		N_("Mars");
		// TRANSLATORS: Name of landscape and planet
		N_("Saturn");
		// TRANSLATORS: Name of landscape
		N_("Geneva");
		// TRANSLATORS: Name of landscape
		N_("Grossmugl");


		
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
		N_("Analemma");
		// TRANSLATORS: Name of script
		N_("Sky Culture Tour");
		// TRANSLATORS: Name and description of script
		N_("Earth Events from Mercury");
		// TRANSLATORS: Name and description of script
		N_("Earth Events from a floating city on Venus");
		// TRANSLATORS: Name and description of script
		N_("Earth Events from Mars");
		// TRANSLATORS: Name of script
		N_("Earth and Venus Greatest Elongations and Brilliancies from Mars");
		// TRANSLATORS: Name of script
		N_("Earth and Mars Greatest Elongations and Transits from Callisto");
		
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
		N_("Screensaver of various happenings in the Solar System. 232 events in all!");
		N_("A tour of the western constellations.");
		N_("Look at the Sun from big planets of Solar System and Pluto.");
		N_("Best views of Earth from other Solar System bodies in the 21st Century.");
		N_("Transit of Venus as seen from Sydney Australia, 6th June 2012.");
		N_("A demonstration of the analemma - the path of the Sun across the sky during the year.");
		N_("Flash of the supernova observed by Tycho Brahe in 1572. The Supernovae plugin has to be enabled.");
		N_("Earth and Venus Greatest Elongations and Brilliancies from Mars 2000-3000");
		N_("Earth Greatest Elongations and Transits from Callisto 2000-3000. Why Callisto? Well of the 4 Galilean Moons, Callisto is the only one outside of Jupiter's radiation belt. Therefore, if humans ever colonize Jupiter's moons, Callisto will be the one.");
		
		// =====================================================================
		// List of countries
		N_("Andorra");
		N_("United Arab Emirates");
		N_("Afghanistan");
		N_("Antigua and Barbuda");
		N_("Anguilla");
		N_("Albania");
		N_("Armenia");
		N_("Netherlands Antilles");
		N_("Angola");
		N_("Antarctica");
		N_("Argentina");
		N_("American Samoa");
		N_("Austria");
		N_("Australia");
		N_("Aruba");
		N_("Azerbaijan");
		N_("Bosnia and Herzegowina");
		N_("Barbados");
		N_("Bangladesh");
		N_("Belgium");
		N_("Burkina Faso");
		N_("Bulgaria");
		N_("Bahrain");
		N_("Burundi");
		N_("Benin");
		N_("Bermuda");
		N_("Brunei Darussalam");
		N_("Bolivia");
		N_("Brazil");
		N_("Bahamas");
		N_("Bhutan");
		N_("Bouvet Island");
		N_("Botswana");
		N_("Belarus");
		N_("Belize");
		N_("Canada");
		N_("Cocos Islands");
		N_("Democratic Republic of the Congo");
		N_("Central African Republic");
		N_("Republic of the Congo");
		N_("Switzerland");
		N_("Cote d'Ivoire");
		N_("Cook Islands");
		N_("Chile");
		N_("Cameroon");
		N_("China");
		N_("Colombia");
		N_("Costa Rica");
		N_("Serbia and Montenegro");
		N_("Cuba");
		N_("Cape Verde");
		N_("Christmas Island");
		N_("Cyprus");
		N_("Czech Republic");
		N_("Germany");
		N_("Djibouti");
		N_("Denmark");
		N_("Dominica");
		N_("Dominican Republic");
		N_("Algeria");
		N_("Ecuador");
		N_("Estonia");
		N_("Egypt");
		N_("Western Sahara");
		N_("Eritrea");
		N_("Spain");
		N_("Ethiopia");
		N_("Finland");
		N_("Fiji");
		N_("Falkland Islands");
		N_("Micronesia");
		N_("Faroe Islands");
		N_("France");
		N_("Gabon");
		N_("United Kingdom");
		N_("Grenada");
		N_("Georgia");
		N_("French Guiana");
		N_("Ghana");
		N_("Gibraltar");
		N_("Greenland");
		N_("Gambia");
		N_("Guinea");
		N_("Guadeloupe");
		N_("Equatorial Guinea");
		N_("Greece");
		N_("South Georgia and the South Sandwich Islands");
		N_("Guatemala");
		N_("Guam");
		N_("Guinea-Bissau");
		N_("Guyana");
		N_("Hong Kong");
		N_("Heard Island and McDonald Islands");
		N_("Honduras");
		N_("Croatia");
		N_("Haiti");
		N_("Hungary");
		N_("Indonesia");
		N_("Ireland");
		N_("Israel");
		N_("India");
		N_("British Indian Ocean Territory");
		N_("Iraq");
		N_("Iran");
		N_("Iceland");
		N_("Italy");
		N_("Jamaica");
		N_("Jordan");
		N_("Japan");
		N_("Kenya");
		N_("Kyrgyzstan");
		N_("Cambodia");
		N_("Kiribati");
		N_("Comoros");
		N_("Saint Kitts and Nevis");
		N_("Democratic People's Republic of Korea");
		N_("Republic of Korea");
		N_("Kuwait");
		N_("Cayman Islands");
		N_("Kazakhstan");
		N_("Lao");
		N_("Lebanon");
		N_("Saint Lucia");
		N_("Liechtenstein");
		N_("Sri Lanka");
		N_("Liberia");
		N_("Lesotho");
		N_("Lithuania");
		N_("Luxembourg");
		N_("Latvia");
		N_("Libyan Arab Jamahiriya");
		N_("Morocco");
		N_("Monaco");
		N_("Moldova");
		N_("Madagascar");
		N_("Marshall Islands");
		N_("Macedonia");
		N_("Mali");
		N_("Myanmar");
		N_("Mongolia");
		N_("Macau");
		N_("Northern Mariana Islands");
		N_("Martinique");
		N_("Mauritania");
		N_("Montserrat");
		N_("Malta");
		N_("Mauritius");
		N_("Maldives");
		N_("Malawi");
		N_("Mexico");
		N_("Malaysia");
		N_("Mozambique");
		N_("Namibia");
		N_("New Caledonia");
		N_("Niger");
		N_("Norfolk Island");
		N_("Nigeria");
		N_("Nicaragua");
		N_("Netherlands");
		N_("Norway");
		N_("Nepal");
		N_("Nauru");
		N_("Niue");
		N_("New Zealand");
		N_("Oman");
		N_("Panama");
		N_("Peru");
		N_("French Polynesia");
		N_("Papua New Guinea");
		N_("Philippines");
		N_("Pakistan");
		N_("Poland");
		N_("Saint Pierre and Miquelon");
		N_("Pitcairn");
		N_("Puerto Rico");
		N_("Palestinian Territories");
		N_("Portugal");
		N_("Palau");
		N_("Paraguay");
		N_("Qatar");
		N_("Réunion");
		N_("Romania");
		N_("Serbia");
		N_("Russian Federation");
		N_("Rwanda");
		N_("Saudi Arabia");
		N_("Solomon Islands");
		N_("Seychelles");
		N_("Sudan");
		N_("Sweden");
		N_("Singapore");
		N_("Saint Helena");
		N_("Slovenia");
		N_("Svalbard and Jan Mayen");
		N_("Slovakia");
		N_("Sierra Leone");
		N_("San Marino");
		N_("Senegal");
		N_("Somalia");
		N_("Suriname");
		N_("Sao Tome and Principe");
		N_("El Salvador");
		N_("Syrian Arab Republic");
		N_("Swaziland");
		N_("Turks and Caicos Islands");
		N_("Chad");
		N_("French Southern Territories");
		N_("Togo");
		N_("Thailand");
		N_("Tajikistan");
		N_("Tokelau");
		N_("East Timor");
		N_("Turkmenistan");
		N_("Tunisia");
		N_("Tonga");
		N_("Turkey");
		N_("Trinidad and Tobago");
		N_("Tuvalu");
		N_("Taiwan");
		N_("Tanzania");
		N_("Ukraine");
		N_("Uganda");
		N_("United States Minor Outlying Islands");
		N_("United States");
		N_("Uruguay");
		N_("Uzbekistan");
		N_("Vatican City State");
		N_("Saint Vincent and the Grenadines");
		N_("Venezuela");
		N_("British Virgin Islands");
		N_("United States Virgin Islands");
		N_("Viet Nam");
		N_("Vanuatu");
		N_("Wallis and Futuna");
		N_("Samoa");
		N_("Yemen");
		N_("Mayotte");
		N_("Yugoslavia");
		N_("South Africa");
		N_("Zambia");
		N_("Zimbabwe");

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
		N_("Files of type:");
		N_("Date Modified");
		N_("Directories");
		N_("Computer");
		N_("&Open");
		N_("File &name:");
		N_("Copy &Link Location");
	}
};
