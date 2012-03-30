#include "AABB.hpp"

AABB::AABB(Vec3f min, Vec3f max)
{
    this->min = min;
    this->max = max;
}

AABB::~AABB() {}

Vec3f AABB::getCorner(Corner corner) const
{
    Vec3f out;

    switch(corner)
    {
    case MinMinMin:
        out = min;
        break;

    case MaxMinMin:
        out = Vec3f(max.v[0], min.v[1], min.v[2]);
        break;

    case MaxMaxMin:
        out = Vec3f(max.v[0], max.v[1], min.v[2]);
        break;

    case MinMaxMin:
        out = Vec3f(min.v[0], max.v[1], min.v[2]);
        break;

    case MinMinMax:
        out = Vec3f(min.v[0], min.v[1], max.v[2]);
        break;

    case MaxMinMax:
        out = Vec3f(max.v[0], min.v[1], max.v[2]);
        break;

    case MaxMaxMax:
        out = max;
        break;

    case MinMaxMax:
        out = Vec3f(min.v[0], max.v[1], max.v[2]);
        break;

    default:
        break;
    }

    return out;
}

Vec3f AABB::positiveVertex(Vec3f& normal) const
{
    Vec3f out = getCorner(MinMinMin);

    if(normal.v[0] >= 0)
        out.v[0] = max.v[0];
    if(normal.v[1] >= 0)
        out.v[1] = max.v[1];
    if(normal.v[2] >= 0)
        out.v[2] = max.v[2];

    return out;
}

Vec3f AABB::negativeVertex(Vec3f& normal) const
{
    Vec3f out = getCorner(MaxMaxMax);

    if(normal.v[0] >= 0)
        out.v[0] = min.v[0];
    if(normal.v[1] >= 0)
        out.v[1] = min.v[1];
    if(normal.v[2] >= 0)
        out.v[2] = min.v[2];

    return out;
}

void AABB::render()
{
    Vec3f ntl = getCorner(MinMaxMin);
    Vec3f ntr = getCorner(MaxMaxMin);
    Vec3f nbr = getCorner(MaxMinMin);
    Vec3f nbl = getCorner(MinMinMin);
    Vec3f ftr = getCorner(MaxMaxMax);
    Vec3f ftl = getCorner(MinMaxMax);
    Vec3f fbl = getCorner(MinMinMax);
    Vec3f fbr = getCorner(MaxMinMax);

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(5);
    glBegin(GL_LINE_LOOP);
        //near plane
        glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
        glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
        glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
        glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        //far plane
        glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
        glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
        glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
        glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        //bottom plane
        glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
        glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
        glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
        glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        //top plane
        glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
        glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
        glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
        glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        //left plane
        glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
        glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
        glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
        glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        // right plane
        glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
        glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
        glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
        glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
    glEnd();
}
