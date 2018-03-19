/*
 * Stellarium
 * Copyright (C) 2017 Guillaume Chereau
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

#include "HipsMgr.hpp"
#include "StelHips.hpp"
#include "StelPainter.hpp"
#include "StelCore.hpp"
#include "StelApp.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyLayerMgr.hpp"

#include <QNetworkReply>
#include <QSettings>

HipsMgr::HipsMgr()
{
	setObjectName("HipsMgr");
	connect(this, SIGNAL(surveysChanged()), this, SLOT(restoreVisibleSurveys()));
}

HipsMgr::~HipsMgr()
{
	//store active HIPS to config.ini
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("hips");
	conf->setValue("show", getFlagShow());
	conf->beginGroup("visible");
	if (surveys.count()>0)
		conf->remove(""); // Don't clear list in case of just a short offline period.
	for (auto survey: surveys)
	{
		if (survey->isVisible() && survey->planet.isEmpty())
		{
			conf->setValue(survey->getUrl(), "true");
		}
	}
	conf->endGroup();
	conf->endGroup();
	conf->sync();
}

void HipsMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("hips");
	setFlagShow(conf->value("show", true).toBool());

	// Start to load survey lists
	QStringList sources;
	int size = conf->beginReadArray("sources");
	for (int i = 0; i < size; i++)
	{
		conf->setArrayIndex(i);
		sources << conf->value("url").toString();
	}
	conf->endArray();

	// Use alasky & data.stellarium.org if there are not values:
	if (sources.isEmpty())
		sources << "http://alaskybis.unistra.fr/hipslist"
		        << "https://data.stellarium.org/surveys/hipslist";

	for (QUrl source: sources)
	{
		if (source.scheme().isEmpty()) source.setScheme("file");
		QNetworkRequest req = QNetworkRequest(source);
		QNetworkReply* networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		connect(networkReply, &QNetworkReply::finished, [=] {
			QByteArray data = networkReply->readAll();
			QList<HipsSurveyP> newSurveys = HipsSurvey::parseHipslist(data);
			for (HipsSurveyP survey: newSurveys)
			{
				emit gotNewSurvey(survey);
			}
			surveys += newSurveys;
			emit surveysChanged();
		});
	}

	conf->endGroup();

	addAction("actionShow_Hips_Surveys", N_("Display Options"), N_("Toggle Hierarchical Progressive Surveys (experimental)"), "flagShow", "Ctrl+Alt+D");
}


void HipsMgr::restoreVisibleSurveys()
{
	//qDebug() << "HipsMgr::restoreVisibleSurveys()";
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("hips");

//	// restore visible state of last run, if possible
//	for (auto survey: surveys)
//	{
//		qDebug() << "Known Survey: " << survey->getUrl();
//	}
	conf->beginGroup("visible");
	for (QString key : conf->allKeys())
	{
		if (conf->value(key).toBool()==true)
		{
			// We have to restore one slash after colon.
			QString realURL=key.replace(":/", "://");
			qDebug() << "HIPS: Restore visible survey:" << realURL;
			HipsSurveyP survey=getSurveyByUrl(realURL);
			if (survey)
				survey->setVisible(true);
		}
	}
	conf->endGroup();
	conf->endGroup();
}

void HipsMgr::deinit()
{
}

void HipsMgr::draw(StelCore* core)
{
	if (!visible) return;
	StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
	for (auto survey: surveys)
	{
		if (survey->isVisible() && survey->planet.isEmpty())
		{
			survey->draw(&sPainter);
		}
	}
}

void HipsMgr::update(double deltaTime)
{
	for (auto survey: surveys)
	{
		survey->fader.update((int)(deltaTime * 1000));
	}
}

double HipsMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return 7;
	return 0;
}

HipsSurveyP HipsMgr::getSurveyByUrl(const QString &url)
{
	for (auto survey: surveys)
	{
		if (survey->getUrl() == url) return survey;
	}
	return HipsSurveyP(NULL);
}

bool HipsMgr::getFlagShow(void) const
{
	return visible;
}

void HipsMgr::setFlagShow(bool b)
{
	if (visible != b)
	{
		visible = b;
		emit showChanged(b);
	}
}
