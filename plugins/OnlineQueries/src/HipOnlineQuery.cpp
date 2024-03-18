/*
 * Copyright (C) 2020 Georg Zotti (based on F. Chereau's SimbadSearcher)
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

#include "HipOnlineQuery.hpp"

#include "StelTranslator.hpp"
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QDebug>


HipOnlineReply::HipOnlineReply(const QString& aurl, QNetworkAccessManager* anetMgr)
	: url(aurl)
	, netMgr(anetMgr)
	, currentStatus(HipQueryQuerying)
{
	reply = netMgr->get(QNetworkRequest(url));
	connect(reply, SIGNAL(finished()), this, SLOT(httpQueryFinished()));
}

HipOnlineReply::~HipOnlineReply()
{
	if (reply)
	{
		disconnect(reply, SIGNAL(finished()), this, SLOT(httpQueryFinished()));
		reply->abort();
		//do not use delete here
		reply->deleteLater();
		reply = nullptr;
	}
}

//This is provided for the correct deletion of the reply in the RemoteControl plugin
void HipOnlineReply::deleteNetworkReply()
{
	if(reply)
	{
		disconnect(reply, SIGNAL(finished()), this, SLOT(httpQueryFinished()));
		reply->abort();
		delete reply;
		reply = nullptr;
	}
}

void HipOnlineReply::httpQueryFinished()
{
	//qDebug() << "HipOnlineReply::httpQueryFinished()";
	if (reply->error()!=QNetworkReply::NoError || reply->bytesAvailable()==0)
	{
		qDebug() << "HipOnlineReply::httpQueryFinished() has errors:" << reply->errorString();
		currentStatus = HipQueryErrorOccured;
		errorString = QString("%1: %2").arg(q_("Network error"), reply->errorString());
		emit statusChanged();
		return;
	}

	// No error, process the Starnames result. If we receive HTML, just return it?
	//QByteArray line;
	//qDebug() << reply->readAll();
	QString replyString=reply->readAll();
	//qDebug() << replyString;
	htmlResult=replyString;

	currentStatus = HipQueryFinished;
	emit statusChanged();
}

// Get a I18n string describing the current status.
QString HipOnlineReply::getCurrentStatusString() const
{
	switch (currentStatus)
	{
		case HipQueryQuerying:
			return q_("Querying");
		case HipQueryErrorOccured:
			return q_("Error");
		case HipQueryFinished:
			return htmlResult.isEmpty() ? q_("Not found") : q_("Found");
	}
	return QString();
}

HipOnlineQuery::HipOnlineQuery(QString baseURL, QObject* parent) : QObject(parent), baseURL(baseURL)
{
	networkMgr = new QNetworkAccessManager(this);
}

// Lookup in external database for the passed HIP number.
HipOnlineReply* HipOnlineQuery::lookup(const int hipID)
{
	// Create the Star ID query
	QString url=QString(baseURL).arg(QString::number(hipID));
	//qDebug() << "looking up HIP " << hipID << "at "<< url;
	return new HipOnlineReply(url, networkMgr);
}

// Lookup in external database for the passed name.
HipOnlineReply* HipOnlineQuery::lookup(const QString name)
{
	// Create the Starnames query
	QString url=QString(baseURL).arg(name);
	//qDebug() << "looking up " << name << "at "<< url;
	return new HipOnlineReply(url, networkMgr);
}

// Lookup in external database for the passed HIP number.
HipOnlineReply* HipOnlineQuery::lookup(const QString url, const int hipID)
{
	baseURL=url;
	// Create the Star ID query
	QString queryUrl=QString(baseURL).arg(QString::number(hipID));
	//qDebug() << "looking up HIP " << hipID << "at "<< url;
	return new HipOnlineReply(queryUrl, networkMgr);
}

// Lookup in external database for the passed name.
HipOnlineReply* HipOnlineQuery::lookup(const QString url, const QString name)
{
	baseURL=url;
	// Create the Starnames query
	QString queryUrl=QString(baseURL).arg(name);
	//qDebug() << "looking up " << name << "at "<< url;
	return new HipOnlineReply(queryUrl, networkMgr);
}
