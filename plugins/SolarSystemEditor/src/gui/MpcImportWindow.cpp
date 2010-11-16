/*
 * Solar System editor plug-in for Stellarium
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

#include "SolarSystemEditor.hpp"

#include "MpcImportWindow.hpp"
#include "ui_mpcImportWindow.h"

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QHash>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>

MpcImportWindow::MpcImportWindow()
{
	ui = new Ui_mpcImportWindow();
	ssoManager = GETSTELMODULE(SolarSystemEditor);

	networkManager = StelApp::getInstance().getNetworkAccessManager();

	downloadReply = NULL;
	queryReply = NULL;
	downloadProgressBar = NULL;
	queryProgressBar = NULL;

	countdownTimer = new QTimer(this);

	QHash<QString,QString> asteroidBookmarks;
	QHash<QString,QString> cometBookmarks;
	bookmarks.insert(MpcComets, cometBookmarks);
	bookmarks.insert(MpcMinorPlanets, asteroidBookmarks);
}

MpcImportWindow::~MpcImportWindow()
{
	delete ui;
	delete countdownTimer;
	if (downloadReply)
		downloadReply->deleteLater();
	if (queryReply)
		queryReply->deleteLater();
	if (downloadProgressBar)
		downloadProgressBar->deleteLater();
	if (queryProgressBar)
		queryProgressBar->deleteLater();
}

void MpcImportWindow::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->pushButtonAcquire, SIGNAL(clicked()), this, SLOT(acquireObjectData()));
	connect(ui->pushButtonAbortDownload, SIGNAL(clicked()), this, SLOT(abortDownload()));
	connect(ui->pushButtonAdd, SIGNAL(clicked()), this, SLOT(addObjects()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()), this, SLOT(discardObjects()));

	connect(ui->pushButtonBrowse, SIGNAL(clicked()), this, SLOT(selectFile()));
	connect(ui->pushButtonPasteURL, SIGNAL(clicked()), this, SLOT(pasteClipboardURL()));
	connect(ui->comboBoxBookmarks, SIGNAL(currentIndexChanged(QString)), this, SLOT(bookmarkSelected(QString)));

	//connect(ui->radioButtonSingle, SIGNAL(toggled(bool)), ui->frameSingle, SLOT(setVisible(bool)));
	connect(ui->radioButtonFile, SIGNAL(toggled(bool)), ui->frameFile, SLOT(setVisible(bool)));
	connect(ui->radioButtonURL, SIGNAL(toggled(bool)), ui->frameURL, SLOT(setVisible(bool)));

	connect(ui->radioButtonAsteroids, SIGNAL(toggled(bool)), this, SLOT(switchImportType(bool)));
	connect(ui->radioButtonComets, SIGNAL(toggled(bool)), this, SLOT(switchImportType(bool)));

	connect(ui->pushButtonMarkAll, SIGNAL(clicked()), this, SLOT(markAll()));
	connect(ui->pushButtonMarkNone, SIGNAL(clicked()), this, SLOT(unmarkAll()));

	connect(ui->pushButtonSendQuery, SIGNAL(clicked()), this, SLOT(sendQuery()));
	connect(ui->pushButtonAbortQuery, SIGNAL(clicked()), this, SLOT(abortQuery()));
	connect(ui->lineEditQuery, SIGNAL(textEdited(QString)), this, SLOT(resetNotFound()));
	//connect(ui->lineEditQuery, SIGNAL(editingFinished()), this, SLOT(sendQuery()));
	connect(countdownTimer, SIGNAL(timeout()), this, SLOT(updateCountdown()));

	loadBookmarks();

	resetCountdown();
	resetDialog();
}

void MpcImportWindow::resetDialog()
{
	ui->stackedWidget->setCurrentIndex(0);

	//ui->tabWidget->setCurrentIndex(0);
	ui->groupBoxType->setVisible(true);
	ui->radioButtonAsteroids->setChecked(true);

	ui->radioButtonFile->setChecked(true);
	ui->frameURL->setVisible(false);

	ui->lineEditFilePath->clear();
	ui->lineEditQuery->clear();
	ui->lineEditURL->setText("http://");
	ui->checkBoxAddBookmark->setChecked(false);
	ui->frameBookmarkTitle->setVisible(false);
	ui->comboBoxBookmarks->setCurrentIndex(0);

	ui->radioButtonUpdate->setChecked(true);
	ui->checkBoxOnlyOrbitalElements->setChecked(true);

	//TODO: Is this the right place?
	ui->pushButtonAbortQuery->setVisible(false);
	ui->pushButtonAbortDownload->setVisible(false);

	//Resetting the dialog should not reset the timer
	//resetCountdown();
	resetNotFound();
	enableInterface(true);
}

void MpcImportWindow::populateBookmarksList()
{
	ui->comboBoxBookmarks->clear();
	ui->comboBoxBookmarks->addItem("Select bookmark...");
	QStringList bookmarkTitles(bookmarks.value(importType).keys());
	bookmarkTitles.sort();
	ui->comboBoxBookmarks->addItems(bookmarkTitles);
}

void MpcImportWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void MpcImportWindow::acquireObjectData()
{
	if (ui->radioButtonFile->isChecked())
	{
		QString filePath = ui->lineEditFilePath->text();
		if (filePath.isEmpty())
			return;

		QList<SsoElements> objects = readElementsFromFile(importType, filePath);
		if (objects.isEmpty())
			return;

		//Temporary, until the slot/socket mechanism is ready
		populateCandidateObjects(objects);
		ui->stackedWidget->setCurrentIndex(1);
	}
	else if (ui->radioButtonURL->isChecked())
	{
		QString url = ui->lineEditURL->text();
		if (url.isEmpty())
			return;
		startDownload(url);
	}
	//close();
}

void MpcImportWindow::addObjects()
{
	disconnect(ssoManager, SIGNAL(solarSystemChanged()), this, SLOT(resetDialog()));

	QList<QString> checkedObjectsNames;

	//Extract the marked objects
	while (ui->listWidgetObjects->count() > 0)
	{
		QListWidgetItem * item = ui->listWidgetObjects->takeItem(0);
		if (item->checkState() == Qt::Checked)
		{
			checkedObjectsNames.append(item->text());
		}
		delete item;
	}
	//qDebug() << "Checked:" << checkedObjectsNames;

	QList<SsoElements> approvedForAddition;
	for (int i = 0; i < candidatesForAddition.count(); i++)
	{
		QString name = candidatesForAddition.at(i).value("name").toString();
		if (checkedObjectsNames.contains(name))
			approvedForAddition.append(candidatesForAddition.at(i));
	}

	bool overwrite = ui->radioButtonOverwrite->isChecked();
	QList<SsoElements> approvedForUpdate;
	for (int j = 0; j < candidatesForUpdate.count(); j++)
	{
		QString name = candidatesForUpdate.at(j).value("name").toString();
		if (checkedObjectsNames.contains(name))
		{
			if (overwrite)
			{
				approvedForAddition.append(candidatesForUpdate.at(j));
			}
			else
			{
				approvedForUpdate.append(candidatesForUpdate.at(j));
			}
		}
	}

	//Write to file
	ssoManager->appendToSolarSystemConfigurationFile(approvedForAddition);

	if (ui->radioButtonUpdate->isChecked())
	{
		SolarSystemEditor::UpdateFlags flags(SolarSystemEditor::UpdateNameAndNumber | SolarSystemEditor::UpdateOrbitalElements);
		if (!ui->checkBoxOnlyOrbitalElements->isChecked())
		{
			flags |= SolarSystemEditor::UpdateType;
			flags |= SolarSystemEditor::UpdateMagnitudeParameters;
		}

		ssoManager->updateSolarSystemConfigurationFile(approvedForUpdate, flags);
	}

	//Refresh the Solar System
	GETSTELMODULE(SolarSystem)->reloadPlanets();

	resetDialog();
	emit objectsImported();
}

void MpcImportWindow::discardObjects()
{
	resetDialog();
}

void MpcImportWindow::pasteClipboardURL()
{
	ui->lineEditURL->setText(QApplication::clipboard()->text());
}

void MpcImportWindow::selectFile()
{
	QString filePath = QFileDialog::getOpenFileName(NULL, "Select a text file", StelFileMgr::getDesktopDir());
	ui->lineEditFilePath->setText(filePath);
}

void MpcImportWindow::bookmarkSelected(QString bookmarkTitle)
{
	if (bookmarkTitle.isEmpty() || bookmarkTitle == "Select bookmark...")
	{
		ui->lineEditURL->clear();
		return;
	}
	QString bookmarkUrl = bookmarks.value(importType).value(bookmarkTitle);
	ui->lineEditURL->setText(bookmarkUrl);
}

void MpcImportWindow::populateCandidateObjects(QList<SsoElements> objects)
{
	candidatesForAddition.clear();

	//Get a list of the current objects
	QHash<QString,QString> defaultSsoIdentifiers = ssoManager->getDefaultSsoIdentifiers();
	QHash<QString,QString> loadedSsoIdentifiers = ssoManager->listAllLoadedSsoIdentifiers();

	//Separating the objects into visual groups in the list
	int newDefaultSsoIndex = 0;
	int newLoadedSsoIndex = 0;
	int newNovelSsoIndex = 0;
	int insertionIndex = 0;

	QListWidget * list = ui->listWidgetObjects;
	list->clear();
	foreach (SsoElements object, objects)
	{
		QString name = object.value("name").toString();
		if (name.isEmpty())
			continue;

		QString group = object.value("section_name").toString();
		if (group.isEmpty())
			continue;

		//Prevent name conflicts between asteroids and moons
		if (loadedSsoIdentifiers.contains(name))
		{
			if (loadedSsoIdentifiers.value(name) != group)
			{
				name.append('*');
				object.insert("name", name);
			}
		}

		QListWidgetItem * item = new QListWidgetItem();
		item->setText(name);
		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item->setCheckState(Qt::Unchecked);

		if (defaultSsoIdentifiers.contains(name))
		{
			//Duplicate of a default solar system object
			QFont itemFont(item->font());
			itemFont.setBold(true);
			item->setFont(itemFont);

			candidatesForUpdate.append(object);

			insertionIndex = newDefaultSsoIndex;
			newDefaultSsoIndex++;
			newLoadedSsoIndex++;
			newNovelSsoIndex++;
		}
		else if (loadedSsoIdentifiers.contains(name))
		{
			//Duplicate of another existing object
			QFont itemFont(item->font());
			itemFont.setItalic(true);
			item->setFont(itemFont);

			candidatesForUpdate.append(object);

			insertionIndex = newLoadedSsoIndex;
			newLoadedSsoIndex++;
			newNovelSsoIndex++;
		}
		else
		{
			candidatesForAddition.append(object);

			insertionIndex = newNovelSsoIndex;
			newNovelSsoIndex++;
		}

		list->insertItem(insertionIndex, item);
	}

	//Select the first item
	if (list->count() > 0)
		list->setCurrentRow(0);
}

void MpcImportWindow::enableInterface(bool enable)
{
	ui->groupBoxType->setVisible(enable);

	ui->frameFile->setEnabled(enable);
	ui->frameURL->setEnabled(enable);

	ui->radioButtonFile->setEnabled(enable);
	ui->radioButtonURL->setEnabled(enable);

	ui->pushButtonAcquire->setEnabled(enable);
}

SsoElements MpcImportWindow::readElementsFromString (QString elements)
{
	Q_ASSERT(ssoManager);

	switch (importType)
	{
	case MpcComets:
		return ssoManager->readMpcOneLineCometElements(elements);
	case MpcMinorPlanets:
	default:
		return ssoManager->readMpcOneLineMinorPlanetElements(elements);
	}
}

QList<SsoElements> MpcImportWindow::readElementsFromFile(ImportType type, QString filePath)
{
	Q_ASSERT(ssoManager);

	switch (type)
	{
	case MpcComets:
		return ssoManager->readMpcOneLineCometElementsFromFile(filePath);
	case MpcMinorPlanets:
	default:
		return ssoManager->readMpcOneLineMinorPlanetElementsFromFile(filePath);
	}
}

void MpcImportWindow::switchImportType(bool)
{
	if (ui->radioButtonAsteroids->isChecked())
	{
		importType = MpcMinorPlanets;
	}
	else
	{
		importType = MpcComets;
	}

	populateBookmarksList();

	//Clear the fields
	//ui->lineEditSingle->clear();
	ui->lineEditFilePath->clear();
	ui->lineEditURL->clear();

	//If one of the options is selected, show the rest of the dialog
	ui->groupBoxSource->setVisible(true);
}

void MpcImportWindow::markAll()
{
	QListWidget * const list = ui->listWidgetObjects;
	int rowCount = list->count();
	if (rowCount < 1)
		return;

	for (int row = 0; row < rowCount; row++)
	{
		QListWidgetItem * item = list->item(row);
		if (item)
		{
			item->setCheckState(Qt::Checked);
		}
	}
}

void MpcImportWindow::unmarkAll()
{
	QListWidget * const list = ui->listWidgetObjects;
	int rowCount = list->count();
	if (rowCount < 1)
		return;

	for (int row = 0; row < rowCount; row++)
	{
		QListWidgetItem * item = list->item(row);
		if (item)
		{
			item->setCheckState(Qt::Unchecked);
		}
	}
}

void MpcImportWindow::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (downloadProgressBar == NULL)
		return;

	int currentValue = 0;
	int endValue = 0;

	if (bytesTotal > -1 && bytesReceived <= bytesTotal)
	{
		//Round to the greatest possible derived unit
		while (bytesTotal > 1024)
		{
			bytesReceived = std::floor(bytesReceived / 1024);
			bytesTotal    = std::floor(bytesTotal / 1024);
		}
		currentValue = bytesReceived;
		endValue = bytesTotal;
	}

	downloadProgressBar->setValue(currentValue);
	downloadProgressBar->setMaximum(endValue);
}

void MpcImportWindow::updateQueryProgress(qint64, qint64)
{
	if (queryProgressBar == NULL)
		return;

	//Just show activity
	queryProgressBar->setValue(0);
	queryProgressBar->setMaximum(0);
}

void MpcImportWindow::startDownload(QString urlString)
{
	if (downloadReply)
	{
		//There's already an operation in progress?
		//TODO
		return;
	}

	QUrl url(urlString);
	if (!url.isValid() || url.isRelative() || url.scheme() != "http")
	{
		qWarning() << "Invalid URL:" << urlString;
		return;
	}
	//qDebug() << url.toString();

	//TODO: Interface changes!

	downloadProgressBar = StelApp::getInstance().getGui()->addProgressBar();
	downloadProgressBar->setValue(0);
	downloadProgressBar->setMaximum(0);
	//downloadProgressBar->setFormat("%v/%m");
	downloadProgressBar->setVisible(true);

	//TODO: Better handling of the interface
	//dialog->setVisible(false);
	enableInterface(false);
	ui->pushButtonAbortDownload->setVisible(true);

	connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	downloadReply = networkManager->get(QNetworkRequest(url));
	connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDownloadProgress(qint64,qint64)));
}

void MpcImportWindow::abortDownload()
{
	if (downloadReply == NULL || downloadReply->isFinished())
		return;

	qDebug() << "Aborting download...";

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	deleteDownloadProgressBar();

	downloadReply->abort();
	downloadReply->deleteLater();
	downloadReply = NULL;

	enableInterface(true);
	ui->pushButtonAbortDownload->setVisible(false);
}

void MpcImportWindow::downloadComplete(QNetworkReply *reply)
{
	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	deleteDownloadProgressBar();
	ui->pushButtonAbortDownload->setVisible(false);

	/*
	qDebug() << "reply->isOpen():" << reply->isOpen()
		<< "reply->isReadable():" << reply->isReadable()
		<< "reply->isFinished():" << reply->isFinished();
	*/

	if(reply->error())
	{
		qWarning() << "Download error: While downloading"
		           << reply->url().toString()
				   << "the following error occured:"
				   << reply->errorString();
		enableInterface(true);
		reply->deleteLater();
		downloadReply = NULL;
		return;
	}

	QList<SsoElements> objects;
	QTemporaryFile file;
	if (file.open())
	{
		file.write(reply->readAll());
		file.close();
		objects = readElementsFromFile(importType, file.fileName());
	}
	else
	{
		qWarning() << "Unable to open a temporary file. Aborting operation.";
	}

	if (objects.isEmpty())
	{
		qWarning() << "No objects found in the file downloaded from"
		           << reply->url().toString();
	}
	else
	{
		//The request has been successful: add the URL to bookmarks?
		if (ui->checkBoxAddBookmark->isChecked())
		{
			QString url = reply->url().toString();
			QString title = ui->lineEditBookmarkTitle->text().trimmed();
			//If no title has been entered, use the URL as a title
			if (title.isEmpty())
				title = url;
			if (!bookmarks.value(importType).values().contains(url))
			{
				bookmarks[importType].insert(title, url);
				populateBookmarksList();
				saveBookmarks();
			}
		}
	}

	reply->deleteLater();
	downloadReply = NULL;

	//Temporary, until the slot/socket mechanism is ready
	populateCandidateObjects(objects);
	ui->stackedWidget->setCurrentIndex(1);
	//As this window is persistent, if the Solar System is changed
	//while there is a list, it should be reset.
	connect(ssoManager, SIGNAL(solarSystemChanged()), this, SLOT(resetDialog()));
}

