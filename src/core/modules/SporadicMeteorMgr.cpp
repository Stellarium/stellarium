/*
 * Stellarium
 * Copyright (C) 2015 Marcos Cardinot
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

#include "LandscapeMgr.hpp"
#include "SolarSystem.hpp"
#include "SporadicMeteorMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelPainter.hpp"
#include "StelTextureMgr.hpp"

#include <QSettings>

SporadicMeteorMgr::SporadicMeteorMgr(int zhr, int maxv)
	: m_zhr(zhr)
	, m_maxVelocity(maxv)
	, m_flagShow(true)
	, m_flagForcedShow(false)
{
	setObjectName("SporadicMeteorMgr");
}

SporadicMeteorMgr::~SporadicMeteorMgr()
{
	qDeleteAll(activeMeteors);
	activeMeteors.clear();
	m_bolideTexture.clear();
}

void SporadicMeteorMgr::init()
{
	m_bolideTexture = StelApp::getInstance().getTextureManager().createTextureThread(
				StelFileMgr::getInstallationDir() + "/textures/cometComa.png",
				StelTexture::StelTextureParams(true, GL_LINEAR, GL_CLAMP_TO_EDGE));

	QSettings* conf = StelApp::getInstance().getSettings();
	setZHR(conf->value("astro/meteor_zhr", 10).toInt());
	setFlagForcedMeteorsActivity(conf->value("astro/flag_forced_meteor_activity", false).toBool());
}

double SporadicMeteorMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
	{
		return GETSTELMODULE(SolarSystem)->getCallOrder(actionName) + 10.;
	}
	return 0;
}

void SporadicMeteorMgr::update(double deltaTime)
{
	if (!m_flagShow)
	{
		return;
	}

	// step through and update all active meteors
	foreach (SporadicMeteor* m, activeMeteors)
	{
		if (!m->update(deltaTime))
		{
			//important to delete when no longer active
			activeMeteors.removeOne(m);
			delete m;
		}
	}

	StelCore* core = StelApp::getInstance().getCore();

	// going forward/backward OR current ZHR is zero ?
	// don't create new meteors
	if(!core->getRealTimeSpeed() || m_zhr < 1)
	{
		return;
	}

	// average meteors per frame
	float mpf = m_zhr * static_cast<float>(deltaTime) / 3600.f;

	// maximum amount of meteors for the current frame
	int maxMpf = qRound(mpf);
	maxMpf = maxMpf < 1 ? 1 : maxMpf;

	float rate = mpf / static_cast<float>(maxMpf);
	for (int i = 0; i < maxMpf; ++i)
	{
		float prob = static_cast<float>(qrand()) / static_cast<float>(RAND_MAX);
		if (prob < rate)
		{
			SporadicMeteor* m = new SporadicMeteor(core, m_maxVelocity, m_bolideTexture);
			if (m->isAlive())
			{
				activeMeteors.append(m);
			}
			else
			{
				delete m;
			}
		}
	}
}

void SporadicMeteorMgr::draw(StelCore* core)
{
	if (!m_flagShow || (!core->getSkyDrawer()->getFlagHasAtmosphere() && !getFlagForcedMeteorsActivity()))
	{
		return;
	}

	LandscapeMgr* landmgr = GETSTELMODULE(LandscapeMgr);
	if (landmgr->getFlagAtmosphere() && landmgr->getLuminance() > 5.f)
	{
		return;
	}

	// step through and draw all active meteors
	StelPainter sPainter(core->getProjection(StelCore::FrameAltAz));
	for (auto* m: activeMeteors)
	{
		m->draw(core, sPainter);
	}
}

void SporadicMeteorMgr::setZHR(int zhr)
{
	if(zhr!=m_zhr)
	{
		m_zhr = zhr;
		emit zhrChanged(zhr);
	}
}
