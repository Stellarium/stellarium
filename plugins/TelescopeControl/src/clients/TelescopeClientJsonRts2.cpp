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

#include "QByteArray"
#include "QJsonDocument"
#include "QJsonParseError"
#include "QJsonObject"
#include "QUrlQuery"

#include "TelescopeClientJsonRts2.hpp"

TelescopeClientJsonRts2::TelescopeClientJsonRts2(const QString &name, const QString &params, Equinox eq)
	: TelescopeClient(name)
	, networkManager(new QNetworkAccessManager)
	, equinox(eq)
	, port(0)
	, address("")
	, baseurl("http://localhost:8889/")
{
	// Example params:
	// localhost:10000:petr:test
	// split into:
	// host       = localhost
	// port       = 10000 (int)
	// login      = petr
	// password   = test

	QRegExp paramRx("^([^:]*):(\\d+):([^:]*):([^:]*)$");
	QString host;
	QString login;
	QString password;

	if (paramRx.exactMatch(params))
	{
		host         = paramRx.capturedTexts().at(1).trimmed();
		port         = paramRx.capturedTexts().at(2).toInt();
		login        = paramRx.capturedTexts().at(3).trimmed();
		password     = paramRx.capturedTexts().at(4).trimmed();
	}
	else
	{
		qWarning() << "WARNING - incorrect TelescopeRTS2 parameters";
		return;
	}

	qDebug() << "TelescopeRTS2 paramaters host, port, login, password:" << host << port << login << password;

	if (port <= 0 || port > 0xffff)
	{
		qWarning() << "ERROR creating TelescopeRTS2 - port not valid (should be less than 32767)";
		return;
	}

	baseurl.setHost(host);
	baseurl.setPort(port);
	baseurl.setUserName(login);
	baseurl.setPassword(password);

	QUrl rurl(baseurl);

	rurl.setPath("/api/get");

	QUrlQuery query;
	query.addQueryItem("d", "T0");
	rurl.setQuery(query);

	request.setUrl(rurl);

	networkManager.get(request);

	connect(&networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
}

TelescopeClientJsonRts2::~TelescopeClientJsonRts2(void)
{
}

void TelescopeClientJsonRts2::replyFinished(QNetworkReply *reply)
{
	qDebug() << "TelescopeRTS2 error" << reply->error();
	if (reply->error() != QNetworkReply::NoError)
		return;

	QByteArray data = reply->readAll();
	qDebug() << "TelescopeRTS2 data" << (QString) data;

	QJsonDocument doc;
	QJsonParseError jsonError;
	doc = QJsonDocument::fromJson(data, &jsonError);

	QJsonObject docObject = doc.object();
	QJsonObject dObject = docObject["d"].toObject();
	QJsonObject telObject = dObject["TEL"].toObject();

	qDebug() << "TelescopeRTS2 object" << doc.isNull() << jsonError.errorString() << jsonError.offset << telObject["ra"].toDouble() << telObject["dec"].toDouble();

	const double ra = telObject["ra"].toDouble() * M_PI / 180.0;
	const double dec = telObject["dec"].toDouble() * M_PI / 180.0;
	const double cdec = cos(dec);
	
	qDebug() << "TelescopeRTS2 RADEC" << ra << dec;

	Vec3d pos(cos(ra)*cdec, sin(ra)*cdec, sin(dec));
	position = pos;
	
	networkManager.get(request);
}

bool TelescopeClientJsonRts2::isConnected(void) const
{
	return true;
}

Vec3d TelescopeClientJsonRts2::getJ2000EquatorialPos(const StelCore* core) const
{
	return position;
}

void TelescopeClientJsonRts2::telescopeGoto(const Vec3d &j2000Pos)
{
	if (!isConnected())
		return;

	const double ra_signed = atan2(j2000Pos[1], j2000Pos[0]);
	const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2 * M_PI);
	const double dec = atan2(j2000Pos[2], std::sqrt(j2000Pos[0]*j2000Pos[0]+j2000Pos[1]*j2000Pos[1]));

	QUrl set(baseurl);
	set.setPath("/api/cmd");

	QUrlQuery query;
	query.addQueryItem("d", "T0");
	query.addQueryItem("c", QString("move+%1+%2").arg(ra * 180 / M_PI).arg(dec * 180 / M_PI));
	set.setQuery(query);

	QNetworkRequest setR;
	setR.setUrl(set);

	networkManager.get(setR);
}

bool TelescopeClientJsonRts2::hasKnownPosition(void) const
{
	return true;
}
