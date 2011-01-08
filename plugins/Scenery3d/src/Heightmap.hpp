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
    long xStart, yStart;
    long xLength, yLength;
    float* heightData;
};

#endif // HEIGHTMAP_HPP
