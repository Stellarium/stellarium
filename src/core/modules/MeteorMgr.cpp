/*
 * Stellarium
 * This file Copyright (C) 2004 Robert Spearman
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

#include <functional>
#include <cstdlib>
#include <QSettings>

#include "LandscapeMgr.hpp"
#include "Meteor.hpp"
#include "MeteorMgr.hpp"
#include "renderer/StelRenderer.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelProjector.hpp"

MeteorMgr::MeteorMgr(int zhr, int maxv ) : flagShow(true)
{
	setObjectName("MeteorMgr");
			
	ZHR = zhr;
	maxVelocity = maxv;

	// calculate factor for meteor creation rate per second since visible area ZHR is for
	// estimated visible radius of 458km
	// (calculated for average meteor magnitude of +2.5 and limiting magnitude of 5)

	//  zhrToWsr = 1.0f/3600.f;
	zhrToWsr = 1.6667f/3600.f;
	// this is a correction factor to adjust for the model as programmed to match observed rates
}

MeteorMgr::~MeteorMgr()
{
	for(std::vector<Meteor*>::iterator iter = active.begin(); iter != active.end(); ++iter)
	{
		delete *iter;
	}
	active.clear();
}

void MeteorMgr::init()
{
	setZHR(StelApp::getInstance().getSettings()->value("astro/meteor_rate", 10).toInt());
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double MeteorMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+10;
	return 0;
}

void MeteorMgr::setZHR(int zhr)
{
	ZHR = zhr;
	emit zhrChanged(zhr);
}

int MeteorMgr::getZHR()
{
	return ZHR;
}

void MeteorMgr::setMaxVelocity(int maxv)
{
	maxVelocity = maxv;
}

void MeteorMgr::update(double deltaTime)
{
	if (!flagShow)
		return;
	
	deltaTime*=1000;
	StelCore* core = StelApp::getInstance().getCore();

	// step through and update all active meteors
	for (std::vector<Meteor*>::iterator iter = active.begin(); iter != active.end(); ++iter)
	{
		if (!(*iter)->update(deltaTime))
		{
			// remove dead meteor
			//      qDebug("Meteor \tdied\n");

			delete *iter;
			active.erase(iter);
			iter--;  // important!
		}
	}

	// only makes sense given lifetimes of meteors to draw when timeSpeed is realtime
	// otherwise high overhead of large numbers of meteors
	double tspeed = core->getTimeRate()*86400;  // sky seconds per actual second
	if (tspeed<=0 || fabs(tspeed)>1.)
	{
		// don't start any more meteors
		return;
	}

	// if stellarium has been suspended, don't create huge number of meteors to
	// make up for lost time!
	if (deltaTime > 500)
	{
		deltaTime = 500;
	}

	// determine average meteors per frame needing to be created
	int mpf = (int)((double)ZHR*zhrToWsr*deltaTime/1000.0 + 0.5);
	if (mpf<1)
		mpf = 1;

	int mlaunch = 0;
	for (int i=0; i<mpf; ++i)
	{
		// start new meteor based on ZHR time probability
		double prob = ((double)rand())/RAND_MAX;
		if (ZHR>0 && prob<((double)ZHR*zhrToWsr*deltaTime/1000.0/(double)mpf) )
		{
			Meteor *m = new Meteor(StelApp::getInstance().getCore(), maxVelocity);
			active.push_back(m);
			mlaunch++;
		}
	}
	//  qDebug("mpf: %d\tm launched: %d\t(mps: %f)\t%d\n", mpf, mlaunch, ZHR*zhrToWsr, deltaTime);
}


void MeteorMgr::draw(StelCore* core, StelRenderer* renderer)
{
	if (!flagShow)
		return;
	
	LandscapeMgr* landmgr = (LandscapeMgr*)StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr");
	if (landmgr->getFlagAtmosphere() && landmgr->getLuminance()>5)
		return;

	renderer->setBlendMode(BlendMode_Alpha);

	// step through and draw all active meteors
	for (std::vector<Meteor*>::iterator iter = active.begin(); iter != active.end(); ++iter)
	{
		(*iter)->draw(core, core->getProjection(StelCore::FrameAltAz), renderer);
	}
}
