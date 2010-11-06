/*
 * Comet and asteroids importer plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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

#ifndef _IMPORT_WINDOW_
#define _IMPORT_WINDOW_

#include <QObject>
#include "StelDialog.hpp"

#include "CAImporter.hpp"

class Ui_importWindow;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressBar;

/*! \brief Window for importing orbital elements from various sources.
  \author Bogdan Marinov
*/
class ImportWindow : public StelDialog
{
	Q_OBJECT
public:
	enum ImportType {
	                 MpcComets,
	                 MpcMinorPlanets
	                 };

	ImportWindow();
	virtual ~ImportWindow();
	void languageChanged();

signals:
	void objectsImported();

private slots:
	//Radio buttons for type
	void switchImportType(bool checked);

	//Single string
	void pasteClipboard();

	//File
	void selectFile();

	//Download
	void bookmarkSelected(QString);

	//Buttons for the list tab
	void acquireObjectData();
	void abortDownload();

	//Online search
	void sendQuery();
	void abortQuery();
	void updateCountdown();
	void resetNotFound();

	//Network
	void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void updateQueryProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadComplete(QNetworkReply * reply);
	void queryComplete(QNetworkReply * reply);

	//! Marks (checks) all items in the results lists
	void markAll();
	//! Unmarks (unchecks) all items in the results lists
	void unmarkAll();
	void addObjects();
	void discardObjects();

	//! resets the dialog to the state it should be in immediately after createDialogContent();.
	void resetDialog();

private:
	CAImporter * ssoManager;
	QList<CAImporter::SsoElements> candidatesForAddition;
	QList<CAImporter::SsoElements> candidatesForUpdate;

	ImportType importType;

	//! wrapper for the single object function to allow multiple formats.
	CAImporter::SsoElements readElementsFromString(QString elements);
	//! wrapper for the file function to allow multiple formats
	QList<CAImporter::SsoElements> readElementsFromFile(ImportType type, QString filePath);

	void populateBookmarksList();
	void populateCandidateObjects(QList<CAImporter::SsoElements>);
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
	int countdown;
	QTimer * countdownTimer;
	void startCountdown();
	void resetCountdown();
	void updateCountdownLabels(int countdownValue);

protected:
	virtual void createDialogContent();
	Ui_importWindow * ui;
};

#endif //_IMPORT_WINDOW_
