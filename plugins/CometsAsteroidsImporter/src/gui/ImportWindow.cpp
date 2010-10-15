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

#include "CAImporter.hpp"

#include "ImportWindow.hpp"
#include "ui_importWindow.h"

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
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

ImportWindow::ImportWindow()
{
	ui = new Ui_importWindow();
	ssoManager = GETSTELMODULE(CAImporter);

	networkManager = StelApp::getInstance().getNetworkAccessManager();

	downloadReply = NULL;
	queryReply = NULL;
	downloadProgressBar = NULL;
	queryProgressBar = NULL;

	countdownTimer = new QTimer(this);

	QHash<QString,QString> asteroidBookmarks;
	QHash<QString,QString> cometBookmarks;
	asteroidBookmarks.insert("MPC's list of bright minor planets at opposition", "http://www.minorplanetcenter.org/iau/Ephemerides/Bright/2010/Soft00Bright.txt");
	asteroidBookmarks.insert("MPCORB: near-Earth asteroids (NEAs)", "http://www.minorplanetcenter.org/iau/MPCORB/NEA.txt");
	asteroidBookmarks.insert("MPCORB: potentially hazardous asteroids (PHAs)", "http://www.minorplanetcenter.org/iau/MPCORB/PHA.txt");
	cometBookmarks.insert("MPC's list of observable comets", "http://www.minorplanetcenter.org/iau/Ephemerides/Comets/Soft00Cmt.txt");
	bookmarks.insert(MpcComets, cometBookmarks);
	bookmarks.insert(MpcMinorPlanets, asteroidBookmarks);
}

ImportWindow::~ImportWindow()
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

void ImportWindow::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->pushButtonAcquire, SIGNAL(clicked()), this, SLOT(acquireObjectData()));
	connect(ui->pushButtonAdd, SIGNAL(clicked()), this, SLOT(addObjects()));

	//connect(ui->pushButtonPaste, SIGNAL(clicked()), this, SLOT(pasteClipboard()));
	connect(ui->pushButtonBrowse, SIGNAL(clicked()), this, SLOT(selectFile()));
	connect(ui->comboBoxBookmarks, SIGNAL(currentIndexChanged(QString)), this, SLOT(bookmarkSelected(QString)));

	//connect(ui->radioButtonSingle, SIGNAL(toggled(bool)), ui->frameSingle, SLOT(setVisible(bool)));
	connect(ui->radioButtonFile, SIGNAL(toggled(bool)), ui->frameFile, SLOT(setVisible(bool)));
	connect(ui->radioButtonURL, SIGNAL(toggled(bool)), ui->frameURL, SLOT(setVisible(bool)));

	connect(ui->radioButtonAsteroids, SIGNAL(toggled(bool)), this, SLOT(switchImportType(bool)));
	connect(ui->radioButtonComets, SIGNAL(toggled(bool)), this, SLOT(switchImportType(bool)));

	connect(ui->pushButtonMarkAll, SIGNAL(clicked()), this, SLOT(markAll()));
	connect(ui->pushButtonMarkNone, SIGNAL(clicked()), this, SLOT(unmarkAll()));

	connect(ui->pushButtonSearch, SIGNAL(clicked()), this, SLOT(sendQuery()));
	connect(ui->lineEditQuery, SIGNAL(textEdited(QString)), this, SLOT(resetNotFound()));
	connect(countdownTimer, SIGNAL(timeout()), this, SLOT(updateCountdown()));

	//ui->radioButtonSingle->setChecked(true);
	//ui->radioButtonFile->setChecked(true);
	ui->frameFile->setVisible(false);
	ui->frameURL->setVisible(false);

	//This box will be displayed when one of the source types is selected
	ui->groupBoxSource->setVisible(false);

	resetCountdown();
	resetNotFound();

	//In case I or someone else forgets the stack widget saved on another page
	ui->stackedWidget->setCurrentIndex(0);
}

void ImportWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void ImportWindow::acquireObjectData()
{
	if (ui->radioButtonFile->isChecked())
	{
		QString filePath = ui->lineEditFilePath->text();
		if (filePath.isEmpty())
			return;

		QList<CAImporter::SsoElements> objects = readElementsFromFile(importType, filePath);
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

void ImportWindow::addObjects()
{
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

	QList<CAImporter::SsoElements> approvedObjects;
	for (int i = 0; i < candidateObjects.count(); i++)
	{
		QString name = candidateObjects.at(i).value("name").toString();
		if (checkedObjectsNames.contains(name))
			approvedObjects.append(candidateObjects.at(i));
	}

	//Write to file
	ssoManager->appendToSolarSystemConfigurationFile(approvedObjects);

	//Refresh the Solar System
	GETSTELMODULE(SolarSystem)->reloadPlanets();

	ui->stackedWidget->setCurrentIndex(0);
	close();
}

void ImportWindow::pasteClipboard()
{
	//ui->lineEditSingle->setText(QApplication::clipboard()->text());
}

void ImportWindow::selectFile()
{
	QString filePath = QFileDialog::getOpenFileName(NULL, "Select a text file", StelFileMgr::getDesktopDir());
	ui->lineEditFilePath->setText(filePath);
}

void ImportWindow::bookmarkSelected(QString bookmarkTitle)
{
	if (bookmarkTitle.isEmpty() || bookmarkTitle == "Select bookmark...")
	{
		ui->lineEditURL->clear();
		return;
	}
	QString bookmarkUrl = bookmarks.value(importType).value(bookmarkTitle);
	ui->lineEditURL->setText(bookmarkUrl);
}

void ImportWindow::populateCandidateObjects(QList<CAImporter::SsoElements> objects)
{
	candidateObjects.clear();

	//Get a list of the current objects
	QStringList existingObjects = GETSTELMODULE(SolarSystem)->getAllPlanetEnglishNames();
	QStringList defaultSsoIds = ssoManager->getAllDefaultSsoIds();
	QStringList currentSsoIds = ssoManager->readAllCurrentSsoIds();

	QListWidget * list = ui->listWidgetObjects;
	list->clear();
	foreach (CAImporter::SsoElements object, objects)
	{
		if (object.contains("name"))
		{
			QString name = object.value("name").toString();
			if (!name.isEmpty())
			{
				QListWidgetItem * item = new QListWidgetItem(list);
				item->setText(name);
				item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
				item->setCheckState(Qt::Unchecked);

				if (object.contains("section_name"))
				{
					QString sectionName = object.value("section_name").toString();
					if (defaultSsoIds.contains(sectionName))
					{
						//Duplicate of a default solar system object
						QFont itemFont(item->font());
						itemFont.setBold(true);
						item->setFont(itemFont);
					} else if (currentSsoIds.contains(sectionName))
					{
						//Duplicate of another existing object
						QFont itemFont(item->font());
						itemFont.setItalic(true);
						item->setFont(itemFont);
					} else if (existingObjects.contains(name))
					{
						//Duplicate name only
						//TODO: Decide what to do in this case
					}
				}
			}

			candidateObjects << object;
		}
	}

	//Select the first item
	if (list->count() > 0)
		list->setCurrentRow(0);
}

void ImportWindow::enableInterface(bool enable)
{
	ui->groupBoxType->setVisible(enable);

	ui->frameFile->setEnabled(enable);
	ui->frameURL->setEnabled(enable);

	ui->radioButtonFile->setEnabled(enable);
	ui->radioButtonURL->setEnabled(enable);

	ui->pushButtonAcquire->setEnabled(enable);
}

CAImporter::SsoElements ImportWindow::readElementsFromString (QString elements)
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

QList<CAImporter::SsoElements> ImportWindow::readElementsFromFile(ImportType type, QString filePath)
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

void ImportWindow::switchImportType(bool checked)
{
	if (ui->radioButtonAsteroids->isChecked())
	{
		importType = MpcMinorPlanets;
	}
	else
	{
		importType = MpcComets;
	}

	ui->comboBoxBookmarks->clear();
	ui->comboBoxBookmarks->addItem("Select bookmark...");
	QStringList bookmarkTitles(bookmarks.value(importType).keys());
	bookmarkTitles.sort();
	ui->comboBoxBookmarks->addItems(bookmarkTitles);

	//Clear the fields
	//ui->lineEditSingle->clear();
	ui->lineEditFilePath->clear();
	ui->lineEditURL->clear();

	//If one of the options is selected, show the rest of the dialog
	ui->groupBoxSource->setVisible(true);
}

void ImportWindow::markAll()
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

void ImportWindow::unmarkAll()
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

void ImportWindow::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
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

void ImportWindow::updateQueryProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (queryProgressBar == NULL)
		return;

	//Just show activity
	queryProgressBar->setValue(0);
	queryProgressBar->setMaximum(0);
}

void ImportWindow::startDownload(QString urlString)
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

	connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	downloadReply = networkManager->get(QNetworkRequest(url));
	connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDownloadProgress(qint64,qint64)));
}

