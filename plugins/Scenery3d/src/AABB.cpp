#include "AABB.hpp"
#include "StelOpenGL.hpp"
#include <limits>

Box::Box()
{

}

void Box::transform(const QMatrix4x4& tf)
{
	for(int i =0;i<8;++i)
	{
		//this is a bit stupid, but only used for debugging, so...
		QVector3D vec(vertices[i].v[0],vertices[i].v[1],vertices[i].v[2]);
		vec = tf * vec;
		vertices[i] = Vec3f(vec.x(),vec.y(),vec.z());
	}
}

void Box::render() const
{
	Vec3f nbl = vertices[0];
	Vec3f nbr = vertices[1];
	Vec3f ntr = vertices[2];
	Vec3f ntl = vertices[3];
	Vec3f fbl = vertices[4];
	Vec3f fbr = vertices[5];
	Vec3f ftr = vertices[6];
	Vec3f ftl = vertices[7];

	glColor3f(1.0f,1.0f,1.0f);
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

AABB::AABB()
{
    float minVal = std::numeric_limits<float>::max();
    float maxVal = -std::numeric_limits<float>::max();

    AABB(Vec3f(minVal), Vec3f(maxVal));
}

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

Vec4f AABB::getEquation(AABB::Plane p) const
{
    Vec4f out;

    switch(p)
    {
    case Front:
        out = Vec4f(0.0f, -1.0f, 0.0f, std::abs(min.v[1]));
        break;

    case Back:
        out = Vec4f(0.0f, 1.0f, 0.0f, std::abs(max.v[1]));
        break;

    case Bottom:
        out = Vec4f(0.0f, 0.0f, -1.0f, std::abs(min.v[2]));
        break;

    case Top:
        out = Vec4f(0.0f, 0.0f, 1.0f, std::abs(max.v[2]));
        break;

    case Left:
        out = Vec4f(-1.0f, 0.0f, 0.0f, std::abs(min.v[0]));
        break;

    case Right:
        out = Vec4f(1.0f, 0.0f, 0.0f, std::abs(max.v[0]));
        break;

    default:
        break;
    }

    return out;
}

Vec3f AABB::positiveVertex(Vec3f& normal) const
{
    Vec3f out = min;

    if(normal.v[0] >= 0.0f)
        out.v[0] = max.v[0];
    if(normal.v[1] >= 0.0f)
        out.v[1] = max.v[1];
    if(normal.v[2] >= 0.0f)
        out.v[2] = max.v[2];

    return out;
}

Vec3f AABB::negativeVertex(Vec3f& normal) const
{
    Vec3f out = max;

    if(normal.v[0] >= 0.0f)
        out.v[0] = min.v[0];
    if(normal.v[1] >= 0.0f)
        out.v[1] = min.v[1];
    if(normal.v[2] >= 0.0f)
        out.v[2] = min.v[2];

    return out;
}

void AABB::expand(const Vec3f &v)
{
    if(v.v[0] > max.v[0]) max.v[0] = v.v[0];
    else if(v.v[0] < min.v[0]) min.v[0] = v.v[0];

    if(v.v[1] > max.v[1]) max.v[1] = v.v[1];
    else if(v.v[1] < min.v[1]) min.v[1] = v.v[1];

    if(v.v[2] > max.v[2]) max.v[2] = v.v[2];
    else if(v.v[2] < min.v[2]) min.v[2] = v.v[2];
}

void AABB::render() const
{
    Vec3f nbl = getCorner(MinMinMin);
    Vec3f nbr = getCorner(MaxMinMin);
    Vec3f ntr = getCorner(MaxMinMax);
    Vec3f ntl = getCorner(MinMinMax);
    Vec3f fbl = getCorner(MinMaxMin);
    Vec3f fbr = getCorner(MaxMaxMin);
    Vec3f ftr = getCorner(MaxMaxMax);
    Vec3f ftl = getCorner(MinMaxMax);

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

Box AABB::toBox()
{
	Box ret;
	for(int i =0;i<CORNERCOUNT;++i)
	{
		ret.vertices[i]= getCorner(static_cast<Corner>(i));
	}
	return ret;
}
