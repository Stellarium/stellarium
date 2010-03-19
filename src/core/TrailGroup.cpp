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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "TrailGroup.hpp"
#include "StelNavigator.hpp"
#include "StelApp.hpp"
#include "StelPainter.hpp"
#include "StelObject.hpp"
#include "Planet.hpp"
#include <QtOpenGL>

TrailGroup::TrailGroup(float te) : timeExtent(te), opacity(1.f)
{
	j2000ToTrailNative=Mat4d::identity();
	j2000ToTrailNativeInverted=Mat4d::identity();
}

void TrailGroup::draw(StelCore* core, StelPainter* sPainter)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float currentTime = core->getNavigator()->getJDay();
	sPainter->setProjector(core->getProjection(core->getNavigator()->getJ2000ModelViewMat()*j2000ToTrailNativeInverted));
	for (QMap<StelObjectP, Trail>::Iterator iter = allTrails.begin();iter!=allTrails.end();++iter)
	{
		Planet* hpl = dynamic_cast<Planet*>(iter.key().data());
		if (hpl!=NULL)
		{
			// Avoid drawing the trails if the object is the home planet
			QString homePlanetName = hpl==NULL ? "" : hpl->getEnglishName();
			if (homePlanetName==StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation().planetName)
				continue;
		}
		const QList<Vec3d>& posHistory = iter.value().posHistory;
		Vec3f color;
		for (int i=0;i<posHistory.size()-1;++i)
		{
			float colorRatio = 1.f-(currentTime-times.at(i))/timeExtent;
			color = iter.key()->getInfoColor();
			sPainter->setColor(color[0], color[1], color[2], colorRatio*opacity);
			sPainter->drawGreatCircleArc(posHistory.at(i), posHistory.at(i+1));
		}
	}
}

// Add 1 point to all the curves at current time and suppress too old points
void TrailGroup::update()
{
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
	times.append(nav->getJDay());
	QMap<StelObjectP, Trail>::Iterator iter = allTrails.begin();
	for (QMap<StelObjectP, Trail>::Iterator iter = allTrails.begin();iter!=allTrails.end();++iter)
	{
		iter.value().posHistory.append(j2000ToTrailNative*iter.key()->getJ2000EquatorialPos(nav));
	}
	float ct = nav->getJDay();
	int i;
	for (i=times.size()-1;i>=0;--i)
	{
		if (ct-times.at(i)>timeExtent)
			break;
	}
	if (i>=0)
	{
		// Remove too old points
		times.erase(times.begin(),times.begin()+i);
		for (QMap<StelObjectP, Trail>::Iterator iter = allTrails.begin();iter!=allTrails.end();++iter)
		{
			iter.value().posHistory.erase(iter.value().posHistory.begin(),iter.value().posHistory.begin()+i);
		}
	}
}

// Set the matrix to use to post process J2000 positions before storing in the trail
void TrailGroup::setJ2000ToTrailNative(const Mat4d& m)
{
	j2000ToTrailNative=m;
	j2000ToTrailNativeInverted=m.inverse();
}

void TrailGroup::addObject(const StelObjectP& obj)
{
	allTrails.insert(obj,TrailGroup::Trail(obj));
}

void TrailGroup::removeObject(const StelObjectP& obj)
{
	allTrails.remove(obj);
}

void TrailGroup::reset()
{
	times.clear();
	for (QMap<StelObjectP, Trail>::Iterator iter = allTrails.begin();iter!=allTrails.end();++iter)
	{
		iter.value().posHistory.clear();
	}
}
