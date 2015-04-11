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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "AABB.hpp"
#include "GLFuncs.hpp"
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
// Minimum to avoid trouble when building on pure OpenGL ES systems
// Not sure about ANGLE!
#if !defined(QT_OPENGL_ES_2)
	Vec3f nbl = vertices[0];
	Vec3f nbr = vertices[1];
	Vec3f ntr = vertices[2];
	Vec3f ntl = vertices[3];
	Vec3f fbl = vertices[4];
	Vec3f fbr = vertices[5];
	Vec3f ftr = vertices[6];
	Vec3f ftl = vertices[7];

	glExtFuncs->glColor3f(1.0f,1.0f,1.0f);
	float oldLineWidth;
	glExtFuncs->glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);
	glExtFuncs->glLineWidth(5);
	glExtFuncs->glBegin(GL_LINE_LOOP);
		//near plane
		glExtFuncs->glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
		glExtFuncs->glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
		glExtFuncs->glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
		glExtFuncs->glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//far plane
		glExtFuncs->glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
		glExtFuncs->glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
		glExtFuncs->glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
		glExtFuncs->glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//bottom plane
		glExtFuncs->glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
		glExtFuncs->glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
		glExtFuncs->glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
		glExtFuncs->glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//top plane
		glExtFuncs->glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
		glExtFuncs->glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
		glExtFuncs->glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
		glExtFuncs->glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//left plane
		glExtFuncs->glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
		glExtFuncs->glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
		glExtFuncs->glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
		glExtFuncs->glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		// right plane
		glExtFuncs->glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
		glExtFuncs->glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
		glExtFuncs->glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
		glExtFuncs->glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
	glExtFuncs->glEnd();
	glExtFuncs->glLineWidth(oldLineWidth);

#endif
}

AABB::AABB()
{
	*this = AABB(std::numeric_limits<float>::max(),-std::numeric_limits<float>::max());
}

AABB::AABB(Vec3f min, Vec3f max)
{
	this->min = min;
	this->max = max;
}

AABB::~AABB() {}

void AABB::reset()
{
	*this = AABB();
}

void AABB::resetToZero()
{
	*this = AABB(Vec3f(0.0f),Vec3f(0.0f));
}

void AABB::expand(const Vec3f &vec)
{
	min = Vec3f(	std::min(vec.v[0], min.v[0]),
			std::min(vec.v[1], min.v[1]),
			std::min(vec.v[2], min.v[2]));
	max = Vec3f(	std::max(vec.v[0], max.v[0]),
			std::max(vec.v[1], max.v[1]),
			std::max(vec.v[2], max.v[2]));
}

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
			out = Vec4f(0.0f, -1.0f, 0.0f, -min.v[1]);
			break;
		case Back:
			out = Vec4f(0.0f, 1.0f, 0.0f, max.v[1]);
			break;
		case Bottom:
			out = Vec4f(0.0f, 0.0f, -1.0f, -min.v[2]);
			break;
		case Top:
			out = Vec4f(0.0f, 0.0f, 1.0f, max.v[2]);
			break;
		case Left:
			out = Vec4f(-1.0f, 0.0f, 0.0f, -min.v[0]);
			break;
		case Right:
			out = Vec4f(1.0f, 0.0f, 0.0f, max.v[0]);
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

void AABB::render() const
{
// Minimum to avoid trouble when building on pure OpenGL ES systems
// Not sure about ANGLE!
#if !defined(QT_OPENGL_ES_2)
	Vec3f nbl = getCorner(MinMinMin);
	Vec3f nbr = getCorner(MaxMinMin);
	Vec3f ntr = getCorner(MaxMinMax);
	Vec3f ntl = getCorner(MinMinMax);
	Vec3f fbl = getCorner(MinMaxMin);
	Vec3f fbr = getCorner(MaxMaxMin);
	Vec3f ftr = getCorner(MaxMaxMax);
	Vec3f ftl = getCorner(MinMaxMax);

	glExtFuncs->glColor3f(1.0f, 1.0f, 1.0f);
	float oldLineWidth;
	glExtFuncs->glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);
	glExtFuncs->glLineWidth(5);
	glExtFuncs->glBegin(GL_LINE_LOOP);
		//near plane
		glExtFuncs->glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
		glExtFuncs->glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
		glExtFuncs->glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
		glExtFuncs->glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//far plane
		glExtFuncs->glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
		glExtFuncs->glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
		glExtFuncs->glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
		glExtFuncs->glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//bottom plane
		glExtFuncs->glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
		glExtFuncs->glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
		glExtFuncs->glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
		glExtFuncs->glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//top plane
		glExtFuncs->glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
		glExtFuncs->glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
		glExtFuncs->glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
		glExtFuncs->glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//left plane
		glExtFuncs->glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
		glExtFuncs->glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
		glExtFuncs->glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
		glExtFuncs->glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		// right plane
		glExtFuncs->glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
		glExtFuncs->glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
		glExtFuncs->glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
		glExtFuncs->glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
	glExtFuncs->glEnd();
	glExtFuncs->glLineWidth(oldLineWidth);
#endif
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
