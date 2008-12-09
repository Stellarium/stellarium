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

#ifndef _STELDOWNLOADMGR_HPP_
#define _STELDOWNLOADMGR_HPP_ 1
 
#include <QObject>
#include <QNetworkReply>

class QProgressBar;
class QDataStream;
class QFile;

//! @class StelDownloadMgr
//! Used to download files from the Internet. QNetworkReply is used internally,
//! although StelDownloadMgr's signal behavior is different (particularly in
//! error handling). Settings are remembered between downloads.
class StelDownloadMgr : public QObject
{
	Q_OBJECT;
public:
	StelDownloadMgr();
	~StelDownloadMgr();

	//! Fetch a remote URL into a local file.
	//! @param url the URL of the remote file.
	//! @param filePath the path of the local file name which is to be
	//! created. If a file already exists at filePath, it will be overwritten.
	//! @param csum (optional) the expected checksum of the downloaded file,
	//! as returned by @c qChecksum(const char* data, uint len). If not provided,
	//! the downloaded file will not be verified.
	//! @sa @c qChecksum(const char* data, uint len) at
	//! http://doc.trolltech.com/4.4/qbytearray.html#qChecksum
	//! @n finished()
	//! @n error(QNetworkReply::NetworkError, QString)
	void get(const QString& url, const QString& filePath, quint16 csum);
	// Overridden version, needed because the default parameter csum=0 can't
	// reliably set useChecksum to false - the actual checksum might be 0.
	void get(const QString& url, const QString& filePath);

	//! Abort the download and remove the partially downloaded file.
	void abort();

	//! Get the URL of the remote file.
	QString url() {return address;}

	//! Get a description of the last error that occurred.
	QString errorString() {return reply ? reply->errorString() : QString();}

	//! Get the local file name.
	QString name();
	
	//! Whether or not to pop up a warning box if the user tries to quit.
	//! @return @c true if a download is in progress @em and @c setBlockQuit(true)
	//! was called. Otherwise, returns @c false.
	//! @sa setBlockQuit(bool)
	bool blockQuit() {return inProgress && block;}

	//! If set to @c true and the user tries to quit Stellarium while
	//! downloading, a warning box will pop up. Default is @c false.
	//! @note
	//! Calling @c setBlockQuit(true) does not guarantee that blockQuit()
	//! will always return @c true.
	//! @sa blockQuit()
	void setBlockQuit(bool b) {block = b;}
	
	//! If set to @c true, a progress bar will appear in the lower right-hand
	//! corner of the screen. Default is @c true.
	void setBarVisible(bool b) {barVisible = b;}
	
	//! Set the text format of the progress bar. Default is @c "%p%".
	//! @sa @c QProgressBar::setFormat(const QString& format) at
	//! http://doc.trolltech.com/4.4/qprogressbar.html#format-prop
	void setBarFormat(const QString& format) {barFormat = format;}

	//! Find out if the download is still in progress.
	bool isDownloading() {return inProgress;}
private:
	QString address;
	QString path;
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
	bool block;

private slots:
	void readData();
	void updateDownloadBar(qint64, qint64);
	void fin();
	void err(QNetworkReply::NetworkError);
signals:
	//! Emitted when the file is completely downloaded and successfully
	//! verified (this differs from @c QNetworkReply). Do not try to open the
	//! destination file before this signal is emitted.
	//! @note
	//! "Completely downloaded" depends on whatever the remote server reports
	//! the file size as. If @c QNetworkReply emits its @c %finished() signal and we
	//! have no info about the total file size, it is assumed to be complete.
	void finished();
	
	//! Emitted whenever the internal @c QNetworkReply object emits an error,
	//! except that @c QNetworkReply::OperationCanceledError is ignored. The
	//! partially downloaded file is removed before emitting this signal.
	//! @param code the error code from @c QNetworkReply::error()
	//! @param errorString the error string from @c QIODevice::errorString()
	//! @sa @c QNetworkReply::NetworkError at
	//! http://doc.trolltech.com/4.4/qnetworkreply.html#NetworkError-enum
	void error(QNetworkReply::NetworkError code, QString errorString);
	
	//! Emitted when verifying a checksum.
	void verifying();
	
	//! Emitted if the provided checksum and downloaded file checksum do not
	//! match.
	void badChecksum();
};

#endif // _STELDOWNLOADMGR_HPP_