void MpcImportWindow::deleteDownloadProgressBar()
{
	disconnect(this, SLOT(updateDownloadProgress(qint64,qint64)));

	if (downloadProgressBar)
	{
		downloadProgressBar->setVisible(false);
		downloadProgressBar->deleteLater();
		downloadProgressBar = NULL;
	}
}

void MpcImportWindow::sendQuery()
{
	if (queryReply != NULL)
		return;

	QString query = ui->lineEditQuery->text().trimmed();
	if (query.isEmpty())
		return;

	//Progress bar
	queryProgressBar = StelApp::getInstance().getGui()->addProgressBar();
	queryProgressBar->setValue(0);
	queryProgressBar->setMaximum(0);
	queryProgressBar->setFormat("Searching...");
	queryProgressBar->setVisible(true);

	//TODO: Better handling of the interface
	enableInterface(false);
	ui->labelQueryMessage->setVisible(false);

	QUrl url;
	url.addQueryItem("ty","e");//Type: ephemerides
	url.addQueryItem("TextArea", query);//Object name query
	//url.addQueryItem("e", "-1");//Elements format: MPC 1-line
	url.addQueryItem("e", "3");//Elements format: XEphem
	//Yes, all of the rest are necessary
	url.addQueryItem("d","");
	url.addQueryItem("l","");
	url.addQueryItem("i","");
	url.addQueryItem("u","d");
	url.addQueryItem("uto", "0");
	url.addQueryItem("c", "");
	url.addQueryItem("long", "");
	url.addQueryItem("lat", "");
	url.addQueryItem("alt", "");
	url.addQueryItem("raty", "a");
	url.addQueryItem("s", "t");
	url.addQueryItem("m", "m");
	url.addQueryItem("adir", "S");
	url.addQueryItem("oed", "");
	url.addQueryItem("resoc", "");
	url.addQueryItem("tit", "");
	url.addQueryItem("bu", "");
	url.addQueryItem("ch", "c");
	url.addQueryItem("ce", "f");
	url.addQueryItem("js", "f");

	QNetworkRequest request(QUrl("http://scully.cfa.harvard.edu/~cgi/MPEph2"));
	request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");//Is this really necessary?
	request.setHeader(QNetworkRequest::ContentLengthHeader, url.encodedQuery().length());

	startCountdown();
	ui->pushButtonAbortQuery->setVisible(true);
	connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(queryComplete(QNetworkReply*)));
	queryReply = networkManager->post(request, url.encodedQuery());
	connect(queryReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateQueryProgress(qint64,qint64)));
}

