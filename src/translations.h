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
		N_("Tupi-Guarani");
		
		// Western Constellations
		N_("Aquila");			// aql
		N_("Andromeda");		// and
		N_("Sculptor");			// scl
		N_("Ara");			// ara
		N_("Libra");			// lib
		N_("Cetus");			// cet
		N_("Aries");			// ari
		N_("Scutum");			// sct
		N_("Pyxis");			// pyx
		N_("Bootes");			// boo
		N_("Caelum");			// cae
		N_("Chamaeleon");		// cha
		N_("Cancer");			// cnc
		N_("Capricornus");		// cap
		N_("Carina");			// car
		N_("Cassiopeia");		// cas
		N_("Centaurus");		// cen
		N_("Cepheus");			// cep
		N_("Coma Berenices");		// com
		N_("Canes Venatici");		// cvn
		N_("Auriga");			// aur
		N_("Columba");			// col
		N_("Circinus");			// cir
		N_("Crater");			// crt
		N_("Corona Australis");		// cra
		N_("Corona Borealis");		// crb
		N_("Corvus");			// crv
		N_("Crux");			// cru
		N_("Cygnus");			// cyg
		N_("Delphinus");		// del
		N_("Dorado");			// dor
		N_("Draco");			// dra
		N_("Norma");			// nor
		N_("Eridanus");			// eri
		N_("Sagitta");			// sge
		N_("Fornax");			// for
		N_("Gemini");			// gem
		N_("Camelopardalis");		// cam
		N_("Canis Major");		// cma
		N_("Ursa Major");		// uma
		N_("Grus");			// gru
		N_("Hercules");			// her
		N_("Horologium");		// hor
		N_("Hydra");			// hya
		N_("Hydrus");			// hyi
		N_("Indus");			// ind
		N_("Lacerta");			// lac
		N_("Monoceros");		// mon
		N_("Lepus");			// lep
		N_("Leo");			// leo
		N_("Lupus");			// lup
		N_("Lynx");			// lyn
		N_("Lyra");			// lyr
		N_("Antlia");			// ant
		N_("Microscopium");		// mic
		N_("Musca");			// mus
		N_("Octans");			// oct
		N_("Apus");			// aps
		N_("Ophiuchus");		// oph
		N_("Orion");			// ori
		N_("Pavo");			// pav
		N_("Pegasus");			// peg
		N_("Pictor");			// pic
		N_("Perseus");			// per
		N_("Equuleus");			// equ
		N_("Canis Minor");		// cmi
		N_("Leo Minor");		// lmi
		N_("Vulpecula");		// vul
		N_("Ursa Minor");		// umi
		N_("Phoenix");			// phe
		N_("Piscis Austrinus");		// psc
		N_("Pisces");			// psa
		N_("Volans");			// vol
		N_("Puppis");			// pup
		N_("Reticulum");		// ret
		N_("Sagittarius");		// sgr
		N_("Scorpius");			// sco
		N_("Scutum");			// sct
		N_("Serpens");			// ser
		N_("Sextans");			// sex
		N_("Mensa");			// men
		N_("Taurus");			// tau
		N_("Telescopium");		// tel
		N_("Tucana");			// tuc
		N_("Triangulum");		// tri
		N_("Triangulum Australe");	// tra
		N_("Aquarius");			// aqr
		N_("Virgo");			// vir
		N_("Vela");			// vel

		// Egyptian Constellations
		N_("Bull's Foreleg");		// 001
		N_("Two Poles");		// 002
		N_("Lion");			// 003
		N_("Two Jaws");			// 004
		N_("Sah");			// 005
		N_("Bird");			// 006
		N_("Sek");			// 007
		N_("Triangle");			// 008
		N_("Ferry Boat");		// 009
		N_("Boat");			// 010
		N_("Crocodile");		// 011
		N_("Selkis");			// 012
		N_("Prow");			// 013
		N_("Horus");			// 014
		N_("Sheepfold");		// 015
		N_("Giant");			// 016
		N_("Hippopotamus");		// 017
		N_("Flock");			// 018
		N_("Pair of Stars");		// 019
		N_("Khanuwy Fish");		// 020
		N_("Net");			// 021
		N_("Jaw");			// 022
		N_("Mooring Post");		// 023
		N_("Kenemet");			// 024
		N_("Chematy");			// 025
		N_("Waty Bekety");		// 026
		N_("Sheep");			// 027
		N_("Stars of Water");		// 028

		// Chinese Constellations
		N_("Northern Dipper");		// 001
		N_("Curved Array");		// 002
		N_("Coiled Thong");		// 003
		N_("Wings");			// 004
		N_("Chariot");			// 005
		N_("Tail");			// 006
		N_("Winnowing Basket");		// 007
		N_("Dipper");			// 008
		N_("Drum");			// 009
		N_("Three Steps");		// 010
		N_("Imperial Guards");		// 011
		N_("Horn");			// 012
		N_("Willow");			// 013
		N_("Imperial Passageway");	// 014
		N_("Kitchen");			// 015
		N_("River Turtle");		// 016
		N_("Stomach");			// 017
		N_("Great General");		// 018
		N_("Wall");			// 019
		N_("Legs");			// 020
		N_("Root");			// 021
		N_("Ramparts");			// 022
		N_("Flying Corridor");		// 023
		N_("Outer Fence");		// 024
		N_("Ford");			// 025
		N_("Seven Excellencies");	// 026
		N_("Market");			// 028
		N_("Five Chariots");		// 030
		N_("Rolled Tongue");		// 031
		N_("Net");			// 032
		N_("Toilet");			// 033
		N_("Screen");			// 034
		N_("Soldiers Market");		// 035
		N_("Square Granary");		// 036
		N_("Three Stars");		// 037
		N_("Four Channels");		// 038
		N_("Well");			// 039
		N_("South River");		// 040
		N_("North River");		// 041
		N_("Five Feudal Kings");	// 042
		N_("Orchard");			// 043
		N_("Meadows");			// 044
		N_("Circular Granary");		// 045
		N_("Purple Palace");		// 046
		N_("Extended Net");		// 047
		N_("Arsenal");			// 048
		N_("Hook");			// 049
		N_("Supreme Palace");		// 050
		N_("Jade Well");		// 051
		N_("Lance");			// 052
		N_("Boat");			// 053
		N_("Mausoleum");		// 054
		N_("Dog");			// 055
		N_("Earth God's Temple");	// 056
		N_("Bow and Arrow");		// 057
		N_("Pestle");			// 058
		N_("Mortar");			// 059
		N_("Rooftop");			// 060
		N_("Thunderbolt");		// 061
		N_("Chariot Yard");		// 062
		N_("Good Gourd");		// 063
		N_("Rotten Gourd");		// 064
		N_("Encampment");		// 065
		N_("Thunder and Lightning");	// 066
		N_("Palace Gate");		// 067
		N_("Emptiness");		// 068
		N_("Weaving Girl");		// 069
		N_("Girl");			// 070
		N_("Ox");			// 071
		N_("Heart");			// 072
		N_("Room");			// 073
		N_("Spring");			// 074
		N_("Establishment");		// 075
		N_("Flail");			// 076
		N_("Spear");			// 077
		N_("Right Flag");		// 078
		N_("Left Flag");		// 079
		N_("Drumstick");		// 080
		N_("Bond");			// 081
		N_("Woman's Bed");		// 082
		N_("Western Door");		// 083
		N_("Eastern Door");		// 084
		N_("Farmland");			// 085
		N_("Star");			// 086
		N_("Ghosts");			// 087
		N_("Xuanyuan");			// 088
		N_("Tripod");			// 089
		N_("Neck");			// 090
		N_("Zaofu");			// 091
		N_("Market Officer");		// 092
		N_("Banner of Three Stars");	// 093

		// Polynesian Constellations
		N_("Bailer");			// 001
		N_("Cat's Cradle");		// 002
		N_("Voice of Joy");		// 004
		N_("The Seven");		// 005
		N_("Maui's Fishhook");		// 006
		N_("Navigator's Triangle");	// 007
		N_("Kite of Kawelo");		// 008
		N_("Frigate Bird");		// 009
		N_("Cared for by Moon");	// 010
		N_("Dolphin");			// 011

		// Navajo Constellations
		N_("Revolving Male");		// 001
		N_("Revolving Female");		// 002
		N_("Man with Feet Apart");	// 003
		N_("Lizard");			// 004
		N_("Dilyehe");			// 005
		N_("First Big One");		// 006
		N_("Rabbit Tracks");		// 007
		N_("First Slim One");		// 008

		// Norse Constellations
		N_("Aurvandil's Toe");		// 001
		N_("Wolf's Mouth");		// 002
		N_("The Fishermen");		// 003
		N_("Woman's Cart");		// 004
		N_("Man's Cart");		// 005
		N_("The Asar Battlefield");	// 006

		// Inuit Constellations
		N_("Two Sunbeams");		// 001
		N_("Two Placed Far Apart");	// 002
		N_("Dogs");			// 003
		N_("Collarbones");		// 004
		N_("Lamp Stand");		// 005
		N_("Caribou");			// 006
		N_("Two in Front");		// 007
		N_("Breastbone");		// 008
		N_("Runners");			// 009
		N_("Blubber Container");	// 010

		// Lakota Constellations
		N_("Hand");			// 001
		N_("Snake");			// 002
		N_("Fireplace");		// 003
		N_("Dipper");			// 004
		N_("Race Track");		// 005
		N_("Animal");			// 006
		N_("Elk");			// 007
		N_("Seven Little Girls");	// 008
		N_("Dried Willow");		// 009
		N_("Salamander");		// 010
		N_("Turtle");			// 011
		N_("Thunderbird");		// 012
		N_("Bear's Lodge");		// 013

		// Korean Constellations
		N_("JuJeong");			// 001
		N_("CheonJeon");		// 002
		N_("JwaGak");			// 003
		N_("JinHyeon");			// 004
		N_("CheonMun");			// 005
		N_("Pyeong");			// 006
		N_("SeopJae");			// 007
		N_("SeopJae");			// 008
		N_("DaeGaak");			// 009
		N_("Hang");			// 010
		N_("JeolWii");			// 011
		N_("DuunWaan");			// 012
		N_("ChoYo");			// 013
		N_("GyeongHaa");		// 014
		N_("JaeSeok");			// 015
		N_("HangJii");			// 016
		N_("CheonYuu");			// 017
		N_("Jeo");			// 018
		N_("JinGeo");			// 019
		N_("CheonPouk");		// 020
		N_("GiJinJangGuun");		// 021
		N_("GiGwan");			// 022
		N_("GeoGii");			// 023
		N_("DongHaam");			// 024
		N_("GeonPae");			// 025
		N_("GuGeom");			// 026
		N_("Beol");			// 027
		N_("Baang");			// 028
		N_("JongGwan");			// 029
		N_("iil");			// 030
		N_("SeoHaam");			// 031
		N_("Shim");			// 032
		N_("JeokJol");			// 033
		N_("CheonGang");		// 034
		N_("BuYeol");			// 035
		N_("Eo");			// 036
		N_("Ku");			// 037
		N_("Mii");			// 038
		N_("ShinGuung");		// 039
		N_("Kii");			// 040
		N_("WaeJeo");			// 041
		N_("Gaang");			// 042
		N_("CheonByeon");		// 043
		N_("Geon");			// 044
		N_("CheonGae");			// 045
		N_("NaamDoo");			// 046
		N_("Goo");			// 047
		N_("GuuGuuk");			// 048
		N_("NongJaang_iin");		// 049
		N_("Byeol");			// 050
		N_("YeonDo");			// 051
		N_("JikNyeo");			// 052
		N_("JaamDae");			// 053
		N_("JwaGi");			// 054
		N_("HaGo");			// 055
		N_("CheonBu");			// 056
		N_("UuGi");			// 057
		N_("GyeonUu");			// 058
		N_("NaaEon");			// 059
		N_("BuuGwang");			// 060
		N_("HaeJuung");			// 061
		N_("CheonJin");			// 062
		N_("Gwa");			// 063
		N_("PaeGwa");			// 064
		N_("iiJuu");			// 065
		N_("SuuNyeo");			// 066
		N_("SaaBii");			// 067
		N_("SaaWii");			// 068
		N_("SaaRok");			// 069
		N_("SaaMyeong");		// 070
		N_("Heo");			// 071
		N_("Gok");			// 072
		N_("Eup");			// 073
		N_("CheonRuSeong");		// 074
		N_("Guu");			// 075
		N_("ChoBo");			// 076
		N_("GeoBuu");			// 077
		N_("iinSeong");			// 078
		N_("NaeJeo");			// 079
		N_("Gu");			// 080
		N_("Wii");			// 081
		N_("BunMyo");			// 082
		N_("GaeOok");			// 083
		N_("PaeGuu");			// 084
		N_("DeungSaa");			// 085
		N_("Shil");			// 086
		N_("iiGuung");			// 087
		N_("TouGongRi");		// 088
		N_("JeonNwoe");			// 089
		N_("NuByeokJin");		// 090
		N_("URimGuun");			// 091
		N_("BukRakSaMuun");		// 092
		N_("CheonMang");		// 093
		N_("CheonGu");			// 094
		N_("DongByeok");		// 095
		N_("TouGong");			// 096
		N_("ByeokRyeok");		// 097
		N_("UunUu");			// 098
		N_("GaakDo");			// 099
		N_("Chaek");			// 100
		N_("WaangRaang");		// 101
		N_("BuRo");			// 102
		N_("Gyu");			// 103
		N_("GuunNaamMun");		// 104
		N_("WaeByeong");		// 105
		N_("CheonHoun");		// 106
		N_("SaaGong");			// 107
		N_("CheonJaangGuun");		// 108
		N_("Ruu");			// 109
		N_("JwaGyeong");		// 110
		N_("UuGyeong");			// 111
		N_("CheonChang");		// 112
		N_("CheonYu");			// 113
		N_("CheonSeon");		// 114
		N_("JeokSuu");			// 115
		N_("DaeReung");			// 116
		N_("JeokShi");			// 117
		N_("Wii");			// 118
		N_("CheonReum");		// 119
		N_("CheonGyun");		// 120
		N_("RyeoSeok");			// 121
		N_("GwonSeol");			// 122
		N_("CheonCham");		// 123
		N_("Myo");			// 124
		N_("Wol");			// 125
		N_("CheonAa");			// 126
		N_("CheonEum");			// 127
		N_("ChuuGo");			// 128
		N_("Cheonwon");			// 129
		N_("OoGeo");			// 130
		N_("HaamJii");			// 131
		N_("Ju");			// 132
		N_("Ju");			// 133
		N_("Ju");			// 134
		N_("CheonHwang");		// 135
		N_("JaeWaang");			// 136
		N_("CheonGwan");		// 137
		N_("SaamGii");			// 138
		N_("CheonGa");			// 139
		N_("CheonGo");			// 140
		N_("Piil");			// 141
		N_("CheonJeol");		// 142
		N_("GuuYuu");			// 143
		N_("GuJuJuGu");			// 144
		N_("CheonWon");			// 145
		N_("JwaGii");			// 146
		N_("SaaGwae");			// 147
		N_("Zaa");			// 148
		N_("Saam");			// 149
		N_("Beol");			// 150
		N_("GuunJeong");		// 151
		N_("OkJeong");			// 152
		N_("Byeong");			// 153
		N_("Cheuk");			// 154
		N_("Shii");			// 155
		N_("JeokShin");			// 156
		N_("JeokSuu");			// 157
		N_("BuukHa");			// 158
		N_("OJeHuu");			// 159
		N_("CheonJuun");		// 160
		N_("DongJeong");		// 161
		N_("SuuBuu");			// 162
		N_("SuuWii");			// 163
		N_("SaaDok");			// 164
		N_("NamHa");			// 165
		N_("GwolGu");			// 166
		N_("Ho");			// 167
		N_("RangSeong");		// 168
		N_("YaGae");			// 169
		N_("GuunShii");			// 170
		N_("Soun");			// 171
		N_("Za");			// 172
		N_("Zaang_iin");		// 173
		N_("Noin");			// 174
		N_("Gwan");			// 175
		N_("Kwii");			// 176
		N_("JeokShii");			// 177
		N_("CheonGu");			// 178
		N_("WaeJuu");			// 179
		N_("CheonGu");			// 180
		N_("CheonSa");			// 181
		N_("JuuGii");			// 182
		N_("Ryu");			// 183
		N_("NaePyeong");		// 184
		N_("HeonWon");			// 185
		N_("Seong");			// 186
		N_("Jiik");			// 187
		N_("Jaang");			// 188
		N_("CheonMyo");			// 189
		N_("iik");			// 190
		N_("DonGu");			// 191
		N_("Jin");			// 192
		N_("JangSaa");			// 193
		N_("GuunMuun");			// 194
		N_("TouSaaGong");		// 195
		N_("NangJaang");		// 196
		N_("SaamTae");			// 197
		N_("OZehu");			// 198
		N_("SangJiin");			// 199
		N_("SoMii");			// 200
		N_("TaeMi");			// 201
		N_("TaeMi");			// 202
		N_("GuGyeong");			// 203
		N_("HoBuun");			// 204
		N_("TeaJaa");			// 205
		N_("JongGwan");			// 206
		N_("OJe");			// 207
		N_("Byeong");			// 208
		N_("SaamGongNaeJwa");		// 209
		N_("AlZaa");			// 210
		N_("MyeongDang");		// 211
		N_("YeongDae");			// 212
		N_("YeoEoGwan");		// 213
		N_("JeonSaa");			// 214
		N_("PalGok");			// 215
		N_("CheonBae");			// 216
		N_("SamGong");			// 217
		N_("SamGong");			// 218
		N_("BukDuu");			// 219
		N_("Bo");			// 220
		N_("EumDeok");			// 221
		N_("Cheon_iil");		// 222
		N_("Tae_iil");			// 223
		N_("NaeJuu");			// 224
		N_("HyeonGwa");			// 225
		N_("Saang");			// 226
		N_("TaeYaangSuu");		// 227
		N_("CheonRyoe");		// 228
		N_("MunChang");			// 229
		N_("NaeGae");			// 230
		N_("CheonChang");		// 231
		N_("HwaGae");			// 232
		N_("OJeJwa");			// 233
		N_("YuukGaap");			// 234
		N_("CheonJu");			// 235
		N_("BukGeuk");			// 236
		N_("GuJiin");			// 237
		N_("Gaang");			// 238
		N_("CheonHwangTaeJae");		// 239
		N_("SaangSeo");			// 240
		N_("CheonJu");			// 241
		N_("JuHaSa");			// 242
		N_("YeoSa");			// 243
		N_("YeoSaang");			// 244
		N_("DaeRii");			// 245
		N_("JaMi");			// 246
		N_("JaMi");			// 247
		N_("CheonGii");			// 248
		N_("ChilGong");			// 249
		N_("GwanSak");			// 250
		N_("CheonShi");			// 251
		N_("CheonShi");			// 252
		N_("Huu");			// 253
		N_("JaeJwa");			// 254
		N_("JongSeong");		// 255
		N_("JongJeong");		// 256
		N_("Jong_iin");			// 257
		N_("JongDaeBuu");		// 258
		N_("ShiRuu");			// 259
		N_("Gok");			// 260
		N_("Duu");			// 261
		N_("HwanJaa");			// 262
		N_("GeoSaa");			// 263
		N_("BaekTaak");			// 264
		N_("DoSaa");			// 265
		N_("YeolSaa");			// 266
		N_("JwaHaal");			// 267
		N_("UuHaal");			// 268
		N_("JwaJipBeop");		// 269
		N_("UuJipBeop");		// 270
		N_("HeoRyang");			// 271
		N_("CheonJeon");		// 272

		// Tupi-Guarani Constellations
		N_("Ema");			// 001
		N_("Homem");			// 002
		N_("Anta");			// 003
		N_("Veado");			// 004
		N_("Joykexo");			// 005
		N_("Vespeiro");			// 006
		N_("Queixada");			// 007

		// TUI script message
		N_("Select and exit to run.");
		N_("USB Script: ");  // Placeholder for translating - Rob
	}
};
