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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "MultiLevelJsonBase.hpp"
#include "StelJsonParser.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelProjector.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDir>
#include <QBuffer>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <stdexcept>
#include <cstdio>

// #include <QNetworkDiskCache>

// Init statics
QNetworkAccessManager* MultiLevelJsonBase::networkAccessManager = Q_NULLPTR;

QNetworkAccessManager& MultiLevelJsonBase::getNetworkAccessManager()
{
	if (networkAccessManager==Q_NULLPTR)
	{
		networkAccessManager = new QNetworkAccessManager(&StelApp::getInstance());
// Cache on JSON files doesn't work, and I don't know why.
// There may be a problem in Qt for text objects caching (compression is activated in cache)
// 		QNetworkDiskCache* cache = new QNetworkDiskCache(networkAccessManager);
// 		QString cachePath = StelApp::getInstance().getCacheDir();
// 		cache->setCacheDirectory(cachePath+"/JSONCache");
// 		networkAccessManager->setCache(cache);
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
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING : Can't parse loaded JSON description: " << e.what();
		tile->errorOccured = true;
	}
}

MultiLevelJsonBase::MultiLevelJsonBase(MultiLevelJsonBase* parent) : StelSkyLayer(parent)
	, errorOccured(false)
	, downloading(false)
	, httpReply(Q_NULLPTR)
	, deletionDelay(2.)
	, loadThread(Q_NULLPTR)
	, timeWhenDeletionScheduled(-1.) // Avoid tiles to be deleted just after constructed
	, loadingState(false)
	, lastPercent(0)
{
	if (parent!=Q_NULLPTR)
	{
		deletionDelay = parent->deletionDelay;
	}
}

