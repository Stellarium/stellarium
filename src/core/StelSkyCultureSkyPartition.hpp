/*
 * Stellarium
 * Copyright (C) 2025 Georg Zotti
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

#ifndef STELSKYCULTURESKYPARTITION_HPP
#define STELSKYCULTURESKYPARTITION_HPP

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>

#include "StelCore.hpp"
#include "StelObject.hpp"
//#include "StelModule.hpp"

class StelTranslator;
class SkyLine;
class QJsonObject;

//! @class StelSkyCultureSkyPartition
//! An optional skyculture element, configurable in the index.json.
//! Currently Stellarium supports two optional such systems per skyculture: Zodiac and LunarSystem.
//! "zodiac": {
//! "name": { "native": "Signa Zodiacalia", "english": "Zodiacal signs"},
//! "partitions": [ 12, 30],
//! "extent": 8,
//! "names": [ { "symbol": "\u2648", "native": "Aries",       "english": "Ram"},
//! 	       { "symbol": "\u2649", "native": "Taurus",      "english": "Bull"},
//! 	       { "symbol": "\u264A", "native": "Gemini",      "english": "Twins"},
//! 	       { "symbol": "\u264B", "native": "Cancer",      "english": "Crab"},
//! 	       { "symbol": "\u264C", "native": "Leo",         "english": "Lion"},
//! 	       { "symbol": "\u264D", "native": "Virgo",       "english": "Virgin"},
//! 	       { "symbol": "\u264E", "native": "Libra",       "english": "Scales"},
//! 	       { "symbol": "\u264F", "native": "Scorpio",     "english": "Scorpion"},
//! 	       { "symbol": "\u2650", "native": "Capricornus", "english": "Capricorn"},
//! 	       { "symbol": "\u2651", "native": "Sagittarius", "english": "Archer"},
//! 	       { "symbol": "\u2652", "native": "Aquarius",    "english": "Water bearer"},
//! 	       { "symbol": "\u2653", "native": "Pisces",      "english": "Fishes"}]},
//! "lunar_system": {
//! "name": "Nakshatras",
//! "partitions": [ 27, 4],
//! "extent": 5,
//! "link": { "star": 65474, "offset": 180},
//! "names": [ { "symbol": "LS01", "native": "अश्विनी",                          "pronounce": "Ashvini",       "english": "Physician to the Gods"},
//! 	       { "symbol": "LS02", "native": "भरणी",                           "pronounce": "Bharani",        "english": "Bearer"},
//! 	       { "symbol": "LS03", "native": "कृत्तिका",                          "pronounce": "Krittika",      "english": "Pleiades"},
//! 	       { "symbol": "LS04", "native": "रोहिणी",                         "pronounce": "Rohini",         "english": "Red One"},
//! 	       { "symbol": "LS05", "native": "मृगशिर",                         "pronounce": "Mrigashira",     "english": "Deer's Head"},
//! 	       { "symbol": "LS06", "native": "आर्द्रा",                            "pronounce": "Ardra",         "english": "Storm God"},
//! 	       { "symbol": "LS07", "native": "पुनर्वसु ",                          "pronounce": "Punarvasu",     "english": "Two Restorers of Goods"},
//! 	       { "symbol": "LS08", "native": "पुष्य",                             "pronounce": "Pushya",         "english": "Nourisher"},
//! 	       { "symbol": "LS09", "native": "आश्ळेषा/आश्लेषा",	"pronounce": "Āshleshā",       "english": "Embrace"},
//! 	       { "symbol": "LS10", "native": "मघा",		 "pronounce": "Maghā",          "english": "Bountiful"},
//! 	       { "symbol": "LS11", "native": "पूर्व फाल्गुनी",		 "pronounce": "Pūrva Phalgunī", "english": "First Reddish One"},
//! 	       { "symbol": "LS12", "native": "उत्तर फाल्गुनी",		"pronounce": "Uttara Phalgunī", "english": "Second Reddish One"},
//! 	       { "symbol": "LS13", "native": "हस्त",		 "pronounce": "Hasta",           "english": "Hand"},
//! 	       { "symbol": "LS14", "native": "चित्रा",                             "pronounce": "Chitra",          "english": "Bright One"},
//! 	       { "symbol": "LS15", "native": "स्वाति",                            "pronounce": "Svati",           "english": "Very good"},
//! 	       { "symbol": "LS16", "native": "विशाखा",                         "pronounce": "Vishakha",         "english": "Forked, having branches"},
//! 	       { "symbol": "LS17", "native": "अनुराधा",                         "pronounce": "Anuradha",          "english": "Following rādhā"},
//! 	       { "symbol": "LS18", "native": "ज्येष्ठा",                            "pronounce": "Jyeshtha",           "english": "Eldest, most excellent"},
//! 	       { "symbol": "LS19", "native": "मूल",                              "pronounce": "Mula",                "english": "Root"},
//! 	       { "symbol": "LS20", "native": "पूर्व आषाढा",                     "pronounce": "Purva Ashadha",        "english": "First Invincible"},
//! 	       { "symbol": "LS21", "native": "उत्तर आषाढा",                   "pronounce": "Uttara Ashadha",       "english": "Later Invincible"},
//! 	       { "symbol": "LS22", "native": "श्रवण",                            "pronounce": "Shravana",             "english": ""},
//! 	       { "symbol": "LS23", "native": "श्रविष्ठा/धनिष्ठा",                 "pronounce": "Dhanishta/Shravishthā", "english": "Most Famous/Swiftest"},
//! 	       { "symbol": "LS24", "native": "शतभिष/शततारका",            "pronounce": "Shatabhisha Nakshatra", "english": "Requiring a hundred Physicians"},
//! 	       { "symbol": "LS25", "native": "पूर्व भाद्रपदा/पूर्व प्रोष्ठपदा",     "pronounce": "Purva Bhadrapada",      "english": "First of the Blessed Feet"},
//! 	       { "symbol": "LS26", "native": "उत्तर भाद्रपदा/उत्तर प्रोष्ठपदा", "pronounce": "Uttara Bhādrapadā",     "english": "Second of the Blessed Feet"},
//! 	       { "symbol": "LS27", "native": "रेवती",                            "pronounce": "Revati",                "english": "Prosperous"},
//! 	       { "symbol": "LS27", "native": "अभिजित",                       "pronounce": "Abhijit",               "english": "Invincible"}]},
//!
//! //! "lunar_system": {
//! "name": "Chinese",
//! "partitions_deg": [0, ... <27 longitude numbers>],
//! "coordsys": "equatorial"
//! "extent": 90,
//! "link": { "star": 65474, "offset": 180},
//! "names": [ { "symbol": "LS01", "native": "", "pronounce": "", "english": "1"},
//! 	       { "symbol": "LS02", "native": "", "pronounce": "", "english": "2"},
//! 	       { "symbol": "LS03", "native": "", "pronounce": "", "english": "3"},
//! 	       { "symbol": "LS04", "native": "", "pronounce": "", "english": "4"},
//! 	       { "symbol": "LS05", "native": "", "pronounce": "", "english": "5"},
//! 	       { "symbol": "LS06", "native": "", "pronounce": "", "english": "6"},
//! 	       { "symbol": "LS07", "native": "", "pronounce": "", "english": "7"},
//! 	       { "symbol": "LS08", "native": "", "pronounce": "", "english": "8"},
//! 	       { "symbol": "LS09", "native": "", "pronounce": "", "english": "9"},
//! 	       { "symbol": "LS10", "native": "", "pronounce": "", "english": "10"},
//! 	       { "symbol": "LS11", "native": "", "pronounce": "", "english": "11"},
//! 	       { "symbol": "LS12", "native": "", "pronounce": "", "english": "12"},
//! 	       { "symbol": "LS13", "native": "", "pronounce": "", "english": "13"},
//! 	       { "symbol": "LS14", "native": "", "pronounce": "", "english": "14"},
//! 	       { "symbol": "LS15", "native": "", "pronounce": "", "english": "15"},
//! 	       { "symbol": "LS16", "native": "", "pronounce": "", "english": "16"},
//! 	       { "symbol": "LS17", "native": "", "pronounce": "", "english": "17"},
//! 	       { "symbol": "LS18", "native": "", "pronounce": "", "english": "18"},
//! 	       { "symbol": "LS19", "native": "", "pronounce": "", "english": "19"},
//! 	       { "symbol": "LS20", "native": "", "pronounce": "", "english": "20"},
//! 	       { "symbol": "LS21", "native": "", "pronounce": "", "english": "21"},
//! 	       { "symbol": "LS22", "native": "", "pronounce": "", "english": "22"},
//! 	       { "symbol": "LS23", "native": "", "pronounce": "", "english": "23"},
//! 	       { "symbol": "LS24", "native": "", "pronounce": "", "english": "24"},
//! 	       { "symbol": "LS25", "native": "", "pronounce": "", "english": "25"},
//! 	       { "symbol": "LS26", "native": "", "pronounce": "", "english": "26"},
//! 	       { "symbol": "LS27", "native": "", "pronounce": "", "english": "27"},
//! 	       { "symbol": "LS27", "native": "", "pronounce": "", "english": "28"}]},
//! It is usually parallel to the ecliptical or equatorial coordinates of date, and splits the ecliptic (or equator) in @param partitions[0] partitions of equal number of @param partitions[1] subpartitions.
//!
class StelSkyCultureSkyPartition
{
	Q_GADGET
public:
	StelSkyCultureSkyPartition(const QJsonObject &description);
	void draw(StelCore *core) const;
	void setFontSize(int newFontSize);
	void update(double deltaTime) {fader.update(static_cast<int>(deltaTime*1000));}
	void updateLabels();

private:
	void drawCap(StelPainter &sPainter, const SphericalCap& viewPortSphericalCap, SphericalCap *cap) const;
	//void drawLabels(StelPainter &sPainter) const;

	StelCore::FrameType frameType;         //!< Useful seem only: FrameObservercentricEclipticOfDate (e.g. Zodiac), FrameEquinoxEqu (e.g. Chin. Lunar Mansions)
	QVector<double> partitions;            //!< A partition into [0] large parts of [1] smaller parts of [2] smaller parts... Currently only 2-part zodiacs [12, 30] or nakshatras [27, 4] are in use.
	QList<StelObject::CulturalName> names; //!< Full culture-sensitive naming support: names of the first-grade partition.
	QStringList symbols;                   //!< A list of very short or optimally 1-char Unicode strings with representative symbols usable as abbreviations.
	double extent;                         //!< (degrees) the displayed partition zone runs so far north and south of the center line.
	SkyLine *centerLine;                   //!< See GridlineMgr: a CUSTOM_ECLIPTIC or CUSTOM_EQUATORIAL SkyLine
	SphericalCap *northernCap;             //!< Limiting cap around the respective pole.
	SphericalCap *southernCap;             //!< Limiting cap around the respective pole.
	Vec3f color;                           //!< color to draw all lines and labels
	LinearFader fader;                     //!< for smooth fade in/out
	QFont font;                            //!< font for labels
	float lineThickness;                   //!< width of lines
};
//! @typedef StelSkyCultureSkyPartitionP
//! Shared pointer on a StelSkyCultureSkyPartition with smart pointers
typedef QSharedPointer<StelSkyCultureSkyPartition> StelSkyCultureSkyPartitionP;

#endif
