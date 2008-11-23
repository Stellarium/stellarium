/*
 * Stellarium
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
 
#include <QObject>
#include <QProgressBar>
#include <QNetworkReply>
#include <QFileInfo>
#include "StelDialog.hpp"
#include "StelApp.hpp"
#include "StelMainGraphicsView.hpp"

class QDataStream;
class QNetworkAccessManager;
class QFile;

class DownloadMgr : public QObject
{
	Q_OBJECT;
public:
	DownloadMgr() : networkManager(StelApp::getInstance().getNetworkAccessManager()),
		reply(NULL), target(NULL), useChecksum(false), progressBar(NULL),
		barVisible(true), inProgress(false), blockQuit(true)
		{}
	~DownloadMgr();
	void get(const QString&, const QString&, quint16 csum = 0);
	void abort();
	QString url() { return address; }
	QString errorString() { return reply ? reply->errorString() : QString(); }
	QString name() { return QFileInfo(path).fileName(); }
	void setBlockQuit(bool b) { blockQuit = b; }
	void setBarVisible(bool b) { barVisible = b; }
	void setBarFormat(const QString& f) { barFormat = f; }
	void setUseChecksum(bool b) { useChecksum = b; }
	bool isDownloading() { return inProgress; }
private:
	QString address;
	QString path;
	QNetworkAccessManager* networkManager;
	QNetworkReply* reply;
	QFile* target;
	bool useChecksum;
	quint16 checksum;
	QDataStream* stream;
	QProgressBar* progressBar;
	QString barFormat;
	qint64 received;
	qint64 total;
	bool barVisible;
	bool inProgress;
	bool blockQuit;

private slots:
	void readData();
	void updateDownloadBar(qint64, qint64);
	void fin();
	void err(QNetworkReply::NetworkError);
signals:
	void finished();
	void error(QNetworkReply::NetworkError, QString);
	void verifying();
	void badChecksum();
};