void ImportWindow::downloadComplete(QNetworkReply *reply)
{
	disconnect(this, SLOT(downloadComplete(QNetworkReply*)));
	deleteDownloadProgressBar();
	//TODO: Better handling of the interface
	//dialog->setVisible(true);

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

	QList<CAImporter::SsoElements> objects;
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
		//TODO: As the bookmarks are destroyed with the window, this doesn't make much sense :)
		if (ui->checkBoxAddBookmark->isChecked())
		{
			QString url = reply->url().toString();
			if (!bookmarks.value(importType).values().contains(url))
			{
				//Use the URL as a title for now
				bookmarks[importType].insert(url, url);
			}
		}
	}

	reply->deleteLater();
	downloadReply = NULL;

	//Temporary, until the slot/socket mechanism is ready
	populateCandidateObjects(objects);
	ui->stackedWidget->setCurrentIndex(1);
}

void ImportWindow::deleteDownloadProgressBar()
{
	disconnect(this, SLOT(updateDownloadProgress(qint64,qint64)));

	if (downloadProgressBar)
	{
		downloadProgressBar->setVisible(false);
		downloadProgressBar->deleteLater();
		downloadProgressBar = NULL;
	}
}

void ImportWindow::sendQuery()
{
	if (queryReply)
		return;

	QString query = ui->lineEditQuery->text();
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

	QUrl url;
	url.addQueryItem("ty","e");//Type: ephemerides
	url.addQueryItem("TextArea", query);//Object name query
	url.addQueryItem("e", "-1");//Elements format: MPC 1-line
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
	connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(queryComplete(QNetworkReply*)));
	QNetworkReply * reply = networkManager->post(request, url.encodedQuery());
	connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateQueryProgress(qint64,qint64)));
}

void ImportWindow::queryComplete(QNetworkReply *reply)
{
	disconnect(this, SLOT(queryComplete(QNetworkReply*)));

	//Remove the progress bar
	disconnect(this, SLOT(updateQueryProgress(qint64,qint64)));
	if (queryProgressBar)
	{
		queryProgressBar->setVisible(false);
		queryProgressBar->deleteLater();
		queryProgressBar = NULL;
	}

	if (reply->error())
	{
		qWarning() << "Download error: While trying to access"
		           << reply->url().toString()
		           << "the following error occured:"
		           << reply->errorString();
		enableInterface(true);
		reply->deleteLater();
		downloadReply = NULL;
		return;
	}

	if (reply->header(QNetworkRequest::ContentTypeHeader) != "text/ascii" ||
	    reply->rawHeader(QByteArray("Content-disposition")) != "attachment; filename=elements.txt")
	{
		ui->labelNotFound->setVisible(true);
	}
	else
	{
		QList<CAImporter::SsoElements> objects;
		QTemporaryFile file;
		if (file.open())
		{
			file.write(reply->readAll());
			file.close();

			//Try to read it as a comet first?
			objects = readElementsFromFile(MpcComets, file.fileName());
			if (objects.isEmpty())
				objects = readElementsFromFile(MpcMinorPlanets, file.fileName());
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
			//TODO: As the bookmarks are destroyed with the window, this doesn't make much sense :)
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
	downloadReply = NULL;
}

void ImportWindow::startCountdown()
{
	if (!countdownTimer->isActive())
		countdownTimer->start(1000);//1 second

	//Disable the interface
	ui->lineEditQuery->setEnabled(false);
	ui->pushButtonSearch->setEnabled(false);
	ui->lcdNumber->setVisible(true);
}

void ImportWindow::resetCountdown()
{
	//Stop the timer
	if (countdownTimer->isActive())
		countdownTimer->stop();

	//Reset the counter
	countdown = 60;
	ui->lcdNumber->setVisible(false);
	ui->lcdNumber->display(countdown);

	//Enable the interface
	ui->lineEditQuery->setEnabled(true);
	ui->pushButtonSearch->setEnabled(true);

}

void ImportWindow::updateCountdown()
{
	--countdown;
	if (countdown < 0)
	{
		resetCountdown();
	}
	else
	{
		ui->lcdNumber->display(countdown);
	}
}

void ImportWindow::resetNotFound()
{
	ui->labelNotFound->setVisible(false);
}
