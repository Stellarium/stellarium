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
#include "StelPainter.hpp"
#include "StelObject.hpp"
#include "Planet.hpp"

TrailGroup::TrailGroup(float te, int maxPoints) : timeExtent(te), maxPoints(maxPoints), opacity(1.f)
{
	j2000ToTrailNative=Mat4d::identity();
	j2000ToTrailNativeInverted=Mat4d::identity();
	core=StelApp::getInstance().getCore();
	Q_ASSERT(core);
}

static QVector<Vec3d> vertexArray;
static QVector<Vec4f> colorArray;
void TrailGroup::draw(StelCore* core, StelPainter* sPainter)
{
	sPainter->setBlending(true);
	float currentTime = static_cast<float>(core->getJDE());
	StelProjector::ModelViewTranformP transfo = core->getJ2000ModelViewTransform();
	transfo->combine(j2000ToTrailNativeInverted);
	sPainter->setProjector(core->getProjection(transfo));
	for (const auto& trail : qAsConst(allTrails))
	{
		Planet* hpl = dynamic_cast<Planet*>(trail.stelObject.data());
		if (hpl!=Q_NULLPTR)
		{
			// Avoid drawing the trails if the object is the home planet
			QString homePlanetName = hpl->getEnglishName();
			if (homePlanetName==core->getCurrentLocation().planetName)
				continue;
		}
		const QList<Vec3d>& posHistory = trail.posHistory;
		vertexArray.resize(posHistory.size());
		colorArray.resize(posHistory.size());
		for (int i=0;i<posHistory.size();++i)
		{
			float colorRatio = 1.f-fabsf(currentTime-times.at(i))/timeExtent;
			colorArray[i].set(trail.color[0], trail.color[1], trail.color[2], colorRatio*opacity);
			vertexArray[i]=posHistory.at(i);
		}
		sPainter->drawPath(vertexArray, colorArray);
	}
}

// Add 1 point to all the curves at current time and remove too old points
void TrailGroup::update()
{
	float newJDE=static_cast<float>(core->getJDE());
	if (times.isEmpty() || fabsf(times.last()-newJDE) > 0.000001f)
	{
		times.append(newJDE);
		for (auto& trail : allTrails)
		{
			trail.posHistory.append(j2000ToTrailNative * trail.stelObject->getJ2000EquatorialPos(core));
		}
		if (fabs(static_cast<float>(core->getJDE())-times.at(0))>timeExtent || times.length()>maxPoints)
		{
			times.pop_front();
			for (auto& trail : allTrails)
			{
				trail.posHistory.pop_front();
			}
		}
	}
}

void TrailGroup::addObject(const StelObjectP& obj, const Vec3f* col)
{
	allTrails.append(TrailGroup::Trail(obj, col==Q_NULLPTR ? obj->getInfoColor() : *col));
}

void TrailGroup::reset(int maxPoints)
{
	this->maxPoints=maxPoints;
	times.clear();
	for (auto& trail : allTrails)
	{
		trail.posHistory.clear();
	}
}
