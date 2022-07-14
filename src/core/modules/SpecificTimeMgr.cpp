/*
 * Copyright (C) 2022 Alexander Wolf
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

#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "SpecificTimeMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelTranslator.hpp"

#include <QSettings>

SpecificTimeMgr::SpecificTimeMgr()
	: twilightAltitude(0.)
{
	setObjectName("SpecificTimeMgr");
	conf = StelApp::getInstance().getSettings();
	core = StelApp::getInstance().getCore();
	objMgr = GETSTELMODULE(StelObjectMgr);
	PlanetP sun=GETSTELMODULE(SolarSystem)->getSun();
}

SpecificTimeMgr::~SpecificTimeMgr()
{
	//
}

double SpecificTimeMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	return 0;
}

void SpecificTimeMgr::init()
{
	// Register all the core actions.
	QString timeGroup = N_("Specific Time");
	StelActionMgr* actionsMgr = StelApp::getInstance().getStelActionManager();
	actionsMgr->addAction("actionNext_Transit",		timeGroup, N_("Next transit of the selected object"),     this, "nextTransit()");
	actionsMgr->addAction("actionNext_Rising",		timeGroup, N_("Next rising of the selected object"),      this, "nextRising()");
	actionsMgr->addAction("actionNext_Setting",		timeGroup, N_("Next setting of the selected object"),     this, "nextSetting()");
	actionsMgr->addAction("actionToday_Transit",		timeGroup, N_("Today's transit of the selected object"),  this, "todayTransit()");
	actionsMgr->addAction("actionToday_Rising",		timeGroup, N_("Today's rising of the selected object"),   this, "todayRising()");
	actionsMgr->addAction("actionToday_Setting",		timeGroup, N_("Today's setting of the selected object"),  this, "todaySetting()");
	actionsMgr->addAction("actionPrevious_Transit",		timeGroup, N_("Previous transit of the selected object"), this, "previousTransit()");
	actionsMgr->addAction("actionPrevious_Rising",		timeGroup, N_("Previous rising of the selected object"),  this, "previousRising()");
	actionsMgr->addAction("actionPrevious_Setting",		timeGroup, N_("Previous setting of the selected object"), this, "previousSetting()");

	actionsMgr->addAction("actionNext_MorningTwilight",     timeGroup, N_("Next morning twilight"),     this, "nextMorningTwilight()");
	actionsMgr->addAction("actionNext_EveningTwilight",     timeGroup, N_("Next evening twilight"),     this, "nextEveningTwilight()");
	actionsMgr->addAction("actionToday_MorningTwilight",    timeGroup, N_("Today's morning twilight"),  this, "todayMorningTwilight()");
	actionsMgr->addAction("actionToday_EveningTwilight",    timeGroup, N_("Today's evening twilight"),  this, "todayEveningTwilight()");
	actionsMgr->addAction("actionPrevious_MorningTwilight", timeGroup, N_("Previous morning twilight"), this, "previousMorningTwilight()");
	actionsMgr->addAction("actionPrevious_EveningTwilight", timeGroup, N_("Previous evening twilight"), this, "previousEveningTwilight()");

	actionsMgr->addAction("actionNext_MorningAtAltitude",     timeGroup, N_("Selected object at altitude at next morning"),      this, "nextMorningAtAltitude()");
	actionsMgr->addAction("actionToday_MorningAtAltitude",    timeGroup, N_("Selected object at altitude this morning"),         this, "todayMorningAtAltitude()");
	actionsMgr->addAction("actionPrevious_MorningAtAltitude", timeGroup, N_("Selected object at altitude at previous morning"),  this, "previousMorningAtAltitude()");
	actionsMgr->addAction("actionNext_EveningAtAltitude",     timeGroup, N_("Selected object at altitude at next evening"),      this, "nextEveningAtAltitude()");
	actionsMgr->addAction("actionToday_EveningAtAltitude",    timeGroup, N_("Selected object at altitude this evening"),         this, "todayEveningAtAltitude()");
	actionsMgr->addAction("actionPrevious_EveningAtAltitude", timeGroup, N_("Selected object at altitude at previous evening"),  this, "previousEveningAtAltitude()");


	setTwilightAltitude(conf->value("astro/twilight_altitude", -6.).toDouble());
}

void SpecificTimeMgr::deinit()
{

}

void SpecificTimeMgr::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);

}

void SpecificTimeMgr::setTwilightAltitude(double alt)
{
	if (!qFuzzyCompare(alt, twilightAltitude))
	{
		twilightAltitude=alt;
		emit twilightAltitudeChanged(alt);
	}
}

void SpecificTimeMgr::nextTransit()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[1]);
	}
}

void SpecificTimeMgr::nextRising()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::nextSetting()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::previousTransit()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[1]);
	}
}

void SpecificTimeMgr::previousRising()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::previousSetting()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::todayTransit()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[1]);
	}
}

void SpecificTimeMgr::todayRising()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::todaySetting()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		Vec4d rts = selected[0]->getRTSTime(core);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::todayMorningTwilight()
{
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
			core->setJD(rts[0]);
}

void SpecificTimeMgr::todayEveningTwilight()
{
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
			core->setJD(rts[2]);
}

void SpecificTimeMgr::previousMorningTwilight()
{
	core->addSolarDays(-1.0);
	core->update(0);
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[0]);
}

void SpecificTimeMgr::previousEveningTwilight()
{
	core->addSolarDays(-1.0);
	core->update(0);
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[2]);
}

void SpecificTimeMgr::nextMorningTwilight()
{
	core->addSolarDays(1.0);
	core->update(0);
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[0]);
}

void SpecificTimeMgr::nextEveningTwilight()
{
	core->addSolarDays(1.0);
	core->update(0);
	Vec4d rts = sun->getRTSTime(core, twilightAltitude);
	if (rts[3]>-1000.)
		core->setJD(rts[2]);
}

void SpecificTimeMgr::todayMorningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::nextMorningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::previousMorningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[0]);
	}
}

void SpecificTimeMgr::todayEveningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::nextEveningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		core->addSolarDays(1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}

void SpecificTimeMgr::previousEveningAtAltitude()
{
	const QList<StelObjectP> selected = objMgr->getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		double az, alt;
		StelUtils::rectToSphe(&az, &alt, selected[0]->getAltAzPosGeometric(core));
		core->addSolarDays(-1.0);
		core->update(0);
		Vec4d rts = selected[0]->getRTSTime(core, alt*M_180_PI);
		if (rts[3]>-1000.)
			core->setJD(rts[2]);
	}
}
