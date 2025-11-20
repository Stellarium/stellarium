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

#include "GridLinesMgr.hpp"
#include "StelCore.hpp"
#include "StelObject.hpp"

class ConstellationMgr;
class StelTranslator;
class QJsonObject;

//! @class StelSkyCultureSkyPartition
//! An optional skyculture element, configurable in the index.json.
//! Currently Stellarium supports two optional such systems per skyculture: Zodiac and LunarSystem.
//! "zodiac": {
//! "name": { "native": "Signa Zodiacalia", "english": "Zodiacal signs"},
//! "context": "western zodiac sign", // optional, improvements for localization support
//! "partitions": [ 12, 30, 60],
//! "extent": 8,
//! "names": [ { "symbol": "\u2648", "native": "Aries",       "english": "Ram"},
//! 	       { "symbol": "\u2649", "native": "Taurus",      "english": "Bull"},
//! 	       { "symbol": "\u264A", "native": "Gemini",      "english": "Twins"},
//! 	       { "symbol": "\u264B", "native": "Cancer",      "english": "Crab"},
//! 	       { "symbol": "\u264C", "native": "Leo",         "english": "Lion"},
//! 	       { "symbol": "\u264D", "native": "Virgo",       "english": "Virgin"},
//! 	       { "symbol": "\u264E", "native": "Libra",       "english": "Scales"},
//! 	       { "symbol": "\u264F", "native": "Scorpius",     "english": "Scorpion"},
//! 	       { "symbol": "\u2650", "native": "Capricornus", "english": "Capricorn"},
//! 	       { "symbol": "\u2651", "native": "Sagittarius", "english": "Archer"},
//! 	       { "symbol": "\u2652", "native": "Aquarius",    "english": "Water bearer"},
//! 	       { "symbol": "\u2653", "native": "Pisces",      "english": "Fishes"}]},
//! The INDIAN Nakshatras are a system of 27 equal-sized stations, 13°20' in longitude each, divided in 4 equal parts.
//! The sequence is rotated along the ecliptic so that it is starting either with beta Ari at 8° in the first Nakshatra,
//! or with Spica at 180°. The literature lists both, probably these are regional differences. This example is for Spica:
//! "lunar_system": {
//! "name": "Nakshatras",
//! "context": "indian lunar mansion", // optional, improvements for localization support
//! "partitions": [ 27, 4],
//! "extent": 5,
//! "link": { "star": 65474, "offset": 180},
//! "names": [ { "symbol":  "1", "native": "अश्विनी",                          "pronounce": "Ashvini",       "english": "Physician to the Gods"},
//! 	       { "symbol":  "2", "native": "भरणी",                           "pronounce": "Bharani",        "english": "Bearer"},
//! 	       { "symbol":  "3", "native": "कृत्तिका",                          "pronounce": "Krittika",      "english": "Pleiades"},
//! 	       { "symbol":  "4", "native": "रोहिणी",                         "pronounce": "Rohini",         "english": "Red One"},
//! 	       { "symbol":  "5", "native": "मृगशिर",                         "pronounce": "Mrigashira",     "english": "Deer's Head"},
//! 	       { "symbol":  "6", "native": "आर्द्रा",                            "pronounce": "Ardra",         "english": "Storm God"},
//! 	       { "symbol":  "7", "native": "पुनर्वसु ",                          "pronounce": "Punarvasu",     "english": "Two Restorers of Goods"},
//! 	       { "symbol":  "8", "native": "पुष्य",                             "pronounce": "Pushya",         "english": "Nourisher"},
//! 	       { "symbol":  "9", "native": "आश्ळेषा/आश्लेषा",	            "pronounce": "Āshleshā",       "english": "Embrace"},
//! 	       { "symbol": "10", "native": "मघा",		             "pronounce": "Maghā",          "english": "Bountiful"},
//! 	       { "symbol": "11", "native": "पूर्व फाल्गुनी",                     "pronounce": "Pūrva Phalgunī", "english": "First Reddish One"},
//! 	       { "symbol": "12", "native": "उत्तर फाल्गुनी",                   "pronounce": "Uttara Phalgunī", "english": "Second Reddish One"},
//! 	       { "symbol": "13", "native": "हस्त",		              "pronounce": "Hasta",           "english": "Hand"},
//! 	       { "symbol": "14", "native": "चित्रा",                             "pronounce": "Chitra",          "english": "Bright One"},
//! 	       { "symbol": "15", "native": "स्वाति",                            "pronounce": "Svati",           "english": "Very good"},
//! 	       { "symbol": "16", "native": "विशाखा",                         "pronounce": "Vishakha",         "english": "Forked, having branches"},
//! 	       { "symbol": "17", "native": "अनुराधा",                         "pronounce": "Anuradha",          "english": "Following rādhā"},
//! 	       { "symbol": "18", "native": "ज्येष्ठा",                            "pronounce": "Jyeshtha",           "english": "Eldest, most excellent"},
//! 	       { "symbol": "19", "native": "मूल",                              "pronounce": "Mula",                "english": "Root"},
//! 	       { "symbol": "20", "native": "पूर्व आषाढा",                     "pronounce": "Purva Ashadha",        "english": "First Invincible"},
//! 	       { "symbol": "21", "native": "उत्तर आषाढा",                   "pronounce": "Uttara Ashadha",       "english": "Later Invincible"},
//! 	       { "symbol": "22", "native": "श्रवण",                            "pronounce": "Shravana",             "english": ""},
//! 	       { "symbol": "23", "native": "श्रविष्ठा/धनिष्ठा",                 "pronounce": "Dhanishta/Shravishthā", "english": "Most Famous/Swiftest"},
//! 	       { "symbol": "24", "native": "शतभिष/शततारका",            "pronounce": "Shatabhisha Nakshatra", "english": "Requiring a hundred Physicians"},
//! 	       { "symbol": "25", "native": "पूर्व भाद्रपदा/पूर्व प्रोष्ठपदा",     "pronounce": "Purva Bhadrapada",      "english": "First of the Blessed Feet"},
//! 	       { "symbol": "26", "native": "उत्तर भाद्रपदा/उत्तर प्रोष्ठपदा", "pronounce": "Uttara Bhādrapadā",     "english": "Second of the Blessed Feet"},
//! 	       { "symbol": "27", "native": "रेवती",                            "pronounce": "Revati",                "english": "Prosperous"},
//! 	       { "symbol": "28", "native": "अभिजित",                       "pronounce": "Abhijit",               "english": "Invincible"}]}, // The #28 is weird, sometimes listed but apparently not used.
//!
//! The Chinese define 28 Lunar Mansions of different sizes in equatorial coordinates, and use defining stars for the onset of a mansion.
//! The sky is dissected into 28 slices which extend from pole to pole.
//! What is still unclear is whether we should keep the link to the stars "live", or let the stars move out of epoch.
//! Currently the lines are kept "live", following the idea that, with each new official star map "from now on the Mansions should be defined like this".
//! //! "lunar_system": {
//! "name": "Chinese",
//! "context": "chinese lunar mansion", // optional, improvements for localization support
//! "defining_stars": [65474, 69427, 72622, 78265, 80763. 82514, 88635, 92041, 100345, 102618, 106278, 109074, 113963, 1067,
//!                    4463, 8903, 12719, 17499, 20889, 26207, 26727, 30343, 41822, 42313, 46390, 48356, 53740, 59803],
//!                   // 28 star HIP indices which define the start of a Lunar Mansion
//! "epoch": double, // optional. If existent, positions of defining stars are computed for epoch, and arcs drawn through their positions, which don't move by precession.
//! "coordsys": "equatorial",
//! "extent": 90,
//! "names": [{ "symbol":  "1", "native": "角", "pronounce": "Jiǎo",  "english": "Horn"},
//!           { "symbol":  "2", "native": "亢", "pronounce": "Kàng",  "english": "Neck"},
//!           { "symbol":  "3", "native": "氐", "pronounce": "Dī",    "english": "Root"},
//!           { "symbol":  "4", "native": "房", "pronounce": "Fáng",  "english": "Room"},
//!           { "symbol":  "5", "native": "心", "pronounce": "Xīn",   "english": "Heart"},
//!           { "symbol":  "6", "native": "尾", "pronounce": "Wěi",   "english": "Tail"},
//!           { "symbol":  "7", "native": "箕", "pronounce": "Jī",    "english": "Winnowing Basket"},
//!           { "symbol":  "8", "native": "斗", "pronounce": "Dǒu",   "english": "Dipper"},
//!           { "symbol":  "9", "native": "牛", "pronounce": "Niú",   "english": "Ox"},
//!           { "symbol": "10", "native": "女", "pronounce": "Nǚ",    "english": "Girl"},
//!           { "symbol": "11", "native": "虛", "pronounce": "Xū",    "english": "Emptiness"},
//!           { "symbol": "12", "native": "危", "pronounce": "Wēi",   "english": "Rooftop"},
//!           { "symbol": "13", "native": "室", "pronounce": "Shì",   "english": "Encampment"},
//!           { "symbol": "14", "native": "壁", "pronounce": "Bì",    "english": "Wall"},
//!           { "symbol": "15", "native": "奎", "pronounce": "Kuí",   "english": "Legs"},
//!           { "symbol": "16", "native": "婁", "pronounce": "Lóu",   "english": "Bond"},
//!           { "symbol": "17", "native": "胃", "pronounce": "Wèi",   "english": "Stomach"},
//!           { "symbol": "18", "native": "昴", "pronounce": "Mǎo",   "english": "Hairy Head"},
//!           { "symbol": "19", "native": "畢", "pronounce": "Bì",    "english": "Net"},
//!           { "symbol": "20", "native": "觜", "pronounce": "Zī",    "english": "Turtle Beak"},
//!           { "symbol": "21", "native": "参", "pronounce": "Shēn",  "english": "Three Stars"},
//!           { "symbol": "22", "native": "井", "pronounce": "Jǐng",  "english": "Well"},
//!           { "symbol": "23", "native": "鬼", "pronounce": "Guǐ",   "english": "Ghost"},
//!           { "symbol": "24", "native": "柳", "pronounce": "Liǔ",   "english": "Willow"},
//!           { "symbol": "25", "native": "星", "pronounce": "Xīng",  "english": "Star"},
//!           { "symbol": "26", "native": "張", "pronounce": "Zhāng", "english": "Extended Net"},
//!           { "symbol": "27", "native": "翼", "pronounce": "Yì",    "english": "Wings"},
//!           { "symbol": "28", "native": "軫", "pronounce": "Zhěn",  "english": "Chariot"}]},
//! It is usually parallel to the ecliptical or equatorial coordinates of date, and splits the ecliptic (or equator)
//! in @param partitions[0] partitions of equal number of @param partitions[1] subpartitions.
//!
class StelSkyCultureSkyPartition
{
	Q_GADGET
public:
        StelSkyCultureSkyPartition(const QJsonObject &description);
	~StelSkyCultureSkyPartition();
	void draw(StelPainter& sPainter, const Vec3d &obsVelocity);
	void setFontSize(int newFontSize);
	//! Update ecliptic obliquity and center line. deltaTime since last frame in seconds.
	void update(double deltaTime);
	void updateLabels();
	//! Update i18n names from English names according to current locale.
	void updateI18n();
	//! Return name of this system in screenLabel style.
	QString getCulturalName() const;
	//! Return longitudinal coordinate of point eqPos as written in the respective system
	//! For zodiac, this is SYMBOL:deg°min'
	//! For Lunar systems, it is usually only STATION or, with partitions, STATION:part
	QString getLongitudeCoordinate(Vec3d &eqPos) const;

private:
	void drawCap(StelPainter &sPainter, const SphericalCap& viewPortSphericalCap, double latDeg) const;
	void drawMansionCap(StelPainter &sPainter, const SphericalCap& viewPortSphericalCap, double latDeg, double lon1, double lon2) const;
	//void drawLabels(StelPainter &sPainter) const;

