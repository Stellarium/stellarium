/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#ifndef HEIGHTMAP_HPP
#define HEIGHTMAP_HPP

#include "StelOBJ.hpp"

//! This represents a heightmap for viewer-ground collision
class Heightmap
{

public:
	typedef QVector<unsigned int> IdxList;
	typedef QVector<Vec3f> PosList;

        //! Construct a heightmap from a loaded OBJ mesh.
        //! The mesh is stored as reference and used for calculations.
        //! @param obj Mesh for building the heightmap.
	Heightmap();
        virtual ~Heightmap();

	void setMeshData(const IdxList& indexList, const PosList& posList);

        //! Get z Value at (x,y) coordinates.
        //! In case of ambiguities always returns the maximum height.
        //! @param x x-value
        //! @param y y-value
        //! @return z-Value at position given by x and y
        float getHeight(const float x, const float y) const;

        //! set/retrieve default height
        void setNullHeight(float h){nullHeight=h;}
        float getNullHeight() const {return nullHeight;}

private:
	IdxList indexList;
	PosList posList;

        static const int GRID_LENGTH = 60; // # of grid spaces is GRID_LENGTH^2

	typedef std::vector<const unsigned int*> FaceVector; //points to first index in Index list for a face

        struct GridSpace {
                FaceVector faces;
		float getHeight(const PosList &posList, const float x, const float y) const;

                //static float face_height_at(const OBJ& obj, const OBJ::Face* face, float x, float y);
		static float face_height_at(const PosList &obj, const unsigned int *pTriangle, const float x, const float y);
        };

        GridSpace* grid;
        float xMin, yMin;
        float xMax, yMax;
        float nullHeight; // return value for areas outside grid

        void initGrid();
        GridSpace* getSpace(const float x, const float y) const ;
	bool face_in_area(const unsigned int* pTriangle, const float xmin, const float ymin, const float xmax, const float ymax) const;

};

#endif // HEIGHTMAP_HPP