void MultiLevelJsonBase::initFromUrl(const QString& url)
{
	const MultiLevelJsonBase* parent = qobject_cast<MultiLevelJsonBase*>(QObject::parent());
	contructorUrl = url;
	if (!url.startsWith("http", Qt::CaseInsensitive) && (parent==Q_NULLPTR || !parent->getBaseUrl().startsWith("http", Qt::CaseInsensitive)))
	{
		// Assume a local file
		QString fileName = StelFileMgr::findFile(url);
		if (fileName.isEmpty())
		{
			if (parent==Q_NULLPTR)
			{
				qWarning() << "NULL parent";
				errorOccured = true;
				return;
			}
			fileName = StelFileMgr::findFile(parent->getBaseUrl()+url);
			if (fileName.isEmpty())
			{
				qWarning() << "WARNING : Can't find JSON description: " << url;
				errorOccured = true;
				return;
			}
		}
		QFileInfo finf(fileName);
		baseUrl = finf.absolutePath()+'/';
		QFile f(fileName);
		if(f.open(QIODevice::ReadOnly))
		{
			const bool compressed = fileName.endsWith(".qZ");
			const bool gzCompressed = fileName.endsWith(".gz");
			try
			{
				loadFromQVariantMap(loadFromJSON(f, compressed, gzCompressed));
			}
			catch (std::runtime_error& e)
			{
				qWarning() << "WARNING: Can't parse JSON document: " << QDir::toNativeSeparators(fileName) << ":" << e.what();
				errorOccured = true;
				f.close();
				return;
			}
			f.close();
		}
	}
	else
	{
		// Use a very short deletion delay to ensure that tile which are outside screen are discared before they are even downloaded
		// This is useful to reduce bandwidth when the user moves rapidely
		deletionDelay = 0.001;
		QUrl qurl;
		if (url.startsWith("http", Qt::CaseInsensitive))
		{
			qurl.setUrl(url);
		}
		else
		{
			Q_ASSERT(parent->getBaseUrl().startsWith("http", Qt::CaseInsensitive));
			qurl.setUrl(parent->getBaseUrl()+url);
		}
		Q_ASSERT(httpReply==Q_NULLPTR);
		QNetworkRequest req(qurl);
		req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
		httpReply = getNetworkAccessManager().get(req);
		//qDebug() << "Started downloading " << httpReply->request().url().path();
		Q_ASSERT(httpReply->error()==QNetworkReply::NoError);
		//qDebug() << httpReply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
		connect(httpReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		//connect(httpReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
		//connect(httpReply, SIGNAL(destroyed()), this, SLOT(replyDestroyed()));
		downloading = true;
		QString turl = qurl.toString();
		baseUrl = turl.left(turl.lastIndexOf('/')+1);
	}
}

// Constructor from a map used for JSON files with more than 1 level
void MultiLevelJsonBase::initFromQVariantMap(const QVariantMap& map)
{
	const MultiLevelJsonBase* parent = qobject_cast<MultiLevelJsonBase*>(QObject::parent());
	if (parent!=Q_NULLPTR)
	{
		baseUrl = parent->getBaseUrl();
		contructorUrl = parent->contructorUrl + "/?";
	}
	try
	{
		loadFromQVariantMap(map);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: invalid variant map: " << e.what();
		errorOccured = true;
		return;
	}
	downloading = false;
}

// Destructor
MultiLevelJsonBase::~MultiLevelJsonBase()
{
	if (httpReply)
	{
		//qDebug() << "Abort: " << httpReply->request().url().path();
		//httpReply->abort();

		// TODO: This line should not be commented, but I have to keep it because of a Qt bug.
		// It should be fixed with Qt 4.5.1
		// It causes a nasty memory leak, but prevents an even more nasty
		//httpReply->deleteLater();
		httpReply = Q_NULLPTR;
	}
	if (loadThread && loadThread->isRunning())
	{
		//qDebug() << "--> Abort thread " << contructorUrl;
		disconnect(loadThread, SIGNAL(finished()), this, SLOT(jsonLoadFinished()));
		// The thread is currently running, it needs to be properly stopped
		if (loadThread->wait(1)==false)
		{
			loadThread->terminate();
			//loadThread->wait(2000);
		}
	}
	for (auto* tile : subTiles)
	{
		tile->deleteLater();
	}
	subTiles.clear();
}


void MultiLevelJsonBase::scheduleChildsDeletion()
{
	for (auto* tile : subTiles)
	{
		if (tile->timeWhenDeletionScheduled<0)
			tile->timeWhenDeletionScheduled = StelApp::getInstance().getTotalRunTime();
	}
}

// If a deletion was scheduled, cancel it.
void MultiLevelJsonBase::cancelDeletion()
{
	timeWhenDeletionScheduled=-1.;
	for (auto* tile : subTiles)
	{
		tile->cancelDeletion();
	}
}

// Load the tile information from a JSON file
QVariantMap MultiLevelJsonBase::loadFromJSON(QIODevice& input, bool qZcompressed, bool gzCompressed)
{
	QVariantMap map;
	if (qZcompressed && input.size()>0)
	{
		QByteArray ar = qUncompress(input.readAll());
		input.close();
		QBuffer buf(&ar);
		buf.open(QIODevice::ReadOnly);
		map = StelJsonParser::parse(&buf).toMap();
		buf.close();
	}
	else if (gzCompressed)
	{
		QByteArray ar = StelUtils::uncompress(input.readAll());
		input.close();
		map = StelJsonParser::parse(ar).toMap();
	}
	else
	{
		map = StelJsonParser::parse(&input).toMap();
	}

	if (map.isEmpty())
		throw std::runtime_error("empty JSON file, cannot load");
	return map;
}


// Called when the download for the JSON file terminated
void MultiLevelJsonBase::downloadFinished()
{
	//qDebug() << "Finished downloading " << httpReply->request().url().path();
	Q_ASSERT(downloading);
	if (httpReply->error()!=QNetworkReply::NoError)
	{
		if (httpReply->error()!=QNetworkReply::OperationCanceledError)
			qWarning() << "WARNING : Problem while downloading JSON description for " << httpReply->request().url().path() << ": "<< httpReply->errorString();
		errorOccured = true;
		httpReply->deleteLater();
		httpReply=Q_NULLPTR;
		downloading=false;
		timeWhenDeletionScheduled = StelApp::getInstance().getTotalRunTime();
		return;
	}

	QByteArray content = httpReply->readAll();
	if (content.isEmpty())
	{
		qWarning() << "WARNING : empty JSON description for " << httpReply->request().url().path();
		errorOccured = true;
		httpReply->deleteLater();
		httpReply=Q_NULLPTR;
		downloading=false;
		return;
	}

	const bool qZcompressed = httpReply->request().url().path().endsWith(".qZ");
	const bool gzCompressed = httpReply->request().url().path().endsWith(".gz");
	httpReply->deleteLater();
	httpReply=Q_NULLPTR;

	Q_ASSERT(loadThread==Q_NULLPTR);
	loadThread = new JsonLoadThread(this, content, qZcompressed, gzCompressed);
	connect(loadThread, SIGNAL(finished()), this, SLOT(jsonLoadFinished()));
	loadThread->start(QThread::LowestPriority);
}

// Called when the element is fully loaded from the JSON file
void MultiLevelJsonBase::jsonLoadFinished()
{
	loadThread->wait();
	delete loadThread;
	loadThread = Q_NULLPTR;
	downloading = false;
	if (errorOccured)
		return;
	try
	{
		loadFromQVariantMap(temporaryResultMap);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: invalid variant map: " << e.what();
		errorOccured = true;
		return;
	}
}


// Delete all the subtiles which were not displayed since more than lastDrawTrigger seconds
void MultiLevelJsonBase::deleteUnusedSubTiles()
{
	if (subTiles.isEmpty())
		return;
	const double now = StelApp::getInstance().getTotalRunTime();
	bool deleteAll = true;
	for (auto* tile : subTiles)
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
		for (auto* tile : subTiles)
			tile->deleteLater();
		subTiles.clear();
	}
	else
	{
		// Nothing to delete at this level, propagate
		for (auto* tile : subTiles)
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

	const int p = qBound(0, static_cast<int>(100.f*tot/(tot+toBeLoaded)), 100);
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