	StelCore::FrameType frameType;         //!< Useful seem only: FrameObservercentricEclipticOfDate (e.g. Zodiac), FrameEquinoxEqu (e.g. Chin. Lunar Mansions)
	QList<int> partitions;                 //!< A partition into [0] large parts of [1] smaller parts of [2] smaller parts... Currently only 2-part zodiacs [12, 30] or nakshatras [27, 4] are in use.
	StelObject::CulturalName name;         //!< Full culture-sensitive naming support: name of the system.
	QList<StelObject::CulturalName> names; //!< Full culture-sensitive naming support: names of the first-grade partition.
	QStringList symbols;                   //!< A list of very short or optimally 1-char Unicode strings with representative symbols usable as abbreviations.	
	QList<double> extent;                  //!< (degrees) If only one element (usual case), the displayed partition zone runs so far north and south of the center line.
					       //!<           Else, the list MUST contain 2x number of partitions[0], and contains north and south latitudes of each lunar station.
public:
	SkyLine *centerLine;                   //!< See GridlineMgr: a CUSTOM_ECLIPTIC or CUSTOM_EQUATORIAL SkyLine
private:
	int fontSize;                          //!< fontsize for labels
	QList<int> linkStars;                  //!< HIP numbers defining start of mansions (Chinese style), or with just one entry, of star that defines a given offset longitude.
	double offset;                         //!< the longitude (degrees) in the respective culturalpartition which is defined by linkStar
	double eclObl;                         //!< Ecliptical obliquity (computed in update(), also consumed in getLongitudeCoordinate())
	double offsetFromAries;                //!< (degrees) resulting deviation between ecliptical longitude (or RA) of date and "cultural longitude" (or RA) of date.
	QString context;				//!< A context data for localization support
};
//! @typedef StelSkyCultureSkyPartitionP
//! Shared pointer on a StelSkyCultureSkyPartition with smart pointers
typedef QSharedPointer<StelSkyCultureSkyPartition> StelSkyCultureSkyPartitionP;

#endif
