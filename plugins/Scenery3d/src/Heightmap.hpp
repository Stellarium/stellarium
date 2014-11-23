#ifndef HEIGHTMAP_HPP
#define HEIGHTMAP_HPP

#include "OBJ.hpp"

//! This represents a heightmap for viewer-ground collision
class Heightmap
{

public:

        //! Construct a heightmap from a loaded OBJ mesh.
        //! The mesh is stored as reference and used for calculations.
        //! @param obj Mesh for building the heightmap.
	Heightmap(OBJ *obj);
        virtual ~Heightmap();

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

        static const int GRID_LENGTH = 60; // # of grid spaces is GRID_LENGTH^2

        typedef std::vector<int> FaceVector;

        struct GridSpace {
                FaceVector faces;
		float getHeight(const OBJ& obj, const float x, const float y) const;

                //static float face_height_at(const OBJ& obj, const OBJ::Face* face, float x, float y);
		static float face_height_at(const OBJ& obj, const unsigned int *pTriangle, const float x, const float y);
        };

	OBJ* obj;
        GridSpace* grid;
        float xMin, yMin;
        float xMax, yMax;
        float nullHeight; // return value for areas outside grid

        void initGrid();
        GridSpace* getSpace(const float x, const float y) const ;
	bool face_in_area(const OBJ& obj, const unsigned int* pTriangle, const float xmin, const float ymin, const float xmax, const float ymax) const;

};

#endif // HEIGHTMAP_HPP
