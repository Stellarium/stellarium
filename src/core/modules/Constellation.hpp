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
#include "StelUtils.hpp"
#include "StelFader.hpp"
#include "StelTextureTypes.hpp"
#include "StelSphereGeometry.hpp"
#include "ConstellationMgr.hpp"

#include <vector>
#include <QString>
#include <QFont>

class StarMgr;
class StelPainter;

//! @class Constellation
//! The Constellation class models a grouping of stars in a Sky Culture.
//! Each Constellation consists of a list of stars identified by their
//! abbreviation and Hipparcos catalogue numbers (taken from file: constellationship.fab),
//! another entry in file constellation_names.eng.fab with the defining abbreviated name,
//! nativeName, and translatable englishName (translation goes into nameI18),
//! boundary shape from file constellation_boundaries.dat and an (optional) artistic pictorial representation.
//! GZ NEW: The nativeName should be accessible in a GUI option, so that e.g. original names as written in a
//! concrete book where a skyculture has been taken from can be assured even when translation is available.
//! TODO: There should be a distinction between constellations and asterisms, which are "inofficial" figures within a sky culture.
//! For example, Western sky culture has a "Big Dipper", "Coathanger", etc. These would be nice to see, but in different style.
class Constellation : public StelObject
{
	friend class ConstellationMgr;
private:
	static const QString CONSTELLATION_TYPE;
	Constellation();
	~Constellation();

	// StelObject method to override
	//! Get a string with data about the Constellation.
	//! Constellations support the following InfoStringGroup flags:
	//! - Name
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString a description of the constellation.
	virtual QString getInfoString(const StelCore*, const InfoStringGroup& flags) const;

	//! Get the module/object type string.
	//! @return "Constellation"
	virtual QString getType() const {return CONSTELLATION_TYPE;}
	virtual QString getID() const { return abbreviation; }

	//! observer centered J2000 coordinates.
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const {return XYZname;}

	virtual double getAngularSize(const StelCore*) const {Q_ASSERT(0); return 0.;} // TODO

	//! @param record string containing the following whitespace
	//! separated fields: abbreviation - a three character abbreviation
	//! for the constellation, a number of lines (pairs), and a list of Hipparcos
	//! catalogue numbers which, when connected pairwise, form the lines of the
	//! constellation.
	//! @param starMgr a pointer to the StarManager object.
	//! @return false if can't parse record (invalid result!), else true.
	bool read(const QString& record, StarMgr *starMgr);

	//! Draw the constellation name
	void drawName(StelPainter& sPainter, ConstellationMgr::ConstellationDisplayStyle style) const;
	//! Draw the constellation art
	void drawArt(StelPainter& sPainter) const;
	//! Draw the constellation boundary
	void drawBoundaryOptim(StelPainter& sPainter) const;

	//! Test if a star is part of a Constellation.
	//! This member tests to see if a star is one of those which make up
	//! the lines of a Constellation.
	//! @return a pointer to the constellation which the star is a part of,
	//! or Q_NULLPTR if the star is not part of a constellation
	const Constellation* isStarIn(const StelObject*) const;

	//! Get the brightest star in a Constellation.
	//! Checks all stars which make up the constellation lines, and returns
	//! a pointer to the one with the brightest apparent magnitude.
	//! @return a pointer to the brightest star
	StelObjectP getBrightestStarInConstellation(void) const;

	//! Get the translated name for the Constellation.
	QString getNameI18n() const {return nameI18;}
	//! Get the English name for the Constellation.
	QString getEnglishName() const {return englishName;}
	//! Get the short name for the Constellation (returns the abbreviation).
	QString getShortName() const {return abbreviation;}
	//! Draw the lines for the Constellation.
	//! This method uses the coords of the stars (optimized for use through
	//! the class ConstellationMgr only).
	void drawOptim(StelPainter& sPainter, const StelCore* core, const SphericalCap& viewportHalfspace) const;
	//! Draw the art texture, optimized function to be called through a constellation manager only.
	void drawArtOptim(StelPainter& sPainter, const SphericalRegion& region) const;
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

	//! Check visibility of starlore elements (using for seasonal rules)
	//! @return true if starlore elements rendering it turned on, else false.
	bool checkVisibility() const;

	//! International name (translated using gettext)
	QString nameI18;
	//! Name in english (column 3 in constellation_names.eng.fab)
	QString englishName;
	//! Name in native language (column 2 in constellation_names.eng.fab).
	//! According to practice as of V0.13.1, this may be an empty string.
	//! If empty, will be filled with englishName.
	QString nativeName;
	//! Abbreviation (of the latin name for western constellations)
	//! For non-western, a skyculture designer must invent it. (usually 2-5 letters)
	//! This MUST be filled and be unique within a sky culture.
	QString abbreviation;
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
	StelObjectP* constellation;

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
