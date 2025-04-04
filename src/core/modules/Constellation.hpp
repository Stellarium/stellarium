/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
 * Copyright (C) 2014 Georg Zotti
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

#ifndef CONSTELLATION_HPP
#define CONSTELLATION_HPP

#include "StelObject.hpp"
#include "StelTranslator.hpp"
#include "StelFader.hpp"
#include "StelTextureTypes.hpp"
#include "StelSphereGeometry.hpp"
#include "ConstellationMgr.hpp"

#include <vector>
#include <QJsonObject>
#include <QString>
#include <QFont>

class StarMgr;
class StelPainter;

//! @class Constellation
//! The Constellation class models a grouping of stars in a Sky Culture.
//! Each Constellation consists of a list of stars identified by their
//! abbreviation and Hipparcos catalogue numbers (taken from file: constellationship.fab),
//! another entry in file constellation_names.eng.fab with the defining abbreviated name,
//! nativeName, and translatable englishName (translation goes into nameI18).
//! GZ NEW: The nativeName should be accessible in a GUI option, so that e.g. original names as written in a
//! concrete book where a skyculture has been taken from can be assured even when translation is available.
//! TODO: There should be a distinction between constellations and asterisms, which are "unofficial" figures within a sky culture.
//! For example, Western sky culture has a "Big Dipper", "Coathanger", etc. These would be nice to see, but in different style.
class Constellation : public StelObject
{
	friend class ConstellationMgr;
private:
	static const QString CONSTELLATION_TYPE;
	Constellation();
	~Constellation() override;

	// StelObject method to override
	//! Get a string with data about the Constellation.
	//! Constellations support the following InfoStringGroup flags:
	//! - Name
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString a description of the constellation.
	QString getInfoString(const StelCore*, const InfoStringGroup& flags) const override;

	//! Get the module/object type string.
	//! @return "Constellation"
	QString getType() const override {return CONSTELLATION_TYPE;}
	QString getObjectType() const override { return N_("constellation"); }
	QString getObjectTypeI18n() const override { return q_(getObjectType()); }
	QString getID() const override { return abbreviation; }

	//! observer centered J2000 coordinates.
	Vec3d getJ2000EquatorialPos(const StelCore*) const override {return XYZname;}

	//! @param data a JSON formatted constellation record from index.json
	//! @param starMgr a pointer to the StarManager object.
	//! @return false if can't parse record (invalid result!), else true.
	bool read(const QJsonObject& data, StarMgr *starMgr);

	//! Draw the constellation name. Depending on completeness of names and data, there may be a rich set of options to display.
	void drawName(StelPainter& sPainter, CulturalDisplayStyle style) const;
	//! Draw the constellation art
	void drawArt(StelPainter& sPainter) const;
	//! Draw the constellation boundary. obsVelocity used for aberration
	void drawBoundaryOptim(StelPainter& sPainter, const Vec3d &obsVelocity) const;

	//! Test if a star is part of a Constellation.
	//! This member tests to see if a star is one of those which make up
	//! the lines of a Constellation.
	//! @return a pointer to the constellation which the star is a part of,
	//! or nullptr if the star is not part of a constellation
	const Constellation* isStarIn(const StelObject*) const;

	//! Get the brightest star in a Constellation.
	//! Checks all stars which make up the constellation lines, and returns
	//! a pointer to the one with the brightest apparent magnitude.
	//! @return a pointer to the brightest star
	StelObjectP getBrightestStarInConstellation(void) const;

