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
 
#ifndef _CONFIGURATIONDIALOG_HPP_
#define _CONFIGURATIONDIALOG_HPP_

#include <QObject>
#include <QProgressBar>
#include <QNetworkReply>
#include <QFile>
#include "StelDialog.hpp"
#include "StelApp.hpp"
#include "StelMainGraphicsView.hpp"

class Ui_configurationDialogForm;
class QSettings;
class QDataStream;
class QNetworkAccessManager;
class Downloader;

class ConfigurationDialog : public StelDialog
{
	Q_OBJECT;
public:
	ConfigurationDialog();
	virtual ~ConfigurationDialog();
	void languageChanged();
	//! Notify that the application style changed
	void styleChanged();
protected:
	enum UpdatesState { ShowAvailable, Checking, NoUpdates, Downloading,
		Finished, Verifying, UpdatesError, MoveError, DownloadError,
		ChecksumError };
	
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	
	//! Set the content of the "Star catalog updates" box
	void setUpdatesState(ConfigurationDialog::UpdatesState);
	void checkUpdates(void);
	
	Ui_configurationDialogForm* ui;
	QSettings* starSettings;
	QSettings* updatesData;
	Downloader* downloader;
	QString downloadName;
	QString updatesFileName;
	QStringList newCatalogs;
	int downloaded;

private slots:
	void setNoSelectedInfo(void);
	void setAllSelectedInfo(void);
	void setBriefSelectedInfo(void);
	void languageChanged(const QString& languageCode);
	void setStartupTimeMode(void);
	void setDiskViewport(bool);
	void setSphericMirror(bool);
	void cursorTimeOutChanged();
	void cursorTimeOutChanged(double d) {cursorTimeOutChanged();}
	
	void downloadStars(void);
	void cancelDownload(void);
	void retryDownload(void);
	void badChecksum(void);
	void downloadFinished(void);
	void downloadVerifying(void);
	void downloadError(QNetworkReply::NetworkError, QString);
	void updatesDownloadFinished(void);
	void updatesDownloadError(QNetworkReply::NetworkError, QString);
	
	//! Update the labels displaying the current default state
	void updateConfigLabels();

	//! open a file dialog to browse for a directory to save screenshots to
	//! if a directory is selected (i.e. dialog not cancelled), current
	//! value will be changed, but not saved to config file.
	void browseForScreenshotDir();
	void selectScreenshotDir(const QString& dir);

	//! Save the current viewing option including landscape, location and sky culture
	//! This doesn't include the current viewing direction, time and FOV since those 
	//! have specific controls
	void saveCurrentViewOptions();
	
	//! Reset all stellarium options.
	//! This basically replaces the config.ini by the default one
	void setDefaultViewOptions();

	//! The selection of script in the script list has changed
	//! Updates the script information panel
	void scriptSelectionChanged(const QString& s);

	//! Run the currently selected script from the script list.
	void runScriptClicked(void);
	//! Stop the currently running script.
	void stopScriptClicked(void);

	void aScriptIsRunning(void);
	void aScriptHasStopped(void);
	void populateScriptsList(void);

};

class Downloader : public QObject
{
	Q_OBJECT;
public:
	Downloader(const QString& address, const QString& path, quint32 checksum=-1) : address(address),
		path(path), reply(NULL), target(path), useChecksum(checksum<(quint32)-1),
		checksum((quint16)checksum), stream(&target), networkManager(StelApp::getInstance().getNetworkAccessManager()),
		progressBar(StelMainGraphicsView::getInstance().addProgressBar()),
		received(0), total(0)
		{}
	~Downloader();
	void get(bool showBar, const QString& barFormat = "%p%");
	void abort(void);
	QString url(void) { return address; }
	QString errorString(void) { return reply ? reply->errorString() : QString(); }
private:
	const QString address;
	const QString path;
	QNetworkReply* reply;
	QFile target;
	bool useChecksum;
	quint16 checksum;
	QDataStream stream;
	QNetworkAccessManager* networkManager;
	QProgressBar* progressBar;
	qint64 received;
	qint64 total;
	bool showProgressBar;

private slots:
	void readData(void);
	void updateDownloadBar(qint64, qint64);
	void fin(void);
	void err(QNetworkReply::NetworkError);
signals:
	void finished(void);
	void error(QNetworkReply::NetworkError, QString);
	void verifying(void);
	void badChecksum(void);
};

#endif // _CONFIGURATIONDIALOG_HPP_
