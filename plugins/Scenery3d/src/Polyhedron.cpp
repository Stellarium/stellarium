/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza
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

/**
  * This implementation is based on Stingl's Robust Hard Shadows. */

#include "Polyhedron.hpp"
#include "GLFuncs.hpp"
#include <limits>

Polyhedron::Polyhedron()
{

}

Polyhedron::~Polyhedron()
{

}

void Polyhedron::clear()
{
	//TODO this frees the used memory, but about the same number of objects will get re-added soon after in a loop, leading to unnecssary reallocations. optimize?
	polygons.clear();
	uniqueVerts.clear();
}

void Polyhedron::add(const Frustum &f)
{
	//Front
	polygons.append(SPolygon(f.getCorner(Frustum::NBL), f.getCorner(Frustum::NBR), f.getCorner(Frustum::NTR), f.getCorner(Frustum::NTL)));
	//Back
	polygons.append(SPolygon(f.getCorner(Frustum::FTL), f.getCorner(Frustum::FTR), f.getCorner(Frustum::FBR), f.getCorner(Frustum::FBL)));
	//Left
	polygons.append(SPolygon(f.getCorner(Frustum::NBL), f.getCorner(Frustum::NTL), f.getCorner(Frustum::FTL), f.getCorner(Frustum::FBL)));
	//Right
	polygons.append(SPolygon(f.getCorner(Frustum::NBR), f.getCorner(Frustum::FBR), f.getCorner(Frustum::FTR), f.getCorner(Frustum::NTR)));
	//Bottom
	polygons.append(SPolygon(f.getCorner(Frustum::FBL), f.getCorner(Frustum::FBR), f.getCorner(Frustum::NBR), f.getCorner(Frustum::NBL)));
	//Top
	polygons.append(SPolygon(f.getCorner(Frustum::FTR), f.getCorner(Frustum::FTL), f.getCorner(Frustum::NTL), f.getCorner(Frustum::NTR)));
}

void Polyhedron::add(const SPolygon& p)
{
	polygons.append(p);
}

void Polyhedron::add(const QVector<Vec3f> &verts, const Vec3f &normal)
{
	if(verts.size() < 3) return;

	SPolygon p;

	for(int i=0; i<verts.size(); i++)
	{
		p.addUniqueVert(verts[i]);
	}

	if(p.vertices.size() < 3)
	{
		return;
	}

	//Check normal direction
	Plane polyPlane(p.vertices[0], p.vertices[1], p.vertices[2], SPolygon::CCW);

	//Might need to reverse the vertex order
	if(polyPlane.normal.dot(normal) < 0.0f) p.reverseOrder();

	polygons.append(p);
}

void Polyhedron::intersect(const AABBox &bb)
{
	for(unsigned int i=0; i<AABBox::FACECOUNT; i++)
	{
		intersect(Plane(bb.getPlane(static_cast<AABBox::Face>(i))));
	}
}

void Polyhedron::intersect(const Plane &p)
{
	//Save intersection points
	QVector<Vec3f> intersectionPoints;

	//Iterate over this polyhedron's polygons
	for (auto it = polygons.begin(); it != polygons.end();)
	{
		//Retrive the polygon and intersect it with the specified plane, save the intersection points
		(*it).intersect(p, intersectionPoints);

		//If all vertices were clipped, remove the polygon
		if(!(*it).vertices.size())
		{
			it = polygons.erase(it);
		}
		else
		{
			it++;
		}
	}

	//We need to add a closing polygon
	if(intersectionPoints.size()) add(intersectionPoints, p.normal);
}

void Polyhedron::intersect(const Line &l, const Vec3f &min, const Vec3f &max, QVector<Vec3f> &vertices)
{
	const Vec3f &dir = l.direction;
	const Vec3f &p = l.startPoint;

	float t1 = 0.0f;
	float t2 = std::numeric_limits<float>::infinity();

	bool intersect = clip(-dir.v[0], p.v[0]-min.v[0], t1, t2) && clip(dir.v[0], max.v[0]-p.v[0], t1, t2) &&
			 clip(-dir.v[1], p.v[1]-min.v[1], t1, t2) && clip(dir.v[1], max.v[1]-p.v[1], t1, t2) &&
			 clip(-dir.v[2], p.v[2]-min.v[2], t1, t2) && clip(dir.v[2], max.v[2]-p.v[2], t1, t2);


	if(!intersect) return;

	Vec3f newPoint;
	intersect = false;

	if(t1 >= 0.0f)
	{
		newPoint = p + t1*dir;
		intersect = true;
	}

	if(t2 >= 0.0f)
	{
		newPoint = p + t2*dir;
		intersect = true;
	}

	if(intersect) vertices.append(newPoint);
}

bool Polyhedron::clip(float p, float q, float &u1, float &u2) const
{
	if(p < 0.0f)
	{
		float r = q/p;

		if(r > u2)
		{
			return false;
		}
		else
		{
			if(r > u1)
			{
				u1 = r;
			}

			return true;
		}
	}
	else
	{
		if(p > 0.0f)
		{
			float r = q/p;

			if(r < u1)
			{
				return false;
			}
			else
			{
				if(r < u2)
				{
					u2 = r;
				}

				return true;
			}
		}
		else
		{
			return q >= 0.0f;
		}
	}
}

void Polyhedron::extrude(const Vec3f &dir, const AABBox &bb)
{
	makeUniqueVerts();

	const Vec3f &min = bb.min;
	const Vec3f &max = bb.max;

	unsigned int size = static_cast<unsigned int>(uniqueVerts.size());

	for(unsigned int i=0; i<size; i++)
	{
		intersect(Line(uniqueVerts[i], dir), min, max, uniqueVerts);
	}
}

void Polyhedron::makeUniqueVerts()
{
	uniqueVerts.clear();

	for(int i=0; i<polygons.size(); i++)
	{
		QVector<Vec3f> &verts = polygons[i].vertices;

		for(int j=0; j<verts.size(); j++)
		{
			addUniqueVert(verts[j]);
		}
	}
}

void Polyhedron::addUniqueVert(const Vec3f &v)
{
	bool flag = true;

	for(int i=0; i<uniqueVerts.size() && flag; i++)
	{
		flag = ! v.fuzzyEquals(uniqueVerts[i]);
	}

	if(flag) uniqueVerts.push_back(v);
}

int Polyhedron::getVertCount() const
{
	return uniqueVerts.size();
}

const QVector<Vec3f> &Polyhedron::getVerts() const
{
	return uniqueVerts;
}

void Polyhedron::render() const
{
#if !defined(QT_OPENGL_ES_2)

	//render each polygon
	glExtFuncs->glColor3f(0.4f,0.4f,0.4f);
	for(int i = 0;i<polygons.size();++i)
	{
		glExtFuncs->glBegin(GL_LINE_LOOP);
		const SPolygon& poly = polygons.at(i);
		for(int j = 0;j<poly.vertices.size();++j)
		{
			glExtFuncs->glVertex3fv(poly.vertices.at(j).v);
		}
		glExtFuncs->glEnd();
	}


	//also show the uniqueVerts
	glExtFuncs->glPointSize(4.0f);
	glExtFuncs->glColor3f(1.0f,1.0f,1.0f);


	glExtFuncs->glBegin(GL_POINTS);
	for(int i =0;i<uniqueVerts.size();++i)
	{
		glExtFuncs->glVertex3fv(uniqueVerts.at(i).v);
	}
	glExtFuncs->glEnd();
#endif
}