	//! Get the translated name for the Constellation.
	QString getNameI18n() const override {return culturalName.translatedI18n;}
	//! Get the English name for the Constellation.
	QString getEnglishName() const override {return culturalName.translated;}
	//! Get the native name for the Constellation
	QString getNameNative() const override {return culturalName.native;}
	//! Get (translated) pronouncement of the native name for the Constellation
	QString getNamePronounce() const override {return (culturalName.pronounceI18n.isEmpty() ? culturalName.native : culturalName.pronounceI18n);}
	//! Get the short name for the Constellation (returns the translated version of abbreviation).
	QString getShortName() const {return abbreviationI18n;}
	//! Combine screen label from various components, depending on settings in SkyCultureMgr
	QString getScreenLabel() const override;
	//! Combine InfoString label from various components, depending on settings in SkyCultureMgr
	QString getInfoLabel() const override;
	//! Underlying worker
	QString getCultureLabel(StelObject::CulturalDisplayStyle style) const;
	//! Draw the lines for the Constellation.
	//! This method uses the coords of the stars (optimized for use through
	//! the class ConstellationMgr only).
	void drawOptim(StelPainter& sPainter, const StelCore* core, const SphericalCap& viewportHalfspace) const;
	//! Draw the art texture, optimized function to be called through a constellation manager only.  obsVelocity used for aberration
	void drawArtOptim(StelPainter& sPainter, const SphericalRegion& region, const Vec3d& obsVelocity) const;
	//! Update fade levels according to time since various events.
	void update(int deltaTime);
	//! Turn on and off Constellation line rendering.
	//! @param b new state for line drawing.
	void setFlagLines(const bool b) {lineFader=b;}
	//! Turn on and off Constellation boundary rendering.
	//! @param b new state for boundary drawing.
	void setFlagBoundaries(const bool b) {boundaryFader=b;}
	//! Turn on and off Constellation name label rendering.
	//! @param b new state for name label drawing.
	void setFlagLabels(const bool b) {nameFader=b;}
	//! Turn on and off Constellation art rendering.
	//! @param b new state for art drawing.
	void setFlagArt(const bool b) {artFader=b;}
	//! Get the current state of Constellation line rendering.
	//! @return true if Constellation line rendering it turned on, else false.
	bool getFlagLines() const {return lineFader;}
	//! Get the current state of Constellation boundary rendering.
	//! @return true if Constellation boundary rendering it turned on, else false.
	bool getFlagBoundaries() const {return boundaryFader;}
	//! Get the current state of Constellation name label rendering.
	//! @return true if Constellation name label rendering it turned on, else false.
	bool getFlagLabels() const {return nameFader;}
	//! Get the current state of Constellation art rendering.
	//! @return true if Constellation art rendering it turned on, else false.
	bool getFlagArt() const {return artFader;}

	//! Check visibility of sky culture elements (using for seasonal rules)
	//! @return true if sky culture elements rendering it turned on, else false.
	bool checkVisibility() const;

	//! Constellation name. This is a culture-dependent thing, and in each skyculture a constellation has one name entry only.
	//! Given multiple aspects of naming, we need all the components and more.
	CulturalName culturalName;
	////! International name (translated using gettext)
	//QString nameI18;
	////! Name in English language
	//QString englishName;
	////! Name in native language (original name of constellation in the source)
	////! According to practice as of V0.13.1, this may be an empty string.
	////! If empty, will be filled with englishName.
	//QString nativeName;
	////! Latin-letter based transliteration geared at english pronounciation
	//QString nativeNamePronounce;
	////! Translated transliteration geared at pronounciation in user language
	//QString nativeNamePronounceI18n;
	////! Pronunciation aid in International Phonetic Alphabet (optional)
	//QString nativeNameIPA;
	////! A scientific transliteration that does not help pronunciation. (Example: Tibetan/Wylie.)
	//QString nativeNameTranslit;
	//! Abbreviation (the short name or designation of constellations)
	//! For non-western, a skyculture designer must invent it. (usually 2-5 Latin letters and numerics)
	//! This MUST be filled and be unique within a sky culture.
	QString abbreviation;
	//! Translated version of abbreviation (the short name or designation of constellations)
	//! Latin-based languages should not translate it, but it may be useful to translate for other glyph systems.
	QString abbreviationI18n;
	//! The context for English name of constellation (using for correct translation via gettext)
	QString context;
	//! Direction vector pointing on constellation name drawing position
	Vec3d XYZname;
	Vec3d XYname;
	//! Number of segments in the lines
	unsigned int numberOfSegments;
	//! Month [1..12] of start visibility of constellation (seasonal rules)
	int beginSeason;
	//! Month [1..12] of end visibility of constellation (seasonal rules)
	int endSeason;
	//! List of stars forming the segments
	std::vector<StelObjectP> constellation;
	//! In case this describes a single-star constellation (i.e. just one line segment that starts and ends at the same star),
	//! or we have a line segment with such single star somewhere within the constellation,
	//! we will draw a circle with this opening radius.
	double singleStarConstellationRadius;

	StelTextureSP artTexture;
	StelVertexArray artPolygon;
	SphericalCap boundingCap;

	//! Define whether art, lines, names and boundary must be drawn
	LinearFader artFader, lineFader, nameFader, boundaryFader;
	//! Constellation art opacity
	float artOpacity;
	std::vector<std::vector<Vec3d> *> isolatedBoundarySegments;
	std::vector<std::vector<Vec3d> *> sharedBoundarySegments;

	//! Currently we only need one color for all constellations, this may change at some point
	static Vec3f lineColor;
	static Vec3f labelColor;
	static Vec3f boundaryColor;

	static bool singleSelected;	
	static bool seasonalRuleEnabled;
	// set by ConstellationMgr to fade out art on small FOV values
	// see LP:#1294483
	static float artIntensityFovScale;
};

#endif // CONSTELLATION_HPP
