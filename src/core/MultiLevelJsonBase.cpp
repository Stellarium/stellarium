/*
 * Copyright (C) 2008 Fabien Chereau
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

#include "MultiLevelJsonBase.hpp"
#include "StelJsonParser.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelProjector.hpp"
#include "StelCore.hpp"
#include "kfilterdev.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QHttp>
#include <QUrl>
#include <QBuffer>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <stdexcept>

// Init statics
QNetworkAccessManager* MultiLevelJsonBase::networkAccessManager = NULL;

QNetworkAccessManager& MultiLevelJsonBase::getNetworkAccessManager()
{
	if (networkAccessManager==NULL)
	{
		networkAccessManager = new QNetworkAccessManager(&StelApp::getInstance());
		connect(networkAccessManager, SIGNAL(finished(QNetworkReply*)), &StelApp::getInstance(), SLOT(reportFileDownloadFinished(QNetworkReply*)));
	}
	return *networkAccessManager;
}

/*************************************************************************
  Class used to load a JSON file in a thread
 *************************************************************************/
class JsonLoadThread : public QThread
{
	public:
		JsonLoadThread(MultiLevelJsonBase* atile, QByteArray content, bool aqZcompressed=false, bool agzCompressed=false) : QThread((QObject*)atile),
			tile(atile), data(content), qZcompressed(aqZcompressed), gzCompressed(agzCompressed){;}
		virtual void run();
	private:
		MultiLevelJsonBase* tile;
		QByteArray data;
		const bool qZcompressed;
		const bool gzCompressed;
};

void JsonLoadThread::run()
{
	try
	{
		QBuffer buf(&data);
		buf.open(QIODevice::ReadOnly);
		tile->temporaryResultMap = MultiLevelJsonBase::loadFromJSON(buf, qZcompressed, gzCompressed);
	}
	catch (std::runtime_error e)
	{
		qWarning() << "WARNING : Can't parse loaded JSON description: " << e.what();
		tile->errorOccured = true;
	}
}

MultiLevelJsonBase::MultiLevelJsonBase(MultiLevelJsonBase* parent) : QObject(parent)
{
	errorOccured = false;
	httpReply = NULL;
	downloading = false;
	loadThread = NULL;
	loadingState = false;
	lastPercent = 0;
	// Avoid tiles to be deleted just after constructed
	timeWhenDeletionScheduled = -1.;
	deletionDelay = 2.;
	
	if (parent!=NULL)
	{
		deletionDelay = parent->deletionDelay;
	}
}

