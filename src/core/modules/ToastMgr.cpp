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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

ToastMgr::ToastMgr()
{	
	setObjectName("ToastMgr");
	fader = new LinearFader();
}

ToastMgr::~ToastMgr()
{
	if (survey)
	{
		delete survey;
		survey = NULL;
	}

	delete fader;
	fader = NULL;
}

void ToastMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	QString dssHost = conf->value("dss/host", "http://dss.astro.altspu.ru").toString();
	// TODO: change "results" to "survey" before release
	survey = new ToastSurvey(dssHost+"/results/{level}/{x}_{y}.jpg", 11);
	survey->setParent(this);

	// Hide deep-sky survey by default
	setFlagSurveyShow(conf->value("astro/flag_toast_survey", false).toBool());

	addAction("actionShow_Toast_Survey", N_("Display Options"), N_("Digitized sky survey"), "surveyDisplayed", "Ctrl+Alt+D");
}

void ToastMgr::draw(StelCore* core)
{
	if (!getFlagSurveyShow())
		return;

	StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
	survey->draw(&sPainter);
}

void ToastMgr::update(double deltaTime)
{
	fader->update((int)(deltaTime*1000));
}

void ToastMgr::setFlagSurveyShow(const bool displayed)
{
	if (*fader != displayed)
	{
		*fader = displayed;
		GETSTELMODULE(StelSkyLayerMgr)->setFlagShow(!displayed);
		emit surveyDisplayedChanged(displayed);
	}
}

bool ToastMgr::getFlagSurveyShow() const
{
	return *fader;
}
