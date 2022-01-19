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

#include "SPolygon.hpp"
#include "Plane.hpp"
#include "GLFuncs.hpp"

SPolygon::SPolygon() {}

SPolygon::SPolygon(const Vec3f &c0, const Vec3f &c1, const Vec3f &c2, const Vec3f &c3)
{
	vertices.push_back(c0);
	vertices.push_back(c1);
	vertices.push_back(c2);
	vertices.push_back(c3);
}

SPolygon::~SPolygon() {}

void SPolygon::intersect(const Plane &p, QVector<Vec3f> &intersectionPoints)
{
	if(vertices.size() < 3) return;

	QVector<Vec3f> newVerts;

	for(int i=0; i<vertices.size(); i++)
	{
		int next = (i+1) % vertices.size();

		bool curOut = !p.isBehind(vertices[i]);
		bool nextOut = !p.isBehind(vertices[next]);

		//Both are outside, skip to next iteration
		if(curOut && nextOut) continue;

		float val = 0.0f;
		Line line(vertices[i], vertices[next]-vertices[i]);

		//outside -> inside intersection
		if(curOut)
		{
			if(p.intersect(line, val))
			{
				Vec3f intersection = line.getPoint(val);
				newVerts.append(intersection);
				intersectionPoints.append(intersection);
			}

			newVerts.append(vertices[next]);
			continue;
		}

		//inside -> outside intersection
		if(nextOut)
		{
			if(p.intersect(line, val))
			{
				Vec3f intersection = line.getPoint(val);
				newVerts.append(intersection);
				intersectionPoints.append(intersection);
			}

			continue;
		}

		//since both are inside, just add the next vertex
		newVerts.append(vertices[next]);
	}

	vertices.clear();

	//Polygon degenerated
	if(newVerts.size() > 2)
	{
		vertices = newVerts;
	}
}

void SPolygon::reverseOrder()
{
	if(vertices.size() > 2)
	{
		std::reverse(vertices.begin(), vertices.end());
	}
}

void SPolygon::addUniqueVert(const Vec3f &v)
{
	bool flag = true;

	for(int i=0; i<vertices.size() && flag; i++)
	{
		flag = ! v.fuzzyEquals(vertices[i]);
	}

	if(flag)
	{
		vertices.append(v);
	}
}

void SPolygon::render()
{
#if !defined(QT_OPENGL_ES_2)
	//render each polygon
	glExtFuncs->glColor3f(0.4f,0.4f,0.4f);

	glExtFuncs->glBegin(GL_LINE_LOOP);
	for(int j = 0;j<vertices.size();++j)
	{
		glExtFuncs->glVertex3fv(vertices.at(j).v);
	}
	glExtFuncs->glEnd();
#endif
}
