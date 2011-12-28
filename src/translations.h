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
		
		// TRANSLATORS: Name of supernova SN 1572A
		N_("Tycho's Supernova");
		// TRANSLATORS: Name of supernova SN 1604A
		N_("Kepler's Supernova");
		
		
		// Cardinals names
		N_("N");	// North
		N_("S");	// South
		N_("E");	// East
		N_("W");	// West

		// Constellation cultures
		N_("Arabic");
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

		// Landscapes names
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
	}
};