void MultiLevelJsonBase::initFromUrl(const QString& url)
{
	const MultiLevelJsonBase* parent = qobject_cast<MultiLevelJsonBase*>(QObject::parent());
	contructorUrl = url;
	if (!url.startsWith("http://") && (parent==NULL || !parent->getBaseUrl().startsWith("http://")))
	{
		// Assume a local file
		QString fileName;
		try
		{
			fileName = StelApp::getInstance().getFileMgr().findFile(url);
		}
		catch (std::runtime_error e)
		{
			try
			{
				if (parent==NULL)
					throw std::runtime_error("NULL parent");
				fileName = StelApp::getInstance().getFileMgr().findFile(parent->getBaseUrl()+url);
			}
			catch (std::runtime_error e)
			{
				qWarning() << "WARNING : Can't find JSON description: " << url << ": " << e.what();
				errorOccured = true;
				return;
			}
		}
		QFileInfo finf(fileName);
		baseUrl = finf.absolutePath()+'/';
		QFile f(fileName);
		f.open(QIODevice::ReadOnly);
		const bool compressed = fileName.endsWith(".qZ");
		const bool gzCompressed = fileName.endsWith(".gz");
		try
		{
			loadFromQVariantMap(loadFromJSON(f, compressed, gzCompressed));
		}
		catch (std::runtime_error e)
		{
			qWarning() << "WARNING : Can't parse JSON description: " << fileName << ": " << e.what();
			errorOccured = true;
			f.close();
			return;
		}
		f.close();
	}
	else
	{
		// Use a very short deletion delay to ensure that tile which are outside screen are discared before they are even downloaded
		// This is useful to reduce bandwidth when the user moves rapidely
		deletionDelay = 0.001;
		QUrl qurl;
		if (url.startsWith("http://"))
		{
			qurl.setUrl(url);
		}
		else
		{
			Q_ASSERT(parent->getBaseUrl().startsWith("http://"));
			qurl.setUrl(parent->getBaseUrl()+url);
		}
		Q_ASSERT(httpReply==NULL);
		httpReply = getNetworkAccessManager().get(QNetworkRequest(qurl));
		connect(httpReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		downloading = true;
		QString turl = qurl.toString();
		baseUrl = turl.left(turl.lastIndexOf('/')+1);
	}
}

// Constructor from a map used for JSON files with more than 1 level
void MultiLevelJsonBase::initFromQVariantMap(const QVariantMap& map)
{
	const MultiLevelJsonBase* parent = qobject_cast<MultiLevelJsonBase*>(QObject::parent());
	if (parent!=NULL)
	{
		baseUrl = parent->getBaseUrl();
		contructorUrl = parent->contructorUrl + "/?";
	}
	loadFromQVariantMap(map);
	downloading = false;
}
	
// Destructor
MultiLevelJsonBase::~MultiLevelJsonBase()
{
	if (httpReply)
	{
		httpReply->abort();
		delete httpReply;
		httpReply = NULL;
	}
	if (loadThread && loadThread->isRunning())
	{
		disconnect(loadThread, SIGNAL(finished()), this, SLOT(JsonLoadFinished()));
		// The thread is currently running, it needs to be properly stopped
		if (loadThread->wait(1)==false)
		{
			loadThread->terminate();
			//loadThread->wait(2000);
		}
	}
	foreach (MultiLevelJsonBase* tile, subTiles)
	{
		delete tile;
	}
}


// Schedule a deletion. It will practically occur after the delay passed as argument to deleteUnusedTiles() has expired
void MultiLevelJsonBase::scheduleDeletion()
{
	if (timeWhenDeletionScheduled<0.)
	{
		timeWhenDeletionScheduled = StelApp::getInstance().getTotalRunTime();
// 		foreach (MultiLevelJsonBase* tile, subTiles)
// 		{
// 			tile->scheduleDeletion();
// 		}
	}
}
	
// Load the tile information from a JSON file
QVariantMap MultiLevelJsonBase::loadFromJSON(QIODevice& input, bool qZcompressed, bool gzCompressed)
{
	StelJsonParser parser;
	QVariantMap map;
	if (qZcompressed && input.size()>0)
	{
		QByteArray ar = qUncompress(input.readAll());
		input.close();
		QBuffer buf(&ar);
		buf.open(QIODevice::ReadOnly);
		map = parser.parse(buf).toMap();
		buf.close();
	}
	else if (gzCompressed)
	{
		QIODevice* d = KFilterDev::device(&input, "application/x-gzip", false);
		d->open(QIODevice::ReadOnly);
		map = parser.parse(*d).toMap();
		d->close();
		delete d;
	}
	else
	{	
		map = parser.parse(input).toMap();
	}
	
	if (map.isEmpty())
		throw std::runtime_error("empty JSON file, cannot load");
	return map;
}

// Called when the download for the JSON file terminated
void MultiLevelJsonBase::downloadFinished()
{
	Q_ASSERT(downloading);
	if (httpReply->error()!=QNetworkReply::NoError)
	{
		if (httpReply->error()!=QNetworkReply::OperationCanceledError)
			qWarning() << "WARNING : Problem while downloading JSON description for " << httpReply->request().url().path() << ": "<< httpReply->errorString();
		errorOccured = true;
		httpReply->deleteLater();
		httpReply=NULL;
		downloading=false;
		return;
	}	
	
	QByteArray content = httpReply->readAll();
	if (content.isEmpty())
	{
		qWarning() << "WARNING : empty JSON description for " << httpReply->request().url().path();
		errorOccured = true;
		httpReply->deleteLater();
		httpReply=NULL;
		downloading=false;
		return;
	}
	
	const bool qZcompressed = httpReply->request().url().path().endsWith(".qZ");
	const bool gzCompressed = httpReply->request().url().path().endsWith(".gz");
	httpReply->deleteLater();
	httpReply=NULL;
	
	Q_ASSERT(loadThread==NULL);
	loadThread = new JsonLoadThread(this, content, qZcompressed, gzCompressed);
	connect(loadThread, SIGNAL(finished()), this, SLOT(JsonLoadFinished()));
	loadThread->start(QThread::LowestPriority);
}

// Called when the element is fully loaded from the JSON file
void MultiLevelJsonBase::JsonLoadFinished()
{
	loadThread->wait();
	delete loadThread;
	loadThread = NULL;
	downloading = false;
	if (errorOccured)
		return;
	loadFromQVariantMap(temporaryResultMap);
}


// Delete all the subtiles which were not displayed since more than lastDrawTrigger seconds
void MultiLevelJsonBase::deleteUnusedSubTiles()
{
	if (subTiles.isEmpty())
		return;
	
	const double now = StelApp::getInstance().getTotalRunTime();
	bool deleteAll = true;
	foreach (MultiLevelJsonBase* tile, subTiles)
	{
		if (tile->timeWhenDeletionScheduled<0 || (now-tile->timeWhenDeletionScheduled)<deletionDelay)
		{
			deleteAll = false;
			break;
		}
	}
	if (deleteAll==true)
	{
		//qDebug() << "Delete all tiles for " << this << ": " << contructorUrl;
		foreach (MultiLevelJsonBase* tile, subTiles)
			tile->deleteLater();
		subTiles.clear();
	}
	else
	{
		// Nothing to delete at this level, propagate
		foreach (MultiLevelJsonBase* tile, subTiles)
			tile->deleteUnusedSubTiles();
	}
}
	
void MultiLevelJsonBase::updatePercent(int tot, int toBeLoaded)
{
	if (tot+toBeLoaded==0)
	{
		if (loadingState==true)
		{
			loadingState=false;
			emit(loadingStateChanged(false));
		}
		return;
	}

	int p = (int)(100.f*tot/(tot+toBeLoaded));
	if (p>100)
		p=100;
	if (p<0)
		p=0;
	if (p==100 || p==0)
	{
		if (loadingState==true)
		{
			loadingState=false;
			emit(loadingStateChanged(false));
		}
		return;
	}
	else
	{
		if (loadingState==false)
		{
			loadingState=true;
			emit(loadingStateChanged(true));
		}
	}
	if (p==lastPercent)
		return;
	lastPercent=p;
	emit(percentLoadedChanged(p));
}
