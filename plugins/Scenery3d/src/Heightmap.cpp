#include "Heightmap.hpp"

Heightmap::Heightmap(const OBJ& obj)
{
    // ALGORITHM
    // 1. loop through all vertices to determine xmin, xmax, ymin, ymax
    // 2. allocate memory for height grid (xmax-xmin) x (ymax - ymin)
    // 3. init height grid to -INFINITY
    // 4. loop through all faces
    // 4-1. determine xmin, xmax, ymin, ymax for the face
    // 4-2. set grid points inside the face to MAX(v_old, v_new), where
    //      v_old is the old grid point value and v_new is the interpolated
    //      value from the face
}

Heightmap::~Heightmap()
{
}

float Heightmap::getHeight(float x, float y)
{
}
