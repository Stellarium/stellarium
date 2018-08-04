/*
 * Stellarium
 * Copyright (C) 2017 Alexander Wolf
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

#ifndef ASTERISM_HPP
#define ASTERISM_HPP

#include "StelObject.hpp"
#include "StelUtils.hpp"
#include "StelFader.hpp"
#include "StelSphereGeometry.hpp"
#include "AsterismMgr.hpp"

#include <vector>
#include <QString>
#include <QFont>

class StarMgr;
class StelPainter;

//! @class Asterism
//! The Asterism class models a grouping of stars in a Sky Culture.
//! Each Asterism consists of a list of stars identified by their
//! abbreviation and Hipparcos catalogue numbers (taken from file: asterismship.fab),
//! another entry in file asterism_names.eng.fab with the defining abbreviated name
//! and translatable englishName (translation goes into nameI18).
class Asterism : public StelObject
{
	friend class AsterismMgr;
private:
	static const QString ASTERISM_TYPE;
	Asterism();
	~Asterism();

	// StelObject method to override
	//! Get a string with data about the Asterism.
	//! Constellations support the following InfoStringGroup flags:
	//! - Name
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString a description of the constellation.
	virtual QString getInfoString(const StelCore*, const InfoStringGroup& flags) const;

	//! Get the module/object type string.
	//! @return "Asterism"
	virtual QString getType() const {return ASTERISM_TYPE;}
	virtual QString getID() const { return abbreviation; }

	//! observer centered J2000 coordinates.
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const {return XYZname;}

	virtual double getAngularSize(const StelCore*) const {Q_ASSERT(0); return 0;} // TODO

	//! @param record string containing the following whitespace
	//! separated fields: abbreviation - a three character abbreviation
	//! for the asterism, a number of lines (pairs), and a list of Hipparcos
	//! catalogue numbers which, when connected pairwise, form the lines of the
	//! asterism.
	//! @param starMgr a pointer to the StarManager object.
	//! @return false if can't parse record, else true.
	bool read(const QString& record, StarMgr *starMgr);

	//! Draw the asterism name
	void drawName(StelPainter& sPainter) const;

	//! Test if a star is part of a Asterism.
	//! This member tests to see if a star is one of those which make up
	//! the lines of a Asterism.
	//! @return a pointer to the asterism which the star is a part of,
	//! or Q_NULLPTR if the star is not part of a asterism
	const Asterism* isStarIn(const StelObject*) const;

	//! Get the translated name for the Asterism.
	QString getNameI18n() const {return nameI18;}
	//! Get the English name for the Asterism.
	QString getEnglishName() const {return englishName;}	
	//! Draw the lines for the Asterism.
	//! This method uses the coords of the stars (optimized for use through
	//! the class AsterismMgr only).
	void drawOptim(StelPainter& sPainter, const StelCore* core, const SphericalCap& viewportHalfspace) const;
	//! Update fade levels according to time since various events.
	void update(int deltaTime);
	//! Turn on and off Asterism line rendering.
	//! @param b new state for line drawing.
	void setFlagLines(const bool b) { lineFader=b; }
	//! Turn on and off ray helper rendering.
	//! @param b new state for ray helper drawing.
	void setFlagRayHelpers(const bool b) { rayHelperFader=b; }
	//! Turn on and off Asterism name label rendering.
	//! @param b new state for name label drawing.
	void setFlagLabels(const bool b) { nameFader=b; }
	//! Get the current state of Asterism line rendering.
	//! @return true if Asterism line rendering it turned on, else false.
	bool getFlagLines() const { return lineFader; }
	//! Get the current state of ray helper rendering.
	//! @return true if ray helper rendering it turned on, else false.
	bool getFlagRayHelpers() const { return rayHelperFader; }
	//! Get the current state of Asterism name label rendering.
	//! @return true if Asterism name label rendering it turned on, else false.
	bool getFlagLabels() const { return nameFader; }

	bool isAsterism() const { return flagAsterism; }

	//! International name (translated using gettext)
	QString nameI18;
	//! Name in english (second column in asterism_names.eng.fab)
	QString englishName;
	//! Abbreviation
	//! A skyculture designer must invent it. (usually 2-5 letters)
	//! This MUST be filled and be unique within a sky culture.
	QString abbreviation;	
	//! Context for name
	QString context;
	//! Direction vector pointing on constellation name drawing position
	Vec3d XYZname;
	Vec3d XYname;
	//! Number of segments in the lines
	unsigned int numberOfSegments;
	//! Type of asterism
	int typeOfAsterism;
	bool flagAsterism;
	//! List of stars forming the segments
	StelObjectP* asterism;

	SphericalCap boundingCap;

	//! Define whether lines and names must be drawn
	LinearFader lineFader, rayHelperFader, nameFader;

	//! Currently we only need one color for all asterisms, this may change at some point
	static Vec3f lineColor;
	static Vec3f rayHelperColor;
	static Vec3f labelColor;
};

#endif // ASTERISM_HPP