void MpcImportWindow::abortQuery()
{
	if (queryReply == NULL)
		return;

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(queryComplete(QNetworkReply*)));
	deleteQueryProgressBar();

	queryReply->abort();
	queryReply->deleteLater();
	queryReply = NULL;

	//resetCountdown();
	enableInterface(true);
	ui->pushButtonAbortQuery->setVisible(false);
}

void MpcImportWindow::queryComplete(QNetworkReply *reply)
{
	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(queryComplete(QNetworkReply*)));
	deleteQueryProgressBar();

	//Hide the abort button - a reply has been received
	ui->pushButtonAbortQuery->setVisible(false);

	if (reply->error())
	{
		qWarning() << "Download error: While trying to access"
		           << reply->url().toString()
		           << "the following error occured:"
		           << reply->errorString();
		ui->labelQueryMessage->setText(reply->errorString());//TODO: Decide if this is a good idea
		ui->labelQueryMessage->setVisible(true);
		enableInterface(true);

		reply->deleteLater();
		queryReply = NULL;
		return;
	}

	if (reply->header(QNetworkRequest::ContentTypeHeader) != "text/ascii" ||
	    reply->rawHeader(QByteArray("Content-disposition")) != "attachment; filename=elements.txt")
	{
		ui->labelQueryMessage->setText("Object not found.");
		ui->labelQueryMessage->setVisible(true);
		enableInterface(true);
	}
	else
	{
		QList<SsoElements> objects;
		QTemporaryFile file;
		if (file.open())
		{
			file.write(reply->readAll());
			file.close();

			/*
			//Try to read it as a comet first?
			objects = readElementsFromFile(MpcComets, file.fileName());
			if (objects.isEmpty())
				objects = readElementsFromFile(MpcMinorPlanets, file.fileName());
			*/
			objects = ssoManager->readXEphemOneLineElementsFromFile(file.fileName());
		}
		else
		{
			qWarning() << "Unable to open a temporary file. Aborting operation.";
		}

		if (objects.isEmpty())
		{
			qWarning() << "No objects found in the file downloaded from"
					   << reply->url().toString();
		}
		else
		{
			//The request has been successful: add the URL to bookmarks?
			if (ui->checkBoxAddBookmark->isChecked())
			{
				QString url = reply->url().toString();
				if (!bookmarks.value(importType).values().contains(url))
				{
					//Use the URL as a title for now
					bookmarks[importType].insert(url, url);
				}
			}

			//Temporary, until the slot/socket mechanism is ready
			populateCandidateObjects(objects);
			ui->stackedWidget->setCurrentIndex(1);
		}
	}

	reply->deleteLater();
	queryReply = NULL;
}

