#include "Plane.hpp"
#include "Util.hpp"

Plane::Plane()
{
}

Plane::Plane(Vec3f &v1, Vec3f &v2, Vec3f &v3)
{
    setPoints(v1, v2, v3);
}

Plane::~Plane() {}

void Plane::setPoints(Vec3f &v1, Vec3f &v2, Vec3f &v3)
{
    Vec3f edge1 = v1-v2;
    Vec3f edge2 = v3-v2;

    normal = edge2^edge1;
    normal.normalize();

    p = Vec3f(v2.v[0], v2.v[1], v2.v[2]);
    distance = -(normal.dot(p));
}

float Plane::calcDistance(Vec3f p) const
{
    return (distance + normal.dot(p));
}

bool Plane::isBehind(Vec3f p)
{
    bool result = false;

    if(calcDistance(p) < 0.0f)
        result = true;

    return result;
}

void Plane::saveValues()
{
    sNormal = normal;
    sP = p;
    sDistance = distance;
}
