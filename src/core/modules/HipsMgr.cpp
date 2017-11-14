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
#include "StelFader.hpp"
#include "StelPainter.hpp"
#include "StelCore.hpp"
#include "StelApp.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyLayerMgr.hpp"

#include <QNetworkReply>
#include <QSettings>

HipsMgr::HipsMgr() : survey(Q_NULLPTR)
{
	setObjectName("HipsMgr");
	fader = new LinearFader();
}

HipsMgr::~HipsMgr()
{
	delete fader;
	fader = Q_NULLPTR;
}

void HipsMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("hips");

	addAction("actionShow_Hips_Survey", N_("Display Options"), N_("Digitized Sky Survey (experimental)"), "surveyDisplayed", "Ctrl+Alt+F");

	// Start to load survey lists
	QStringList sources;
	int size = conf->beginReadArray("sources");
	for (int i = 0; i < size; i++)
	{
		conf->setArrayIndex(i);
		sources << conf->value("url").toString();
	}
	conf->endArray();

	// Use alasky if there are not values:
	if (sources.isEmpty())
		sources << "http://alaskybis.unistra.fr/hipslist";


	for (QUrl source: sources)
	{
		if (source.scheme().isEmpty()) source.setScheme("file");
		QNetworkRequest req = QNetworkRequest(source);
		QNetworkReply* networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		connect(networkReply, &QNetworkReply::finished, [=] {
			QByteArray data = networkReply->readAll();
			surveys += HipsSurvey::parseHipslist(data);
			emit surveysChanged();
		});
	}

	setSurveyUrl("http://alaskybis.unistra.fr/DSS/DSSColor");
	conf->endGroup();
}

void HipsMgr::deinit()
{
	survey = HipsSurveyP(NULL);
}

void HipsMgr::draw(StelCore* core)
{
	if (!getFlagSurveyShow())
		return;

	StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
	survey->draw(&sPainter);
}

void HipsMgr::update(double deltaTime)
{
	fader->update((int)(deltaTime*1000));
}

double HipsMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return 7;
	return 0;
}

void HipsMgr::setFlagSurveyShow(const bool displayed)
{
	if (*fader != displayed)
	{
		*fader = displayed;
		GETSTELMODULE(StelSkyLayerMgr)->setFlagShow(!displayed);
		emit surveyDisplayedChanged(displayed);
	}
}

bool HipsMgr::getFlagSurveyShow() const
{
	return *fader;
}

QString HipsMgr::getSurveyUrl() const
{
	return survey ? survey->getUrl() : QString();
}

void HipsMgr::setSurveyUrl(const QString& url)
{
	if (survey && url == survey->getUrl()) return;
	survey.clear();
	for (auto hips: surveys)
	{
		if (hips->getUrl() == url)
		{
			survey = hips;
			break;
		}
	}
	/*
	survey = HipsSurveyP(new HipsSurvey(url));
	survey->setParent(this);
	*/
}