void MpcImportWindow::deleteQueryProgressBar()
{
	disconnect(this, SLOT(updateQueryProgress(qint64,qint64)));
	if (queryProgressBar)
	{
		queryProgressBar->setVisible(false);
		queryProgressBar->deleteLater();
		queryProgressBar = NULL;
	}
}

void MpcImportWindow::startCountdown()
{
	if (!countdownTimer->isActive())
		countdownTimer->start(1000);//1 second

	//Disable the interface
	ui->lineEditQuery->setEnabled(false);
	ui->pushButtonSendQuery->setEnabled(false);
}

void MpcImportWindow::resetCountdown()
{
	//Stop the timer
	if (countdownTimer->isActive())
	{
		countdownTimer->stop();

		//If the query is still active, kill it
		if (queryReply != NULL && queryReply->isRunning())
		{
			abortQuery();
			ui->labelQueryMessage->setText("The query timed out. You can try again, now or later.");
			ui->labelQueryMessage->setVisible(true);
		}
	}

	//Reset the counter
	countdown = 60;

	//Enable the interface
	ui->lineEditQuery->setEnabled(true);
	ui->pushButtonSendQuery->setEnabled(true);
}

void MpcImportWindow::updateCountdown()
{
	--countdown;
	if (countdown < 0)
	{
		resetCountdown();
	}
	//If there has been an answer
	else if (countdown > 50 && queryReply == NULL)
	{
		resetCountdown();
	}
}

