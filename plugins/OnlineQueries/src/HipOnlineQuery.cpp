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

#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QDebug>
#include <QTimer>



HipOnlineReply::HipOnlineReply(const QString& aurl, QNetworkAccessManager* anetMgr, int delayMs)
	: url(aurl)
	, reply(Q_NULLPTR)
	, netMgr(anetMgr)
	, currentStatus(HipQueryQuerying)
{
	if(delayMs <= 0)
		delayTimerCompleted();
	else
	{
		// First wait before starting query. This is likely not necessary.
		QTimer::singleShot(delayMs, this, SLOT(delayTimerCompleted()));
	}
}

HipOnlineReply::~HipOnlineReply()
{
	if (reply)
	{
		disconnect(reply, SIGNAL(finished()), this, SLOT(httpQueryFinished()));
		reply->abort();
		//do not use delete here
		reply->deleteLater();
		reply = Q_NULLPTR;
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
		reply = Q_NULLPTR;
	}
}

void HipOnlineReply::delayTimerCompleted()
{
	reply = netMgr->get(QNetworkRequest(url));
	connect(reply, SIGNAL(finished()), this, SLOT(httpQueryFinished()));
}

void HipOnlineReply::httpQueryFinished()
{
	qDebug() << "StarnamesLookupReply::httpQueryFinished()";
	if (reply->error()!=QNetworkReply::NoError || reply->bytesAvailable()==0)
	{
		qDebug() << "Reply has errors:" << reply->errorString();
		currentStatus = HipQueryErrorOccured;
		errorString = QString("%1: %2").arg(q_("Network error")).arg(reply->errorString());
		emit statusChanged();
		return;
	}

	// No error, process the Starnames result. If we receive HTML, just return it?
	//QByteArray line;
	//qDebug() << reply->readAll();
	QString replyString=reply->readAll();
	qDebug() << replyString;
	htmlResult=replyString;

	currentStatus = HipQueryFinished;
	emit statusChanged();


/*
	// We have 2 kinds of answers...

	if (!reply->isSequential())
		reply->reset();
	bool found = false;

	while (!reply->atEnd())
	{
		line = reply->readLine();
		if (line.startsWith("::data"))
		{
			found = true;
			line = reply->readLine();	// Discard first header line
			break;
		}
	}
	if (found)
	{
		line = reply->readLine();
		line.chop(1); // Remove a line break at the end
		while (!line.isEmpty())
		{
			if (line=="No Coord.")
			{
				reply->readLine();
				line = reply->readLine();
				line.chop(1); // Remove a line break at the end
				continue;
			}
			QList<QByteArray> l = line.split(' ');
			if (l.size()!=2)
			{
				currentStatus = StarnamesLookupErrorOccured;
				errorString = q_("Error parsing position");
				emit statusChanged();
				return;
			}
			else
			{
				bool ok1, ok2;
				const double ra = l[0].toDouble(&ok1)*M_PI/180.;
				const double dec = l[1].toDouble(&ok2)*M_PI/180.;
				if (ok1==false || ok2==false)
				{
					currentStatus = StarnamesLookupErrorOccured;
					errorString = q_("Error parsing position");
					emit statusChanged();
					return;
				}
				Vec3d v;
				StelUtils::spheToRect(ra, dec, v);
				line = reply->readLine();
				line.chop(1); // Remove a line break at the end
				line.replace("NAME " ,"");
				resultPositions[line.simplified()]=v; // Remove an extra spaces
			}
			line = reply->readLine();
			line.chop(1); // Remove a line break at the end
		}
		currentStatus = StarnamesLookupFinished;
		emit statusChanged();
	}
*/
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

// Lookup in Starnames for the passed object name.
HipOnlineReply* HipOnlineQuery::lookup(const int hipID, const int delayMs)
{
	// Create the Starnames query
	QString url(baseURL);
	url.append(QString::number(hipID));
	qDebug() << "looking up HIP " << hipID << "at "<< url;
	return new HipOnlineReply(url, networkMgr, delayMs);
}
