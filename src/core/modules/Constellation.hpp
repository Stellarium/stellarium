/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
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

#ifndef _CONSTELLATION_HPP_
#define _CONSTELLATION_HPP_

#include <vector>
#include <QString>
#include <QFont>

#include "StelObject.hpp"
#include "StelUtils.hpp"
#include "StelFader.hpp"
#include "StelProjector.hpp"
#include "StelSphereGeometry.hpp"

class StarMgr;

//! @class Constellation
//! The Constellation class models a grouping of stars in a Sky Culture.
//! Each Constellation consists of a list of stars identified by their
//! Hipparcos catalogue numbers, a name and optionally an abbreviated name,
//! boundary shape and an artistic pictorial representation.
class Constellation : public StelObject
{
	friend class ConstellationMgr;
private:
	Constellation();
	~Constellation();

	// StelObject method to override
	//! Get a string with data about the Constellation.
	//! Constellations support the following InfoStringGroup flags:
	//! - Name
	//! @param core the Stelore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString a description of the Planet.
	virtual QString getInfoString(const StelCore*, const InfoStringGroup& flags) const
	{
		if (flags&Name) return getNameI18n() + "(" + getShortName() + ")";
		else return "";
	}

	//! Get the module/object type string.
	//! @return "Constellation"
	virtual QString getType() const {return "Constellation";}

	//! observer centered J2000 coordinates.
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const {return XYZname;}

	virtual double getAngularSize(const StelCore*) const {Q_ASSERT(0); return 0;} // TODO

	//! @param record string containing the following whitespace
	//! separated fields: abbreviation - a three character abbreviation
	//! for the constellation, a number of lines, and a list of Hipparcos
	//! catalogue numbers which, when connected form the lines of the
	//! constellation.
	//! @param starMgr a pointer to the StarManager object.
	//! @return false if can't parse record, else true.
	bool read(const QString& record, StarMgr *starMgr);

	//! Draw the constellation name.
	//!
	//! @param renderer  Renderer to draw with.
	//! @param projector Font to draw the name with.
	void drawName(class StelRenderer* renderer, QFont& font) const;

	//! Draw the constellation boundary.
	//!
	//! @param renderer  Renderer to draw with.
	//! @param projector Projector to project vertices to viewport.
	void drawBoundaryOptim(StelRenderer* renderer, StelProjectorP projector) const;

	//! Test if a star is part of a Constellation.
	//! This member tests to see if a star is one of those which make up
	//! the lines of a Constellation.
	//! @return a pointer to the constellation which the star is a part of,
	//! or NULL if the star is not part of a constellation
	const Constellation* isStarIn(const StelObject*) const;

	//! Get the brightest star in a Constellation.
	//! Checks all stars which make up the constellation lines, and returns
	//! a pointer to the one with the brightest apparent magnitude.
	//! @return a pointer to the brightest star
	StelObjectP getBrightestStarInConstellation(void) const;

	//! Get the translated name for the Constellation.
	QString getNameI18n() const {return nameI18;}
	//! Get the English name for the Constellation (returns the abbreviation).
	QString getEnglishName() const {return abbreviation;}
	//! Get the short name for the Constellation (returns the abbreviation).
	QString getShortName() const {return abbreviation;}

	//! Generate the vertex buffer to draw constellation art.
	//!
	//! @param renderer   Renderer to create the vertex buffer.
	//! @param resolution Resolution of the vertex grid to draw art with.
	void generateArtVertices(class StelRenderer* renderer, const int resolution);

	//! Draw the lines for the Constellation.
	//! This method uses the coords of the stars (optimized for use thru
	//! the class ConstellationMgr only).
	void drawOptim(class StelRenderer* renderer, StelProjectorP projector, 
	               const StelCore* core, const SphericalCap& viewportHalfspace) const;

	//! Draw the art texture, optimized function to be called thru a constellation manager only.
	//!
	//! @param renderer  Renderer to draw with.
	//! @param projector Projector to project 3D coordinates to the viewport.
	//! @param region    Spherical region containing viewable area, used for culling.
	void drawArtOptim(class StelRenderer* renderer, StelProjectorP projector, 
	                  const SphericalRegion& region) const;

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

	//! International name (translated using gettext)
	QString nameI18;
	//! Name in english
	QString englishName;
	//! Name in native language
	QString nativeName;
	//! Abbreviation (of the latin name for western constellations)
	QString abbreviation;
	//! Direction vector pointing on constellation name drawing position
	Vec3d XYZname;
	Vec3d XYname;
	//! Number of segments in the lines
	unsigned int numberOfSegments;
	//! List of stars forming the segments
	StelObjectP* asterism;

	class StelTextureNew* artTexture;

	QString artTexturePath;

	//! Vertex with a 3D position and a texture coordinate.
	struct Vertex
	{
		Vec3f position;
		Vec2f texCoord;
		Vertex(const Vec3f& position, const Vec2f texCoord) 
			: position(position), texCoord(texCoord) {}
		VERTEX_ATTRIBUTES(Vec3f Position, Vec2f TexCoord);
	};

	//! Vertex grid to draw constellation art on.
	StelVertexBuffer<Vertex>* artVertices;
	
	//! Index buffer representing triangles of the cells of the grid.
	class StelIndexBuffer* artIndices;

	SphericalCap boundingCap;

	//! Define whether art, lines, names and boundary must be drawn
	LinearFader artFader, lineFader, nameFader, boundaryFader;
	std::vector<std::vector<Vec3f> *> isolatedBoundarySegments;
	std::vector<std::vector<Vec3f> *> sharedBoundarySegments;

	//! Matrix to transform art grid vertices from unnormalized texture
	//! positions (pixel indices) to 3D coordinates to draw them at.
	Mat4f texCoordTo3D;

	//! Currently we only need one color for all constellations, this may change at some point
	static Vec3f lineColor;
	static Vec3f labelColor;
	static Vec3f boundaryColor;

	static bool singleSelected;
};

#endif // _CONSTELLATION_HPP_