void MpcImportWindow::resetNotFound()
{
	ui->labelQueryMessage->setVisible(false);
}

void MpcImportWindow::loadBookmarks()
{
	bookmarks[MpcComets].clear();
	bookmarks[MpcMinorPlanets].clear();

	QString bookmarksFilePath(StelFileMgr::getUserDir() + "/modules/SolarSystemEditor/bookmarks.json");
	if (StelFileMgr::isReadable(bookmarksFilePath))
	{
		QFile bookmarksFile(bookmarksFilePath);
		if (bookmarksFile.open(QFile::ReadOnly | QFile::Text))
		{
			QVariantMap jsonRoot = StelJsonParser::parse(bookmarksFile.readAll()).toMap();

			loadBookmarksGroup(jsonRoot.value("mpcMinorPlanets").toMap(), bookmarks[MpcMinorPlanets]);
			loadBookmarksGroup(jsonRoot.value("mpcComets").toMap(), bookmarks[MpcComets]);

			bookmarksFile.close();

			//If nothing was read, continue
			if (!bookmarks.value(MpcComets).isEmpty() && !bookmarks[MpcMinorPlanets].isEmpty())
				return;
		}
	}

	qDebug() << "Bookmarks file can't be read. Hard-coded bookmarks will be used.";

	//Initialize with hard-coded values
	bookmarks[MpcMinorPlanets].insert("MPC's list of bright minor planets at opposition", "http://www.minorplanetcenter.org/iau/Ephemerides/Bright/2010/Soft00Bright.txt");
	bookmarks[MpcMinorPlanets].insert("MPCORB: near-Earth asteroids (NEAs)", "http://www.minorplanetcenter.org/iau/MPCORB/NEA.txt");
	bookmarks[MpcMinorPlanets].insert("MPCORB: potentially hazardous asteroids (PHAs)", "http://www.minorplanetcenter.org/iau/MPCORB/PHA.txt");
	bookmarks[MpcComets].insert("MPC's list of observable comets", "http://www.minorplanetcenter.org/iau/Ephemerides/Comets/Soft00Cmt.txt");

	//Try to save them to a file
	saveBookmarks();
}

