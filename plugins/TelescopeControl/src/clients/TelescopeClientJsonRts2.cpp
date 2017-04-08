/*
 * RTS2 Json stellarium plugin
 * 
 * Copyright (C) 2014-2016 Petr Kubanek
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

#include "QByteArray"
#include "QJsonArray"
#include "QJsonDocument"
#include "QJsonParseError"
#include "QJsonObject"
#include "QUrlQuery"

#include "TelescopeClientJsonRts2.hpp"

TelescopeClientJsonRts2::TelescopeClientJsonRts2(const QString &name, const QString &params, Equinox eq)
	: TelescopeClient(name)
	, networkManager(new QNetworkAccessManager)
	, equinox(eq)
	, baseurl("http://localhost:8889/")
	, telName("")
	, time_delay(500)
{
	// Example params:
	// petr:test@localhost:8889/tel

	qDebug() << "TelescopeRTS2 paramaters: " << params;

	baseurl.setUrl(params);
	if (!baseurl.isValid())
	{
		qWarning() << "TelescopeRTS2 invalid URL";
		return;
	}

	QUrl rurl(baseurl);

	rurl.setPath(baseurl.path() + "/api/devbytype");

	QUrlQuery query;
	query.addQueryItem("t", "2");
	rurl.setQuery(query);

	QNetworkRequest cfgRequest;

	cfgRequest.setUrl(rurl);

	qDebug() << "request url:" << rurl.toString();

	connect(&networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

	networkManager.get(cfgRequest);
}

TelescopeClientJsonRts2::~TelescopeClientJsonRts2(void)
{
}

void TelescopeClientJsonRts2::replyFinished(QNetworkReply *reply)
{
	qDebug() << "TelescopeRTS2 error" << reply->error();
	if (reply->error() != QNetworkReply::NoError)
	{
		telName = "";
		return;
	}

	QByteArray data = reply->readAll();
	//qDebug() << "TelescopeRTS2 data" << (QString) data;

	QJsonDocument doc;
	QJsonParseError jsonError;
	doc = QJsonDocument::fromJson(data, &jsonError);

	if (reply->url().path().endsWith("/api/devbytype"))
	{
		QJsonArray arr = doc.array();
		telName = arr[0].toString();

		QUrl rurl(baseurl);

		rurl.setPath(baseurl.path() + "/api/get");

		QUrlQuery query;
		query.addQueryItem("d", telName);
		rurl.setQuery(query);

		request.setUrl(rurl);

		qDebug() << "request url:" << rurl.toString();

		networkManager.get(request);
	}
	else if (reply->url().path().endsWith("/api/get"))
	{
		QJsonObject docObject = doc.object();
		QJsonObject dObject = docObject["d"].toObject();
		QJsonObject telObject = dObject["TEL"].toObject();

		qDebug() << "TelescopeRTS2 object" << doc.isNull() << jsonError.errorString() << jsonError.offset << telObject["ra"].toDouble() << telObject["dec"].toDouble();

		const double ra = telObject["ra"].toDouble() * M_PI / 180.0;
		const double dec = telObject["dec"].toDouble() * M_PI / 180.0;
		const double cdec = cos(dec);
	
		qDebug() << "TelescopeRTS2 RADEC" << ra << dec;

		Vec3d pos(cos(ra)*cdec, sin(ra)*cdec, sin(dec));
		interpolatedPosition.add(pos, getNow(), getNow(), 0);

		qDebug() << "request url:" << request.url().toString();
	
		networkManager.get(request);
	}
	else
	{
		qWarning() << "unhandled reply: " << reply->url().toString();
	}
}

bool TelescopeClientJsonRts2::isConnected(void) const
{
	return telName.isEmpty() == false;
}

Vec3d TelescopeClientJsonRts2::getJ2000EquatorialPos(const StelCore* core) const
{
	Q_UNUSED(core);
	const qint64 now = getNow() - time_delay;
	return interpolatedPosition.get(now);
}

void TelescopeClientJsonRts2::telescopeGoto(const Vec3d &j2000Pos)
{
	if (!isConnected())
		return;

	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, j2000Pos);

	QUrl set(baseurl);
	set.setPath(baseurl.path() + "/api/cmd");

	QUrlQuery query;
	query.addQueryItem("d", telName);
	query.addQueryItem("c", QString("move+%1+%2").arg(ra * 180 / M_PI).arg(dec * 180 / M_PI));
	set.setQuery(query);

	QNetworkRequest setR;
	setR.setUrl(set);

	qDebug() << "GoTo request: " << set.toString();

	networkManager.get(setR);
}

bool TelescopeClientJsonRts2::hasKnownPosition(void) const
{
	return interpolatedPosition.isKnown();
}
