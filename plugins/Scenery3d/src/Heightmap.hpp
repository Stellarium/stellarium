#ifndef HEIGHTMAP_HPP
#define HEIGHTMAP_HPP

#include "OBJ.hpp"

class Heightmap
{

public:

   Heightmap(const OBJ& obj);
   virtual ~Heightmap();

   float getHeight(float x, float y);

private:

	static const int GRID_LENGTH = 1; // # of grid spaces is GRID_LENGTH^2

   typedef std::vector<const OBJ::Face*> FaceVector;
   
   struct GridSpace {
      FaceVector faces;
      float getHeight(const OBJ& obj, float x, float y);
      
      static float face_height_at(const OBJ& obj, const OBJ::Face* face, float x, float y);
   };

   const OBJ& obj;
   GridSpace* grid;
   float xMin, yMin;
   float xMax, yMax;

   void initGrid();
   GridSpace* getSpace(float x, float y);
   bool face_in_area (const OBJ::Face* face, float xmin, float ymin, float xmax, float ymax);

};

#endif // HEIGHTMAP_HPP