void MpcImportWindow::loadBookmarksGroup(QVariantMap source, Bookmarks & bookmarkGroup)
{
	if (source.isEmpty())
		return;

	foreach (QString title, source.keys())
	{
		QString url = source.value(title).toString();
		if (!url.isEmpty())
			bookmarkGroup.insert(title, url);
	}
}

void MpcImportWindow::saveBookmarks()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir() + "/modules/SolarSystemEditor");

		QVariantMap jsonRoot;

		QString bookmarksFilePath(StelFileMgr::getUserDir() + "/modules/SolarSystemEditor/bookmarks.json");

		//If the file exists, load it first
		if (StelFileMgr::isReadable(bookmarksFilePath))
		{
			QFile bookmarksFile(bookmarksFilePath);
			if (bookmarksFile.open(QFile::ReadOnly | QFile::Text))
			{
				jsonRoot = StelJsonParser::parse(bookmarksFile.readAll()).toMap();
				bookmarksFile.close();
			}
		}

		QFile bookmarksFile(bookmarksFilePath);
		if (bookmarksFile.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
		{
			QVariantMap minorPlanetsObject;
			saveBookmarksGroup(bookmarks[MpcMinorPlanets], minorPlanetsObject);
			//qDebug() << minorPlanetsObject.keys();
			jsonRoot.insert("mpcMinorPlanets", minorPlanetsObject);

			QVariantMap cometsObject;
			saveBookmarksGroup(bookmarks[MpcComets], cometsObject);
			jsonRoot.insert("mpcComets", cometsObject);

			StelJsonParser::write(jsonRoot, &bookmarksFile);
			bookmarksFile.close();

			qDebug() << "Bookmarks file saved to" << bookmarksFilePath;
			return;
		}
		else
		{
			qDebug() << "Unable to write bookmarks file to" << bookmarksFilePath;
		}
	}
	catch (std::exception & e)
	{
		qDebug() << "Unable to save bookmarks file:" << e.what();
	}
}

void MpcImportWindow::saveBookmarksGroup(Bookmarks & bookmarkGroup, QVariantMap & output)
{
	foreach (QString title, bookmarkGroup.keys())
	{
		output.insert(title, bookmarkGroup.value(title));
	}
}
