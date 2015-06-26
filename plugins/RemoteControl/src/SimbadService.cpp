/*
 * Stellarium Remote Control plugin
 * Copyright (C) 2015 Florian Schaukowitsch
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

#include "SimbadService.hpp"

#include "SimbadSearcher.hpp"
#include "SearchDialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelObjectMgr.hpp"
#include "StelTranslator.hpp"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QMutex>
#include <QThreadPool>
#include <QRunnable>
#include <QWaitCondition>

//! This is used to make the Simbad lookups blocking.
class SimbadLookupTask :public QRunnable
{
public:
	SimbadLookupTask(const QString& url, const QString& searchTerm)
		: url(url), searchTerm(searchTerm)
	{
		parentThread = QThread::currentThread();
		setAutoDelete(false);
	}

	void run() Q_DECL_OVERRIDE
	{
		//make sure this is really a separate thread (QtConcurrent does NOT guarantee that)
		Q_ASSERT(parentThread!=QThread::currentThread());

		//we use a local event loop to simulate synchronous behaviour
		//If we did this in the HTTP thread, this could cause all sorts of problems,
		//(for example: a timeout could disconnect the connection while the handler thread is still working...)
		//but as far as I know thread-pool threads don't have their own loops, so it should be alright
		QEventLoop loop;

		//this MUST be created here for correct thread affinity
		SimbadSearcher* searcher = new SimbadSearcher();
		SimbadLookupReply* reply = searcher->lookup(url,searchTerm,3,0); //last parameter is zero to start lookup immediately
		//statusChanged is only called at the very end of the lookup as far as I can tell
		//so we use it to exit the event queue
		QObject::connect(reply,SIGNAL(statusChanged()),&loop,SLOT(quit()));

		loop.exec();
		//at this point, the reply is finished
		//and we can extract information using getReply
		status = reply->getCurrentStatus();
		statusString = reply->getCurrentStatusString();
		errorString = reply->getErrorString();
		results = reply->getResults();

		//this must be done explicitly here, otherwise the internal QNetworkReply will never be deleted
		reply->deleteNetworkReply();

		delete reply;
		delete searcher; //is deleted while still in the owning thread

		mutex.lock();
		finishedCondition.wakeAll();
		mutex.unlock();
	}

	SimbadLookupReply::SimbadLookupStatus getStatus() { return status; }
	QString getStatusString() { return statusString; }
	QString getErrorString() { return errorString; }
	QMap<QString,Vec3d> getResults() { return results; }
	QMutex* getMutex() { return &mutex; }
	QWaitCondition* getFinishedCondition() { return &finishedCondition; }
private:
	QMutex mutex;
	QWaitCondition finishedCondition;
	QString url;
	QString searchTerm;
	SimbadLookupReply::SimbadLookupStatus status;
	QString statusString;
	QString errorString;
	QMap<QString,Vec3d> results;
	QThread* parentThread;
};

SimbadService::SimbadService(const QByteArray &serviceName, QObject *parent) : AbstractAPIService(serviceName,parent)
{
	//this is run in the main thread
	simbadServerUrl = StelApp::getInstance().getSettings()->value("search/simbad_server_url", SearchDialog::DEF_SIMBAD_URL).toString();
}

void SimbadService::getImpl(const QByteArray& operation, const APIParameters &parameters, APIServiceResponse &response)
{
	if (operation == "lookup")
	{
		//simbad search - this is a bit tricky because we have to block this thread until results are available
		//but QNetworkManager does not provide a synchronous API

		//this may contain greek or other unicode letters
		QString str = QString::fromUtf8(parameters.value("str"));
		str = str.trimmed().toLower();

		if(str.isEmpty())
		{
			response.writeRequestError("empty search string");
			return;
		}

		//Using QtConcurrent is actually bad here!
		//calling waitForFinished may cause the task to be executed in the current thread
		//instead of a separate one, which CAN cause problems with the local event loop leading to crashes
		//See qfutureinterface.cpp line 316 and https://bugreports.qt.io/browse/QTBUG-44296

		//So we have to roll our own solution, using a mutex and a WaitCondition

		SimbadLookupTask task(simbadServerUrl,str);
		task.getMutex()->lock();

		QThreadPool::globalInstance()->start(&task);
		task.getFinishedCondition()->wait(task.getMutex());
		task.getMutex()->unlock();

		QJsonObject obj;
		QString status;
		switch (task.getStatus()) {
			case SimbadLookupReply::SimbadLookupErrorOccured:
				status = QStringLiteral("error");
				obj.insert("errorString",task.getErrorString());
				break;
			case SimbadLookupReply::SimbadLookupFinished:
				if(task.getResults().isEmpty())
					status = QStringLiteral("empty");
				else
					status = QStringLiteral("found");
				break;
			default:
				status = QStringLiteral("unknown");
				break;
		}
		obj.insert("status",status);
		obj.insert("status_i18n",task.getStatusString());

		QJsonArray names;
		QJsonArray positions;

		QMap<QString, Vec3d> res = task.getResults();
		QMapIterator<QString, Vec3d> it(res);
		while(it.hasNext())
		{
			it.next();
			names.append(it.key());

			const Vec3d& pos = it.value();
			QJsonArray position;
			position.append(pos[0]);
			position.append(pos[1]);
			position.append(pos[2]);
			positions.append(position);
		}

		QJsonObject results;
		results.insert("names",names);
		results.insert("positions",positions);

		obj.insert("results",results);

		response.writeJSON(QJsonDocument(obj));
	}
	else
	{
		//TODO some sort of service description?
		response.writeRequestError("unsupported operation. GET: lookup");
	}
}
