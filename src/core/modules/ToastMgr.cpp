/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#include <QDebug>

#include "ToastMgr.hpp"
#include "StelToast.hpp"
#include "StelFader.hpp"
#include "StelPainter.hpp"
#include "StelCore.hpp"
#include "StelApp.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyLayerMgr.hpp"

#include <QSettings>

ToastMgr::ToastMgr() :
	StelModule()
	, survey(Q_NULLPTR)
{	
	setObjectName("ToastMgr");
	fader = new LinearFader();
}

ToastMgr::~ToastMgr()
{
	delete fader;
	fader = Q_NULLPTR;
}

void ToastMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	QString toastHost = conf->value("astro/toast_survey_host", "http://dss.stellarium.org").toString();
	QString toastDir = conf->value("astro/toast_survey_directory", "survey").toString();
	int toastLevel = conf->value("astro/toast_survey_levels", 11).toInt();	
	survey = new ToastSurvey(toastHost+"/" + toastDir + "/{level}/{x}_{y}.jpg", toastLevel);
	survey->setParent(this);

	// Hide deep-sky survey by default
	setFlagShow(conf->value("astro/flag_toast_survey", false).toBool());

	addAction("actionShow_Toast_Survey", N_("Display Options"), N_("Digitized Sky Survey (TOAST)"), "surveyDisplayed");
}

void ToastMgr::deinit()
{
	delete survey;
	survey = Q_NULLPTR;
}

void ToastMgr::draw(StelCore* core)
{
	if (!getFlagShow())
		return;

	StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
	survey->draw(&sPainter);
}

void ToastMgr::update(double deltaTime)
{
	fader->update(static_cast<int>(deltaTime*1000));
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double ToastMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return 7;
	return 0;
}

void ToastMgr::setFlagShow(const bool displayed)
{
	if (*fader != displayed)
	{
		*fader = displayed;
		GETSTELMODULE(StelSkyLayerMgr)->setFlagShow(!displayed);
		emit surveyDisplayedChanged(displayed);
	}
}

bool ToastMgr::getFlagShow() const
{
	return *fader;
}
