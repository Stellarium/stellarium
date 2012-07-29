/*
 * Stellarium
 * Copyright (C) 2010 Fabien Chereau
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

#include "TrailGroup.hpp"

#include "StelApp.hpp"
#include "StelObject.hpp"
#include "Planet.hpp"
#include "renderer/StelRenderer.hpp"

TrailGroup::TrailGroup(float te) 
	: timeExtent(te)
	, opacity(1.0f)
	, lastTimeInHistory(0.0)
{
	j2000ToTrailNative=Mat4d::identity();
	j2000ToTrailNativeInverted=Mat4d::identity();
}

TrailGroup::~TrailGroup()
{
	for(int t = 0; t < allTrails.size(); t++) 
	{
		delete allTrails[t].vertexBuffer;
	}
}

void TrailGroup::draw(StelCore* core, StelRenderer* renderer)
{
	renderer->setBlendMode(BlendMode_Alpha);
	float currentTime = core->getJDay();
	StelProjector::ModelViewTranformP transfo = core->getJ2000ModelViewTransform();
	transfo->combine(j2000ToTrailNativeInverted);
	StelProjectorP projector = core->getProjection(transfo);


	// We skip some positions to avoid drawing too many vertices when the history is long.
	int totalPositions = 0;
	foreach(const Trail& trail, allTrails)
	{
		totalPositions += trail.posHistory.size();
	}
	const int desiredMaxTotalTrailVertices = 8192;
	const int skip = std::max(1, totalPositions / desiredMaxTotalTrailVertices);

	for(int t = 0; t < allTrails.size(); t++) 
	{
		Trail& trail(allTrails[t]);
		Planet* hpl = dynamic_cast<Planet*>(trail.stelObject.data());
		if (hpl != NULL)
		{
			// Avoid drawing the trails if the object is the home planet
			QString homePlanetName = hpl == NULL ? "" : hpl->getEnglishName();
			if (homePlanetName == StelApp::getInstance().getCore()->getCurrentLocation().planetName)
			{
				continue;
			}
		}

		const QVector<Vec3f>& posHistory = trail.posHistory;
		// Construct the vertex buffer if not yet constructed, otherwise clear it.
		if(NULL == trail.vertexBuffer)
		{
			trail.vertexBuffer = 
				renderer->createVertexBuffer<ColoredVertex>(PrimitiveType_LineStrip);
		}
		else
		{
			trail.vertexBuffer->unlock();
			trail.vertexBuffer->clear();
		}

		// Hand-optimized as this previously took up to 9% of time when
		// trail groups were enabled. Now it's up to 3%. Further optimization 
		// is possible, but would lead to more uglification.
		int posCount = posHistory.size();
		int p = 0;
		const float timeMult = 1.0f / timeExtent;
		ColoredVertex vertex;
		for (p = 0; p < posCount - 1; ++p)
		{
			// Skip if too many positions, but don't skip the last position.
			if(p % skip != 0)
			{
				continue;
			}
			const float colorRatio = 1.0f - (currentTime - times.at(p)) * timeMult;
			vertex.color = Vec4f(trail.color[0], trail.color[1], trail.color[2], colorRatio * opacity);
			vertex.position = posHistory.at(p);
			trail.vertexBuffer->addVertex(vertex);
		}
		// Last vertex (separate to avoid a branch in the code above)
		const float colorRatio = 1.0f - (currentTime - times.at(p)) * timeMult;
		vertex.color = Vec4f(trail.color[0], trail.color[1], trail.color[2], colorRatio * opacity);
		vertex.position = posHistory.at(p);
		trail.vertexBuffer->addVertex(vertex);

		trail.vertexBuffer->lock();

		renderer->drawVertexBuffer(trail.vertexBuffer, NULL, projector);
	}
}

// Add 1 point to all the curves at current time and suppress too old points
void TrailGroup::update()
{
	const double time = StelApp::getInstance().getCore()->getJDay();
	const double timeDelta = 
		times.size() == 0 ? 1000.0 : std::fabs(lastTimeInHistory - time);
	// Only add a position if enough time has passed.
	if(timeDelta > StelCore::JD_MINUTE)
	{
		times.append(time);
		lastTimeInHistory = time;
		for (QList<Trail>::Iterator iter=allTrails.begin();iter!=allTrails.end();++iter)
		{
			const Vec3d p = j2000ToTrailNative *
			                iter->stelObject->getJ2000EquatorialPos(StelApp::getInstance().getCore());
			iter->posHistory.append(Vec3f(p[0], p[1], p[2]));
		}
	}
	if (StelApp::getInstance().getCore()->getJDay()-times.at(0)>timeExtent)
	{
		times.pop_front();
		for (QList<Trail>::Iterator iter=allTrails.begin();iter!=allTrails.end();++iter)
		{
			iter->posHistory.pop_front();
		}
	}
}

// Set the matrix to use to post process J2000 positions before storing in the trail
void TrailGroup::setJ2000ToTrailNative(const Mat4d& m)
{
	j2000ToTrailNative=m;
	j2000ToTrailNativeInverted=m.inverse();
}

void TrailGroup::addObject(const StelObjectP& obj, const Vec3f* col)
{
	allTrails.append(TrailGroup::Trail(obj, col==NULL ? obj->getInfoColor() : *col));
}

void TrailGroup::reset()
{
	times.clear();
	for (QList<Trail>::Iterator iter=allTrails.begin();iter!=allTrails.end();++iter)
	{
		iter->posHistory.clear();
	}
}
