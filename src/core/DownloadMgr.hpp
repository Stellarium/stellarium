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

//! @class DownloadMgr
//! Used to download files from the internet.
class DownloadMgr : public QObject
{
	Q_OBJECT;
public:
	DownloadMgr();
	~DownloadMgr();

	//! Fetch a remote URL into a local file.
	//! @param url the URL of the remote file.
	//! @param filePath the path of the local file name which is to be created.
	//! @param csum the expected checksum of the downloaded file.
	void get(const QString& url, const QString& filePath, quint16 csum=0);

	//! Abort the download.
	void abort();

	//! Get the URL of the remote file.
	QString url() {return address;}

	//! After an error, get a description of the error.
	QString errorString() {return reply ? reply->errorString() : QString();}

	//! Get the local file name.
	QString name() {return QFileInfo(path).fileName();}

	// ???
	void setBlockQuit(bool b) {blockQuit = b;}
	void setBarVisible(bool b) {barVisible = b;}
	void setBarFormat(const QString& f) {barFormat = f;}

	//! Set the checksum.
	void setUseChecksum(bool b) {useChecksum = b;}

	//! Find out if the download is still in progress
	bool isDownloading() {return inProgress;}
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
