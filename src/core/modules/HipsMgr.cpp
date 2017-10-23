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
	addAction("actionShow_Hips_Survey", N_("Display Options"), N_("Digitized Sky Survey (experimental)"), "surveyDisplayed", "Ctrl+Alt+F");

	// Start to load survey list:
	QString listPath = "http://alaskybis.unistra.fr/hipslist";
	QNetworkRequest req = QNetworkRequest(listPath);
	QNetworkReply* networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(networkReply, &QNetworkReply::finished, [=] {
		QByteArray data = networkReply->readAll();
		QString url;
		foreach(QString line, data.split('\n'))
		{
			if (line.startsWith('#')) continue;
			QString key = line.section("=", 0, 0).trimmed();
			QString value = line.section("=", 1, -1).trimmed();
			if (key == "hips_service_url") url = value;
			if (key == "hips_status" && value.split(' ').contains("public")) {
				addHips(url);
			}
		}
		emit surveyListChanged();
	});

	setSurveyUrl("http://alaskybis.unistra.fr/DSS/DSSColor");
}

void HipsMgr::deinit()
{
	delete survey;
	survey = Q_NULLPTR;
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

	delete survey;
	survey = new HipsSurvey(url);
	survey->setParent(this);
}

void HipsMgr::addHips(const QString& url)
{
	QNetworkRequest req = QNetworkRequest(url + "/properties");
	QNetworkReply* networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);

	connect(networkReply, &QNetworkReply::finished, [&, networkReply] {
		QByteArray data = networkReply->readAll();
		QJsonObject properties;
		foreach(QString line, data.split('\n'))
		{
			if (line.startsWith("#")) continue;
			QString key = line.section("=", 0, 0).trimmed();
			if (key.isEmpty()) continue;
			QString value = line.section("=", 1, -1).trimmed();
			properties[key] = value;
		}
		this->surveyList.append(properties);
		emit surveyListChanged();

		networkReply->deleteLater();
	});
}
