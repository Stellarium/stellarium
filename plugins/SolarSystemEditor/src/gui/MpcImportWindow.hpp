/*
 * Solar System editor plug-in for Stellarium
 *
 * Copyright (C) 2010-2011 Bogdan Marinov
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

#ifndef _MPC_IMPORT_WINDOW_
#define _MPC_IMPORT_WINDOW_

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QStandardItemModel>
#include "StelDialog.hpp"

#include "SolarSystemEditor.hpp"

class Ui_mpcImportWindow;

/*! \brief Window for importing orbital elements from the Minor Planet Center.
  \author Bogdan Marinov
*/
class MpcImportWindow : public StelDialog
{
	Q_OBJECT
public:
	enum ImportType {
	                 MpcComets,
	                 MpcMinorPlanets
	                 };

	MpcImportWindow();
	virtual ~MpcImportWindow();

public slots:
	void retranslate();

signals:
	void objectsImported();

private slots:
	//Radio buttons for type
	void switchImportType(bool checked);

	//File
	void selectFile();

	//Download
	void pasteClipboardURL();
	void bookmarkSelected(QString);

	//Buttons for the list tab
	void acquireObjectData();
	void abortDownload();

	//Online search
	void sendQuery();
	void sendQueryToUrl(QUrl url);
	void abortQuery();
	void updateCountdown();
	void resetNotFound();

	//Network
	void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void updateQueryProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadComplete(QNetworkReply * reply);
	void receiveQueryReply(QNetworkReply * reply);
	void readQueryReply(QNetworkReply * reply);

	//! Marks (checks) all items in the results lists
	void markAll();
	//! Unmarks (unchecks) all items in the results lists
	void unmarkAll();
	void addObjects();
	void discardObjects();

	//! resets the dialog to the state it should be in immediately after createDialogContent();.
	void resetDialog();

private:
	SolarSystemEditor * ssoManager;
	QList<SsoElements> candidatesForAddition;
	QList<SsoElements> candidatesForUpdate;
	QStandardItemModel * candidateObjectsModel;

	ImportType importType;
	
	void updateTexts();

	//! wrapper for the single object function to allow multiple formats.
	SsoElements readElementsFromString(QString elements);
	//! wrapper for the file function to allow multiple formats
	QList<SsoElements> readElementsFromFile(ImportType type, QString filePath);

	void populateBookmarksList();
	//void populateCandidateObjects();
	void populateCandidateObjects(QList<SsoElements>);
	void enableInterface(bool enable);

	//Downloading
	QNetworkAccessManager * networkManager;
	QNetworkReply * downloadReply;
	QNetworkReply * queryReply;
	QProgressBar * downloadProgressBar;
	QProgressBar * queryProgressBar;
	void startDownload(QString url);
	void deleteDownloadProgressBar();
	void deleteQueryProgressBar();

	typedef QHash<QString,QString> Bookmarks;
	QHash<ImportType, Bookmarks> bookmarks;
	void loadBookmarks();
	void loadBookmarksGroup(QVariantMap source, Bookmarks & bookmarkGroup);
	void saveBookmarks();
	void saveBookmarksGroup(Bookmarks & bookmarkGroup, QVariantMap & output);

	//Online search
	QString query;
	int countdown;
	QTimer * countdownTimer;
	void startCountdown();
	void resetCountdown();

protected:
	virtual void createDialogContent();
	Ui_mpcImportWindow * ui;
};

#endif //_MPC_IMPORT_WINDOW_
