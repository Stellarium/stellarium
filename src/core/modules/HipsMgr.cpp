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
#include "StelUtils.hpp"

#include <QNetworkReply>
#include <QSettings>
#include <QTimer>

HipsMgr::HipsMgr()
{
	setObjectName("HipsMgr");
	connect(this, SIGNAL(surveysChanged()), this, SLOT(restoreVisibleSurveys()));
}

HipsMgr::~HipsMgr()
{
	if (StelApp::getInstance().getNetworkAccessManager()->networkAccessible()==QNetworkAccessManager::Accessible)
	{
		// Store active HiPS to config.ini if network is available
		QSettings* conf = StelApp::getInstance().getSettings();
		conf->beginGroup("hips");
		conf->setValue("show", getFlagShow());

		// remove 0.18.0 scheme
		conf->remove("visible");

		QStringList surveyUrls;
		for (auto survey: surveys)
		{
			if (survey->isVisible() && survey->planet.isEmpty())
				surveyUrls << survey->getUrl();
		}

		int surveyListSize = surveyUrls.count();
		if (surveyListSize==0)
		{
			// remove old urls
			conf->remove("surveys");
		}
		else
		{
			conf->beginWriteArray("surveys");
			for(int i=0;i<surveyListSize;i++)
			{
				conf->setArrayIndex(i);
				conf->setValue("url", surveyUrls.at(i));
			}
			conf->endArray();
		}

		conf->endGroup();
		conf->sync();
	}
}

void HipsMgr::loadSources()
{
	if (state != Created)
		return; // Already loaded.

	state = Loading;
	emit stateChanged(state);
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("hips");

	// Start to load survey lists
	QStringList sources;
	int size = conf->beginReadArray("sources");
	for (int i = 0; i < size; i++)
	{
		conf->setArrayIndex(i);
		sources << conf->value("url").toString();
	}
	conf->endArray();

	// Use alasky (all pixelate surveys from MocServer) & data.stellarium.org if there are not values:
	if (sources.isEmpty())
	{
		sources << "http://alasky.u-strasbg.fr/MocServer/query?*/P/*&get=record"
			<< "https://data.stellarium.org/surveys/hipslist";
	}

	for (QUrl source: sources)
	{
		if (source.scheme().isEmpty()) source.setScheme("file");
		QNetworkRequest req = QNetworkRequest(source);
		req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
		QNetworkReply* networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		connect(networkReply, &QNetworkReply::finished, [=] {
			QByteArray data = networkReply->readAll();
			QList<HipsSurveyP> newSurveys = HipsSurvey::parseHipslist(data);
			for (HipsSurveyP survey: newSurveys)
			{
				connect(survey.data(), SIGNAL(propertiesChanged()), this, SIGNAL(surveysChanged()));
				emit gotNewSurvey(survey);
			}
			surveys += newSurveys;
			emit surveysChanged();
			nbSourcesLoaded++;
			if (nbSourcesLoaded == sources.size())
			{
				state = Loaded;
				emit stateChanged(state);
			}
		});
	}
	conf->endGroup();
}

void HipsMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("hips");
	setFlagShow(conf->value("show", false).toBool());
	int size = conf->beginReadArray("surveys");
	conf->endArray();	
	conf->endGroup();
	bool hasVisibleSurvey = size>0 ? true: false;

	if (StelApp::getInstance().getNetworkAccessManager()->networkAccessible()==QNetworkAccessManager::NotAccessible)
	{
		setFlagShow(false);
		hasVisibleSurvey = false;
	}

	addAction("actionShow_Hips_Surveys", N_("Display Options"), N_("Toggle Hierarchical Progressive Surveys"), "flagShow", "Ctrl+Alt+D");

	// Start loading the sources only after stellarium has time to set up the proxy.
	// We only do it if we actually have a visible survey.  Otherwise it's
	// better not to do it until the user open the survey ui so that we
	// don't make a systematic request to the hipslist files!
	if (hasVisibleSurvey)
		QTimer::singleShot(0, this, SLOT(loadSources()));
}


void HipsMgr::restoreVisibleSurveys()
{
	//qDebug() << "HipsMgr::restoreVisibleSurveys()";
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("hips");

	// Start to load survey lists
	int size = conf->beginReadArray("surveys");
	for (int i = 0; i < size; i++)
	{
		conf->setArrayIndex(i);
		QString url = conf->value("url").toString();
		if (!url.isEmpty())
		{
			// qDebug() << "HiPS: Restore visible survey:" << url;
			HipsSurveyP survey=getSurveyByUrl(url);
			if (survey)
				survey->setVisible(true);
		}
	}
	conf->endArray();
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
		survey->fader.update(static_cast<int>(deltaTime * 1000));
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
