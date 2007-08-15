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
		assert(0);
		
		// Planets and satellites from ssystem.ini
		N_("Sun");
		N_("Mercury");
		N_("Venus");
		N_("Earth");
		N_("Moon");
		N_("Mars");
		N_("Deimos");
		N_("Phobos");
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
		N_("Pluto");
		N_("Charon");
		N_("Solar System Observer");
		
		// Cardinals names
		N_("N");	// North
		N_("S");	// South
		N_("E");	// East
		N_("W");	// West

		// Constellation cultures
		N_("Chinese");
		N_("Egyptian");
		N_("Inuit");
		N_("Korean");
		N_("Lakota");
		N_("Navajo");
		N_("Norse");
		N_("Polynesian");
		N_("Western");
		
		// Western Constellations
		N_("Aquila");				// aql
		N_("Andromeda");			// and
		N_("Sculptor");			// scl
		N_("Ara");				// ara
		N_("Libra");				// lib
		N_("Cetus");				// cet
		N_("Aries");				// ari
		N_("Scutum");				// sct
		N_("Pyxis");				// pyx
		N_("Bootes");				// boo
		N_("Caelum");				// cae
		N_("Chamaeleon");			// cha
		N_("Cancer");				// cnc
		N_("Capricornus");		// cap
		N_("Carina");				// car
		N_("Cassiopeia");			// cas
		N_("Centaurus");			// cen
		N_("Cepheus");			// cep
		N_("Coma Berenices");		// com
		N_("Canes Venatici");		// cvn
		N_("Auriga");				// aur
		N_("Columba");			// col
		N_("Circinus");			// cir
		N_("Crater");				// crt
		N_("Corona Australis");	// cra
		N_("Corona Borealis");	// crb
		N_("Corvus");				// crv
		N_("Crux");				// cru
		N_("Cygnus");				// cyg
		N_("Delphinus");			// del
		N_("Dorado");				// dor
		N_("Draco");				// dra
		N_("Norma");				// nor
		N_("Eridanus");			// eri
		N_("Sagitta");			// sge
		N_("Fornax");				// for
		N_("Gemini");				// gem
		N_("Camelopardalis");		// cam
		N_("Canis Major");		// cma
		N_("Ursa Major");			// uma
		N_("Grus");				// gru
		N_("Hercules");			// her
		N_("Horologium");			// hor
		N_("Hydra");				// hya
		N_("Hydrus");				// hyi
		N_("Indus");				// ind
		N_("Lacerta");			// lac
		N_("Monoceros");			// mon
		N_("Lepus");				// lep
		N_("Leo");				// leo
		N_("Lupus");				// lup
		N_("Lynx");				// lyn
		N_("Lyra");				// lyr
		N_("Antlia");				// ant
		N_("Microscopium");		// mic
		N_("Musca");				// mus
		N_("Octans");				// oct
		N_("Apus");				// aps
		N_("Ophiuchus");			// oph
		N_("Orion");				// ori
		N_("Pavo");				// pav
		N_("Pegasus");			// peg
		N_("Pictor");				// pic
		N_("Perseus");			// per
		N_("Equuleus");			// equ
		N_("Canis Minor");		// cmi
		N_("Leo Minor");			// lmi
		N_("Vulpecula");			// vul
		N_("Ursa Minor");			// umi
		N_("Phoenix");			// phe
		N_("Piscis Austrinus");	// psc
		N_("Pisces");				// psa
		N_("Volans");				// vol
		N_("Puppis");				// pup
		N_("Reticulum");			// ret
		N_("Sagittarius");		// sgr
		N_("Scorpius");			// sco
		N_("Scutum");				// sct
		N_("Serpens");			// ser
		N_("Sextans");			// sex
		N_("Mensa");				// men
		N_("Taurus");				// tau
		N_("Telescopium");		// tel
		N_("Tucana");				// tuc
		N_("Triangulum");			// tri
		N_("Triangulum Australe");// tra
		N_("Aquarius");			// aqr
		N_("Virgo");				// vir
		N_("Vela");				// vel

		// Egyptian Constellations
        N_("Bull's Foreleg");   // 001
        N_("Two Poles");        // 002
        N_("Lion");     // 003
        N_("Two Jaws"); // 004
        N_("Sah");      // 005
        N_("Bird");     // 006
        N_("Sek");      // 007
        N_("Triangle"); // 008
        N_("Ferry Boat");       // 009
        N_("Boat");     // 010
        N_("Crocodile");        // 011
        N_("Selkis");   // 012
        N_("Prow");     // 013
        N_("Horus");    // 014
        N_("Sheepfold");        // 015
        N_("Giant");    // 016
        N_("Hippopotamus");     // 017
        N_("Flock");    // 018
        N_("Pair of Stars");    // 019
        N_("Khanuwy Fish");     // 020
        N_("Net");      // 021
        N_("Jaw");      // 022
        N_("Mooring Post");     // 023
        N_("Kenemet");  // 024
        N_("Chematy");  // 025
        N_("Waty Bekety");      // 026
        N_("Sheep");    // 027
        N_("Stars of Water");   // 028

		// Chinese Constellations
        N_("Northern Dipper");  // 001
        N_("Curved Array");     // 002
        N_("Coiled Thong");     // 003
        N_("Wings");    // 004
        N_("Chariot");  // 005
        N_("Tail");     // 006
        N_("Winnowing Basket"); // 007
        N_("Dipper");   // 008
        N_("Drum");     // 009
        N_("Three Steps");      // 010
        N_("Imperial Guards");  // 011
        N_("Horn");     // 012
        N_("Willow");   // 013
        N_("Imperial Passageway");      // 014
        N_("Kitchen");  // 015
        N_("River Turtle");     // 016
        N_("Stomach");  // 017
        N_("Great General");    // 018
        N_("Wall");     // 019
        N_("Legs");     // 020
        N_("Root");     // 021
        N_("Ramparts"); // 022
        N_("Flying Corridor");  // 023
        N_("Outer Fence");      // 024
        N_("Ford");     // 025
        N_("Seven Excellencies");       // 026
        N_("Market");   // 028
        N_("Five Chariots");    // 030
        N_("Rolled Tongue");    // 031
        N_("Net");      // 032
        N_("Toilet");   // 033
        N_("Screen");   // 034
        N_("Soldiers Market");  // 035
        N_("Square Granary");   // 036
        N_("Three Stars");      // 037
        N_("Four Channels");    // 038
        N_("Well");     // 039
        N_("South River");      // 040
        N_("North River");      // 041
        N_("Five Feudal Kings");        // 042
        N_("Orchard");  // 043
        N_("Meadows");  // 044
        N_("Circular Granary"); // 045
        N_("Purple Palace");    // 046
        N_("Extended Net");     // 047
        N_("Arsenal");  // 048
        N_("Hook");     // 049
        N_("Supreme Palace");   // 050
        N_("Jade Well");        // 051
        N_("Lance");    // 052
        N_("Boat");     // 053
        N_("Mausoleum");        // 054
        N_("Dog");      // 055
        N_("Earth God's Temple");       // 056
        N_("Bow and Arrow");    // 057
        N_("Pestle");   // 058
        N_("Mortar");   // 059
        N_("Rooftop");  // 060
        N_("Thunderbolt");      // 061
        N_("Chariot Yard");     // 062
        N_("Good Gourd");       // 063
        N_("Rotten Gourd");     // 064
        N_("Encampment");       // 065
        N_("Thunder and Lightning");    // 066
        N_("Palace Gate");      // 067
        N_("Emptiness");        // 068
        N_("Weaving Girl");     // 069
        N_("Girl");     // 070
        N_("Ox");       // 071
        N_("Heart");    // 072
        N_("Room");     // 073
        N_("Spring");   // 074
        N_("Establishment");    // 075
        N_("Flail");    // 076
        N_("Spear");    // 077
        N_("Right Flag");       // 078
        N_("Left Flag");        // 079
        N_("Drumstick");        // 080
        N_("Bond");     // 081
        N_("Woman's Bed");      // 082
        N_("Western Door");     // 083
        N_("Eastern Door");     // 084
        N_("Farmland"); // 085
        N_("Star");     // 086
        N_("Ghosts");   // 087
        N_("Xuanyuan"); // 088
        N_("Tripod");   // 089
        N_("Neck");     // 090
        N_("Zaofu");    // 091
        N_("Market Officer");   // 092
        N_("Banner of Three Stars");    // 093

		// Polynesian Constellations
        N_("Bailer");   // 001
        N_("Cat's Cradle");     // 002
        N_("Voice of Joy");     // 004
        N_("The Seven");        // 005
        N_("Maui's Fishhook");  // 006
        N_("Navigator's Triangle");     // 007
        N_("Kite of Kawelo");   // 008
        N_("Frigate Bird");     // 009
        N_("Cared for by Moon");        // 010
        N_("Dolphin");  // 011

		// Navajo Constellations
        N_("Revolving Male");   // 001
        N_("Revolving Female");     // 002
        N_("Man with Feet Apart");     // 003
        N_("Lizard");        // 004
        N_("Dilyehe");        // 005
        N_("First Big One");  // 006
        N_("Rabbit Tracks");     // 007
        N_("First Slim One");   // 008

		// Norse Constellations
        N_("Aurvandil's Toe");   // 001
        N_("Wolf's Mouth");     // 002
        N_("The Fishermen");     // 003
        N_("Woman's Cart");        // 004
        N_("Man's Cart");        // 005
        N_("The Asar Battlefield");  // 006

		// Inuit Constellations
        N_("Two Sunbeams");   // 001
        N_("Two Placed Far Apart");     // 002
        N_("Dogs");     // 003
        N_("Collarbones");        // 004
        N_("Lamp Stand");        // 005
        N_("Caribou");        // 006
        N_("Two in Front");     // 007
        N_("Breastbone");   // 008
        N_("Runners");   // 009
        N_("Blubber Container");   // 010

		// Lakota Constellations
        N_("Hand");   // 001
        N_("Snake");     // 002
        N_("Fireplace");     // 003
        N_("Dipper");        // 004
        N_("Race Track");        // 005
        N_("Animal");  // 006
        N_("Elk");     // 007
        N_("Seven Little Girls");   // 008
        N_("Dried Willow");   // 009
        N_("Salamander");   // 010
        N_("Turtle");   // 011
        N_("Thunderbird");   // 012
        N_("Bear's Lodge");   // 013

		// Korean Constellations
		// TODO

		// TUI script message
		N_("Select and exit to run.");
		N_("USB Script: ");  // Placeholder for translating - Rob
	}
}
