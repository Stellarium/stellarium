/*
 * RTS2 JSON stellarium plugin
 * 
 * Copyright (C) 2014-2017 Petr Kubanek
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

#include "StelUtils.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

#include "QByteArray"
#include "QJsonArray"
#include "QJsonDocument"
#include "QJsonParseError"
#include "QJsonObject"
#include "QUrlQuery"
#include "QTimer"
#include "QTimerEvent"

#include "TelescopeClientJsonRts2.hpp"

TelescopeClientJsonRts2::TelescopeClientJsonRts2(const QString &name, const QString &params, Equinox eq)
	: TelescopeClient(name)
	, networkManager(new QNetworkAccessManager)
	, equinox(eq)
	, baseurl("http://localhost:8889/")
	, telName("")
	, telReadonly(false)
	, telLatitude(static_cast<double>(NAN))
	, telLongitude(static_cast<double>(NAN))
	, telAltitude(static_cast<double>(NAN))
	, telTargetDist(static_cast<double>(NAN))
	, time_delay(50)
	, reconnectTimer(-1)
	, refresh_delay(500)
	, server_micros(0)
{
	telescopeManager = GETSTELMODULE(TelescopeControl);

	// Example params:
	// 1000:test:1234@localhost:8889/tel

	QRegExp paramRx("^(\\d+):(.*)$");
	QString url;
	if (paramRx.exactMatch(params))
	{
		refresh_delay = paramRx.cap(1).toInt() / 1000; // convert microseconds to milliseconds
		url           = paramRx.cap(2).trimmed();
	}
	else
	{
		qWarning() << "ERROR creating TelescopeClientJsonRts2: invalid parameters.";
		return;
	}

	qDebug() << "TelescopeRTS2(" << name << ") URL, refresh timeout: " << url << "," << refresh_delay;

	baseurl.setUrl(url);
	if (!baseurl.isValid())
	{
		qWarning() << "TelescopeRTS2(" << name << ") invalid URL: " << url;
		return;
	}

	QUrl rurl(baseurl);

	rurl.setPath(baseurl.path() + "/api/devbytype");

	QUrlQuery query;
	query.addQueryItem("t", "2");
	rurl.setQuery(query);

	cfgRequest.setUrl(rurl);

	qDebug() << "TelescopeRTS2(" << name << ")::TelescopeRTS2: request url:" << rurl.toString();

	connect(&networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

	networkManager.get(cfgRequest);
}

TelescopeClientJsonRts2::~TelescopeClientJsonRts2(void)
{
}

void TelescopeClientJsonRts2::refreshTimer()
{
	server_micros = getNow();
	networkManager.get(request);
}

void TelescopeClientJsonRts2::replyFinished(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NoError)
	{
		if (reply->url().path().endsWith("/api/cmd") && reply->error() == QNetworkReply::ProtocolInvalidOperationError)
		{
			setReadOnly(true);
			return;
		}
		qDebug() << "TelescopeRTS2(" << name << ")::replyFinished: error " << reply->error() << " url: " << reply->url().toString();
		telName = "";
		if (reconnectTimer < 0)
			reconnectTimer = startTimer(15000);

		return;
	}

	if (reconnectTimer > 0)
	{
		killTimer(reconnectTimer);
		reconnectTimer = -1;
	}

	QByteArray data = reply->readAll();
	//qDebug() << "TelescopeRTS2(" << name << ")::replyFinished: data " << (QString) data;

	QJsonDocument doc;
	QJsonParseError jsonError;
	doc = QJsonDocument::fromJson(data, &jsonError);

	if (reply->url().path().endsWith("/api/devbytype"))
	{
		QJsonArray arr = doc.array();
		telName = arr[0].toString();

		getReadOnly();
	}
	else if (reply->url().path().endsWith("/api/deviceinfo"))
	{
		QJsonObject docObject = doc.object();
		setReadOnly(docObject["readonly"].toBool());

		QUrl rurl(baseurl);

		rurl.setPath(baseurl.path() + "/api/get");

		QUrlQuery query;
		query.addQueryItem("d", telName);
		rurl.setQuery(query);

		request.setUrl(rurl);

		qDebug() << "TelescopeRTS2(" << name << ")::replyFinished: request url:" << rurl.toString();

		refreshTimer();
	}
	else if (reply->url().path().endsWith("/api/get"))
	{
		QJsonObject docObject = doc.object();
		QJsonObject dObject = docObject["d"].toObject();
		QJsonObject telObject = dObject["TEL"].toObject();

		telLongitude = dObject["LONGITUD"].toDouble() * M_PI / 180.0;
		telLatitude = dObject["LATITUDE"].toDouble() * M_PI / 180.0;
		telAltitude = dObject["ALTITUDE"].toDouble();
		telTargetDist = dObject["target_distance"].toDouble() * M_PI / 180.0;

		const double ra = telObject["ra"].toDouble() * M_PI / 180.0;
		const double dec = telObject["dec"].toDouble() * M_PI / 180.0;
		const double cdec = cos(dec);
	
		qDebug() << "TelescopeRTS2(" << name << ")::replyFinished: RADEC" << ra << dec;

		lastPos.set(cos(ra)*cdec, sin(ra)*cdec, sin(dec));
		interpolatedPosition.add(lastPos, getNow(), server_micros, 0);

		QTimer::singleShot(refresh_delay, this, SLOT(refreshTimer()));
	}
	else if (reply->url().path().endsWith("/api/cmd"))
	{
		QJsonObject docObject = doc.object();
		int cmdRet = docObject["ret"].toInt();
		qDebug() << "Move command finished: " << cmdRet;
		if (cmdRet == 0)
			getReadOnly();
		else
			setReadOnly(true);
	}
	else
	{
		qWarning() << "TelescopeRTS2(" << name << ")::replyFinished: unhandled reply: " << reply->url().toString();
	}
	reply->deleteLater();
}

bool TelescopeClientJsonRts2::isConnected(void) const
{
	return telName.isEmpty() == false;
}

Vec3d TelescopeClientJsonRts2::getJ2000EquatorialPos(const StelCore*) const
{
	const qint64 now = getNow() - time_delay;
	return interpolatedPosition.get(now);
}

void TelescopeClientJsonRts2::telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject)
{
	if (!isConnected())
		return;

	QUrl set(baseurl);
	set.setPath(baseurl.path() + "/api/cmd");

	QUrlQuery query;
	query.addQueryItem("d", telName);

	bool commanded = false;

	// if it's satellite, use move_tle
	if (selectObject)
	{
		QVariantMap objectMap = selectObject->getInfoMap(StelApp::getInstance().getCore());
		if (objectMap.contains("tle1") && objectMap.contains("tle2"))
		{
			query.addQueryItem("c", QString("move_tle+%22") + objectMap["tle1"].toString() + QString("%22+%22") + objectMap["tle2"].toString() + QString("%22"));
			commanded = true;
		}
	}

	if (commanded == false)
	{
		double ra, dec;
		StelUtils::rectToSphe(&ra, &dec, j2000Pos);

		query.addQueryItem("c", QString("move+%1+%2").arg(ra * 180 / M_PI).arg(dec * 180 / M_PI));
	}
	set.setQuery(query);

	QNetworkRequest setR;
	setR.setUrl(set);

	qDebug() << "TelescopeRTS2(" << name << ")::telescopeGoto: request: " << set.toString();

	networkManager.get(setR);
}

void TelescopeClientJsonRts2::telescopeSync(const Vec3d &j2000Pos, StelObjectP selectObject)
{
	Q_UNUSED(j2000Pos)
	Q_UNUSED(selectObject)
	if (!isConnected())
		return;

	return;
}

bool TelescopeClientJsonRts2::hasKnownPosition(void) const
{
	return interpolatedPosition.isKnown();
}

void TelescopeClientJsonRts2::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == reconnectTimer)
	{
		qDebug() << "Telescope reconnect";
		networkManager.get(cfgRequest);
	}
}

QString TelescopeClientJsonRts2::getTelescopeInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	Q_UNUSED(core);
	Q_UNUSED(flags);
	QString str;
	QTextStream oss(&str);
	
	if (telReadonly)
	{
		oss << q_("Read-only telescope") << "<br /";
	}

	oss << q_("Telescope position: ") << QString("%1 %2 %3m").arg(StelUtils::radToDmsStr(telLongitude,true), StelUtils::radToDmsStr(telLatitude,true)).arg(telAltitude) << "<br />";

	oss << q_("Distance to target position: ") << StelUtils::radToDmsStr(telTargetDist,true) << "<br />";

	return str;
}

void TelescopeClientJsonRts2::getReadOnly()
{
	QUrl diurl(baseurl);

	diurl.setPath(baseurl.path() + "/api/deviceinfo");

	QUrlQuery query;
	query.addQueryItem("d", telName);
	diurl.setQuery(query);

	request.setUrl(diurl);

	qDebug() << "TelescopeRTS2(" << name << ")::replyFinished: request url:" << diurl.toString();

	networkManager.get(request);
}

void TelescopeClientJsonRts2::setReadOnly(bool readonly)
{
	telReadonly = readonly;
	QSettings* settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);

	if (telescopeManager)
	{
		if (telReadonly)
		{
			telescopeManager->setReticleColor(Vec3f(settings->value("color_telescope_readonly", "1,0,0").toString()));
		}
		else
		{
			telescopeManager->setReticleColor(Vec3f(settings->value("color_telescope_reticles", "0.6,0.4,0").toString()));
		}
	}
}
