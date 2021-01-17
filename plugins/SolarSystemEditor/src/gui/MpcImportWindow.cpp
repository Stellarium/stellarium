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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "SolarSystemEditor.hpp"

#include "MpcImportWindow.hpp"
#include "ui_mpcImportWindow.h"

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "SolarSystem.hpp"
#include "StelProgressController.hpp"
#include "SearchDialog.hpp"
#include "StelUtils.hpp"

#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QSortFilterProxyModel>
#include <QHash>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStandardItemModel>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QDir>
#include <stdexcept>

MpcImportWindow::MpcImportWindow()
	: StelDialog("SolarSystemEditorMPCimport")
	, importType(ImportType())
	, downloadReply(Q_NULLPTR)
	, queryReply(Q_NULLPTR)
	, downloadProgressBar(Q_NULLPTR)
	, queryProgressBar(Q_NULLPTR)
	, countdown(0)
{
	ui = new Ui_mpcImportWindow();
	ssoManager = GETSTELMODULE(SolarSystemEditor);

	networkManager = StelApp::getInstance().getNetworkAccessManager();

	countdownTimer = new QTimer(this);

	QHash<QString,QString> asteroidBookmarks;
	QHash<QString,QString> cometBookmarks;
	bookmarks.insert(MpcComets, cometBookmarks);
	bookmarks.insert(MpcMinorPlanets, asteroidBookmarks);

	candidateObjectsModel = new QStandardItemModel(this);
}

MpcImportWindow::~MpcImportWindow()
{
	delete ui;
	delete countdownTimer;
	candidateObjectsModel->clear();
	delete candidateObjectsModel;
	if (downloadReply)
		downloadReply->deleteLater();
	if (queryReply)
		queryReply->deleteLater();
	if (downloadProgressBar)
		StelApp::getInstance().removeProgressBar(downloadProgressBar);
	if (queryProgressBar)
		StelApp::getInstance().removeProgressBar(queryProgressBar);
}

void MpcImportWindow::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->pushButtonAcquire, SIGNAL(clicked()),
	        this, SLOT(acquireObjectData()));
	connect(ui->pushButtonAbortDownload, SIGNAL(clicked()),
	        this, SLOT(abortDownload()));
	connect(ui->pushButtonAdd, SIGNAL(clicked()), this, SLOT(addObjects()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()),
	        this, SLOT(discardObjects()));

	connect(ui->pushButtonBrowse, SIGNAL(clicked()), this, SLOT(selectFile()));
	connect(ui->comboBoxBookmarks, SIGNAL(currentIndexChanged(QString)),
	        this, SLOT(bookmarkSelected(QString)));

	connect(ui->radioButtonFile, SIGNAL(toggled(bool)),
	        ui->frameFile, SLOT(setVisible(bool)));
	connect(ui->radioButtonURL, SIGNAL(toggled(bool)),
	        ui->frameURL, SLOT(setVisible(bool)));

	connect(ui->radioButtonAsteroids, SIGNAL(toggled(bool)),
	        this, SLOT(switchImportType(bool)));
	connect(ui->radioButtonComets, SIGNAL(toggled(bool)),
	        this, SLOT(switchImportType(bool)));

	connect(ui->pushButtonMarkAll, SIGNAL(clicked()),
	        this, SLOT(markAll()));
	connect(ui->pushButtonMarkNone, SIGNAL(clicked()),
	        this, SLOT(unmarkAll()));

	connect(ui->pushButtonSendQuery, SIGNAL(clicked()),
	        this, SLOT(sendQuery()));
	connect(ui->lineEditQuery, SIGNAL(returnPressed()),
		this, SLOT(sendQuery()));
	connect(ui->pushButtonAbortQuery, SIGNAL(clicked()),
	        this, SLOT(abortQuery()));
	connect(ui->lineEditQuery, SIGNAL(textEdited(QString)),
	        this, SLOT(resetNotFound()));
	//connect(ui->lineEditQuery, SIGNAL(editingFinished()), this, SLOT(sendQuery()));
	connect(countdownTimer, SIGNAL(timeout()), this, SLOT(updateCountdown()));

	QSortFilterProxyModel * filterProxyModel = new QSortFilterProxyModel(this);
	filterProxyModel->setSourceModel(candidateObjectsModel);
	filterProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->listViewObjects->setModel(filterProxyModel);
	connect(ui->lineEditSearch, SIGNAL(textChanged(const QString&)),
	        filterProxyModel, SLOT(setFilterFixedString(const QString&)));

	loadBookmarks();
	updateTexts();

	resetCountdown();
	resetDialog();
}

void MpcImportWindow::updateTexts()
{
	QString linkText("<a href=\"https://www.minorplanetcenter.net/iau/MPEph/MPEph.html\">Minor Planet &amp; Comet Ephemeris Service</a>");
	// TRANSLATORS: A link showing the text "Minor Planet & Comet Ephemeris Service" is inserted.
	QString queryString(q_("Query the MPC's %1:"));
	ui->labelQueryLink->setText(QString(queryString).arg(linkText));
	
	QString firstLine(q_("Only one result will be returned if the query is successful."));
	QString secondLine(q_("Both comets and asteroids can be identified with their number, name (in English) or provisional designation."));
	QString cPrefix("<b>C/</b>");
	QString pPrefix("<b>P/</b>");
	QString cometQuery("<tt>C/Halley</tt>");
	QString cometName("1P/Halley");
	QString asteroidQuery("<tt>Halley</tt>");
	QString asteroidName("(2688) Halley");
	QString nameWarning(q_("Comet <i>names</i> need to be prefixed with %1 or %2. If more than one comet matches a name, only the first result will be returned. For example, searching for \"%3\" will return %4, Halley's Comet, but a search for \"%5\" will return the asteroid %6."));
	QString thirdLine = QString(nameWarning).arg(cPrefix, pPrefix, cometQuery,
	                                             cometName, asteroidQuery,
	                                             asteroidName);
	ui->labelQueryInstructions->setText(QString("%1<br/>%2<br/>%3").arg(firstLine, secondLine, thirdLine));
}

void MpcImportWindow::resetDialog()
{
	ui->stackedWidget->setCurrentIndex(0);

	//ui->tabWidget->setCurrentIndex(0);
	ui->groupBoxType->setVisible(true);
	ui->radioButtonAsteroids->setChecked(true);

	ui->radioButtonURL->setChecked(true);
	ui->frameFile->setVisible(false);

	ui->lineEditFilePath->clear();
	ui->lineEditQuery->clear();
	ui->lineEditURL->setText("https://");
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
	ui->comboBoxBookmarks->addItem(q_("Select bookmark..."));
	QStringList bookmarkTitles(bookmarks.value(importType).keys());
	bookmarkTitles.sort();
	ui->comboBoxBookmarks->addItems(bookmarkTitles);
}

void MpcImportWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateTexts();
	}
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

	// Collect names of marked objects
	//TODO: Something smarter?
	for (int row = 0; row < candidateObjectsModel->rowCount(); row++)
	{
		QStandardItem * item = candidateObjectsModel->item(row);
		if (item->checkState() == Qt::Checked)
		{
			checkedObjectsNames.append(item->text());
			if (row==0)
				SearchDialog::extSearchText = item->text();
		}
	}
	//qDebug() << "Checked:" << checkedObjectsNames;

	// collect from candidatesForAddition all candidates that were selected by the user into `approvedForAddition` ...
	QList<SsoElements> approvedForAddition;
	for (int i = 0; i < candidatesForAddition.count(); i++)
	{
		auto candidate = candidatesForAddition.at(i);
		QString name = candidate.value("name").toString();
		if (checkedObjectsNames.contains(name))
			approvedForAddition.append(candidate);
	}

	//qDebug() << "Approved for addition:" << approvedForAddition;

	// collect all new (!!!) candidates that were selected by the user into `approvedForUpdate`
	// if the user opted to overwrite, those candidates are added to `approvedForAddition` instead
	bool overwrite = ui->radioButtonOverwrite->isChecked();
	QList<SsoElements> approvedForUpdate;
	for (int j = 0; j < candidatesForUpdate.count(); j++)
	{
		auto candidate = candidatesForUpdate.at(j);
		QString name = candidate.value("name").toString();
		if (checkedObjectsNames.contains(name))
		{
			// XXX: odd... if "overwrite" is false, data is overwritten anyway.
			if (overwrite)
			{
				approvedForAddition.append(candidate);
			}
			else
			{
				approvedForUpdate.append(candidate);
			}
		}
	}

	//qDebug() << "Approved for updates:" << approvedForUpdate;

	// append *** + update *** the approvedForAddition candidates to custom solar system config
	ssoManager->appendToSolarSystemConfigurationFile(approvedForAddition);

	// if instead "update existing objects" was selected, update existing candidates from `approvedForUpdate` in custom solar system config
	// update name, MPC number, orbital elements
	// if the user asked more to update, include type (asteroid, comet, plutino, cubewano, ...) and magnitude parameters
	bool update = ui->radioButtonUpdate->isChecked();
	// ASSERT(update != overwrite); // because of radiobutton behaviour. TODO this UI is not very clear anyway.
	if (update) 
	{
		SolarSystemEditor::UpdateFlags flags(SolarSystemEditor::UpdateNameAndNumber | SolarSystemEditor::UpdateOrbitalElements);
		bool onlyorbital = ui->checkBoxOnlyOrbitalElements->isChecked();
		if (!onlyorbital)
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
	ui->lineEditURL->setText(QGuiApplication::clipboard()->text());
}

void MpcImportWindow::selectFile()
{
	QString filter = q_("Plain Text File");
	filter.append(" (*.txt);;");
	filter.append(q_("All Files"));
	filter.append(" (*.*)");
	QString filePath = QFileDialog::getOpenFileName(Q_NULLPTR, q_("Select a file"), QDir::homePath(), filter);
	ui->lineEditFilePath->setText(filePath);
}

void MpcImportWindow::bookmarkSelected(QString bookmarkTitle)
{
	if (bookmarkTitle.isEmpty() || bookmarks.value(importType).value(bookmarkTitle).isEmpty())
	{
		ui->lineEditURL->clear();
		return;
	}
	QString bookmarkUrl = bookmarks.value(importType).value(bookmarkTitle);
	ui->lineEditURL->setText(bookmarkUrl);
}

void MpcImportWindow::populateCandidateObjects(QList<SsoElements> objects)
{
	candidatesForAddition.clear();	// new objects
	candidatesForUpdate.clear();	// existing objects

	//Get a list of the current objects
	//QHash<QString,QString> defaultSsoIdentifiers = ssoManager->getDefaultSsoIdentifiers();
	QHash<QString,QString> loadedSsoIdentifiers = ssoManager->listAllLoadedSsoIdentifiers();

	//Separate the objects into visual groups in the list
	//int newDefaultSsoIndex = 0;
	int newLoadedSsoIndex = 0;
	int newNovelSsoIndex = 0;
	int insertionIndex = 0;

	QStandardItemModel * model = candidateObjectsModel;
	model->clear();
	model->setColumnCount(1);

	for (auto object : objects)
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

		QStandardItem * item = new QStandardItem();
		item->setText(name);
		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item->setCheckState(Qt::Unchecked);

//		if (defaultSsoIdentifiers.contains(name))
//		{
//			//Duplicate of a default solar system object
//			QFont itemFont(item->font());
//			itemFont.setBold(true);
//			item->setFont(itemFont);

//			candidatesForUpdate.append(object);

//			insertionIndex = newDefaultSsoIndex;
//			newDefaultSsoIndex++;
//			newLoadedSsoIndex++;
//			newNovelSsoIndex++;
//		}
//		else

		// identify existing (in italic) and new objects
		if (loadedSsoIdentifiers.contains(name))
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

		model->insertRow(insertionIndex, item);
	}

	//Scroll to the first items
	ui->listViewObjects->scrollToTop();
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
	int rowCount = candidateObjectsModel->rowCount();
	if (rowCount < 1)
		return;

	for (int row = 0; row < rowCount; row++)
	{
		QStandardItem * item = candidateObjectsModel->item(row);
		if (item)
		{
			item->setCheckState(Qt::Checked);
		}
	}
}

void MpcImportWindow::unmarkAll()
{
	int rowCount = candidateObjectsModel->rowCount();
	if (rowCount < 1)
		return;

	for (int row = 0; row < rowCount; row++)
	{
		QStandardItem * item = candidateObjectsModel->item(row);
		if (item)
		{
			item->setCheckState(Qt::Unchecked);
		}
	}
}

void MpcImportWindow::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (downloadProgressBar == Q_NULLPTR)
		return;

	int currentValue = 0;
	int endValue = 0;

	if (bytesTotal > -1 && bytesReceived <= bytesTotal)
	{
		//Round to the greatest possible derived unit
		while (bytesTotal > 1024)
		{
			bytesReceived >>= 10;
			bytesTotal    >>= 10;
		}
		currentValue = static_cast<int>(bytesReceived);
		endValue = static_cast<int>(bytesTotal);
	}

	downloadProgressBar->setValue(currentValue);
	downloadProgressBar->setRange(0, endValue);
}

void MpcImportWindow::updateQueryProgress(qint64, qint64)
{
	if (queryProgressBar == Q_NULLPTR)
		return;

	//Just show activity
	queryProgressBar->setValue(0);
	queryProgressBar->setRange(0, 0);
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
	if (!url.isValid() || url.isRelative() || !url.scheme().startsWith("http", Qt::CaseInsensitive))
	{
		qWarning() << "Invalid URL:" << urlString;
		return;
	}
	//qDebug() << url.toString();

	//TODO: Interface changes!

	downloadProgressBar = StelApp::getInstance().addProgressBar();
	downloadProgressBar->setValue(0);
	downloadProgressBar->setRange(0, 0);

	//TODO: Better handling of the interface
	//dialog->setVisible(false);
	enableInterface(false);
	ui->pushButtonAbortDownload->setVisible(true);

	connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	QNetworkRequest request;
	request.setUrl(QUrl(url));
	request.setRawHeader("User-Agent", StelUtils::getUserAgentString().toUtf8());
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
	downloadReply = networkManager->get(request);
	connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDownloadProgress(qint64,qint64)));
}

void MpcImportWindow::abortDownload()
{
	if (downloadReply == Q_NULLPTR || downloadReply->isFinished())
		return;

	qDebug() << "Aborting download...";

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	deleteDownloadProgressBar();

	downloadReply->abort();
	downloadReply->deleteLater();
	downloadReply = Q_NULLPTR;

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

	if(reply->error() || reply->bytesAvailable()==0)
	{
		qWarning() << "Download error: While downloading"
		           << reply->url().toString()
				   << "the following error occured:"
				   << reply->errorString();
		enableInterface(true);
		reply->deleteLater();
		downloadReply = Q_NULLPTR;
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
	downloadReply = Q_NULLPTR;

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
		StelApp::getInstance().removeProgressBar(downloadProgressBar);
		downloadProgressBar = Q_NULLPTR;
	}
}

void MpcImportWindow::sendQuery()
{
	if (queryReply != Q_NULLPTR)
		return;

	query = ui->lineEditQuery->text().trimmed();
	if (query.isEmpty())
		return;

	//Progress bar
	queryProgressBar = StelApp::getInstance().addProgressBar();
	queryProgressBar->setValue(0);
	queryProgressBar->setRange(0, 0);
	queryProgressBar->setFormat("Searching...");

	//TODO: Better handling of the interface
	enableInterface(false);
	ui->labelQueryMessage->setVisible(false);

	startCountdown();
	ui->pushButtonAbortQuery->setVisible(true);

	//sendQueryToUrl(QUrl("http://stellarium.org/mpc-mpeph"));
	//sendQueryToUrl(QUrl("http://scully.cfa.harvard.edu/cgi-bin/mpeph2.cgi"));
	// MPC requirements now :(
	sendQueryToUrl(QUrl("https://www.minorplanetcenter.net/cgi-bin/mpeph2.cgi"));
}

void MpcImportWindow::sendQueryToUrl(QUrl url)
{
	QUrlQuery q(url);
	q.addQueryItem("ty","e");//Type: ephemerides
	q.addQueryItem("TextArea", query);//Object name query
	q.addQueryItem("e", "-1");//Elements format: MPC 1-line
	//Switch to MPC 1-line format --AW
	//XEphem's format is used instead because it doesn't truncate object names.
	//q.addQueryItem("e", "3");//Elements format: XEphem
	//Yes, all of the rest are necessary
	q.addQueryItem("d","");
	q.addQueryItem("l","");
	q.addQueryItem("i","");
	q.addQueryItem("u","d");
	q.addQueryItem("uto", "0");
	q.addQueryItem("c", "");
	q.addQueryItem("long", "");
	q.addQueryItem("lat", "");
	q.addQueryItem("alt", "");
	q.addQueryItem("raty", "a");
	q.addQueryItem("s", "t");
	q.addQueryItem("m", "m");
	q.addQueryItem("adir", "S");
	q.addQueryItem("oed", "");
	q.addQueryItem("resoc", "");
	q.addQueryItem("tit", "");
	q.addQueryItem("bu", "");
	q.addQueryItem("ch", "c");
	q.addQueryItem("ce", "f");
	q.addQueryItem("js", "f");
	url.setQuery(q);

	QNetworkRequest request;
	request.setUrl(QUrl(url));
	request.setRawHeader("User-Agent", StelUtils::getUserAgentString().toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded"); //Is this really necessary?
	request.setHeader(QNetworkRequest::ContentLengthHeader, url.query(QUrl::FullyEncoded).length());
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

	connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(receiveQueryReply(QNetworkReply*)));
	queryReply = networkManager->post(request, url.query(QUrl::FullyEncoded).toUtf8());	
	connect(queryReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateQueryProgress(qint64,qint64)));
}

void MpcImportWindow::abortQuery()
{
	if (queryReply == Q_NULLPTR)
		return;

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(receiveQueryReply(QNetworkReply*)));
	deleteQueryProgressBar();

	queryReply->abort();
	queryReply->deleteLater();
	queryReply = Q_NULLPTR;

	//resetCountdown();
	enableInterface(true);
	ui->pushButtonAbortQuery->setVisible(false);
}

void MpcImportWindow::receiveQueryReply(QNetworkReply *reply)
{
	if (reply == Q_NULLPTR)
		return;

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(receiveQueryReply(QNetworkReply*)));

	int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (statusCode == 301 || statusCode == 302 || statusCode == 307)
	{
		QUrl rawUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		QUrl redirectUrl(rawUrl.toString(QUrl::RemoveQuery));
		qDebug() << "The search query has been redirected to" << redirectUrl.toString();

		//TODO: Add counter and cycle check.

		reply->deleteLater();
		queryReply = Q_NULLPTR;
		sendQueryToUrl(redirectUrl);
		return;
	}

	deleteQueryProgressBar();

	//Hide the abort button - a reply has been received
	ui->pushButtonAbortQuery->setVisible(false);

	if (reply->error() || reply->bytesAvailable()==0)
	{
		qWarning() << "Download error: While trying to access"
		           << reply->url().toString()
			   << "the following error occured:"
		           << reply->errorString();
		ui->labelQueryMessage->setText(reply->errorString());//TODO: Decide if this is a good idea
		ui->labelQueryMessage->setVisible(true);
		enableInterface(true);

		reply->deleteLater();
		queryReply = Q_NULLPTR;
		return;
	}

	QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
	QString contentDisposition = reply->rawHeader(QByteArray("Content-disposition"));
	if (contentType == "text/ascii" &&
	    contentDisposition == "attachment; filename=elements.txt")
	{
		readQueryReply(reply);
	}
	else
	{
		ui->labelQueryMessage->setText("Object not found.");
		ui->labelQueryMessage->setVisible(true);
		enableInterface(true);
	}

	reply->deleteLater();
	queryReply = Q_NULLPTR;
}

void MpcImportWindow::readQueryReply(QNetworkReply * reply)
{
	Q_ASSERT(reply);

	QList<SsoElements> objects;
	QTemporaryFile file;
	if (file.open())
	{
		file.write(reply->readAll());		
		file.close();

		QRegExp cometProvisionalDesignation("[PCDXI]/");
		QRegExp cometDesignation("(\\d)+[PCDXI]/");
		QString queryData = ui->lineEditQuery->text().trimmed();

		if (cometDesignation.indexIn(queryData) == 0 || cometProvisionalDesignation.indexIn(queryData) == 0)
			objects = readElementsFromFile(MpcComets, file.fileName());
		else
			objects = readElementsFromFile(MpcMinorPlanets, file.fileName());

		/*
		//Try to read it as a comet first?
		objects = readElementsFromFile(MpcComets, file.fileName());
		if (objects.isEmpty())
			objects = readElementsFromFile(MpcMinorPlanets, file.fileName());
		*/
		//XEphem given wrong data for comets --AW
		//objects = ssoManager->readXEphemOneLineElementsFromFile(file.fileName());
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

void MpcImportWindow::deleteQueryProgressBar()
{
	disconnect(this, SLOT(updateQueryProgress(qint64,qint64)));
	if (queryProgressBar)
	{
		StelApp::getInstance().removeProgressBar(queryProgressBar);
		queryProgressBar = Q_NULLPTR;
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
		if (queryReply != Q_NULLPTR && queryReply->isRunning())
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
	else if (countdown > 50 && queryReply == Q_NULLPTR)
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
	bool outdated = false;
	if (StelFileMgr::isReadable(bookmarksFilePath))
	{
		QFile bookmarksFile(bookmarksFilePath);
		if (bookmarksFile.open(QFile::ReadOnly | QFile::Text))
		{
			QVariantMap jsonRoot;
			QString fileVersion = "0.0.0";
			try
			{
				jsonRoot = StelJsonParser::parse(bookmarksFile.readAll()).toMap();
				bookmarksFile.close();

				fileVersion = jsonRoot.value("version").toString();
				if (fileVersion.isEmpty())
					fileVersion = "0.0.0";

				loadBookmarksGroup(jsonRoot.value("mpcMinorPlanets").toMap(), bookmarks[MpcMinorPlanets]);
				loadBookmarksGroup(jsonRoot.value("mpcComets").toMap(), bookmarks[MpcComets]);
			}
			catch (std::runtime_error &e)
			{
				qDebug() << "File format is wrong! Error: " << e.what();
				outdated = true;
			}

			if (StelUtils::compareVersions(fileVersion, SOLARSYSTEMEDITOR_PLUGIN_VERSION)<0)
				outdated = true; // Oops... the list is outdated!

			//If nothing was read, continue
			if (!bookmarks.value(MpcComets).isEmpty() && !bookmarks[MpcMinorPlanets].isEmpty() && !outdated)
				return;
		}
	}

	if (outdated)
		qDebug() << "Bookmarks file is outdated! The list will be upgraded by hard-coded bookmarks.";
	else
		qDebug() << "Bookmarks file can't be read. Hard-coded bookmarks will be used.";

	//Initialize with hard-coded values
	bookmarks[MpcMinorPlanets].insert("MPC's list of bright minor planets at opposition in 2018", "https://www.minorplanetcenter.net/iau/Ephemerides/Bright/2018/Soft00Bright.txt");
	bookmarks[MpcMinorPlanets].insert("MPC's list of observable critical-list numbered minor planets", "https://www.minorplanetcenter.net/iau/Ephemerides/CritList/Soft00CritList.txt");
	bookmarks[MpcMinorPlanets].insert("MPC's list of observable distant minor planets", "https://www.minorplanetcenter.net/iau/Ephemerides/Distant/Soft00Distant.txt");
	bookmarks[MpcMinorPlanets].insert("MPC's list of observable unusual minor planets", "https://www.minorplanetcenter.net/iau/Ephemerides/Unusual/Soft00Unusual.txt");
	bookmarks[MpcMinorPlanets].insert("MPCORB: near-Earth asteroids (NEAs)", "https://www.minorplanetcenter.net/iau/MPCORB/NEA.txt");
	bookmarks[MpcMinorPlanets].insert("MPCORB: potentially hazardous asteroids (PHAs)", "https://www.minorplanetcenter.net/iau/MPCORB/PHA.txt");
	bookmarks[MpcMinorPlanets].insert("MPCORB: TNOs, Centaurs and SDOs", "https://www.minorplanetcenter.net/iau/MPCORB/Distant.txt");
	bookmarks[MpcMinorPlanets].insert("MPCORB: other unusual objects", "https://www.minorplanetcenter.net/iau/MPCORB/Unusual.txt");
	bookmarks[MpcMinorPlanets].insert("MPCORB: orbits from the latest DOU MPEC", "https://www.minorplanetcenter.net/iau/MPCORB/DAILY.DAT");
	bookmarks[MpcMinorPlanets].insert("MPCORB: elements of NEAs for current epochs (today)", "https://www.minorplanetcenter.net/iau/MPCORB/NEAm00.txt");
	bookmarks[MpcMinorPlanets].insert("MPCORB: orbits from the latest DOU MPEC", "https://www.minorplanetcenter.net/iau/MPCORB/DAILY.DAT");
	bookmarks[MpcMinorPlanets].insert("MPCAT: Unusual minor planets (including NEOs)", "https://www.minorplanetcenter.net/iau/ECS/MPCAT/unusual.txt");
	bookmarks[MpcMinorPlanets].insert("MPCAT: Distant minor planets (Centaurs and transneptunians)", "https://www.minorplanetcenter.net/iau/ECS/MPCAT/distant.txt");
	int start = 0;
	int finish = 52;
	QString limits, idx;
	for (int i=start; i<=finish; i++)
	{
		limits = QString("%1%2%3").arg(QString::number(i*10000), QChar(0x2014), QString::number(i*10000 + 9999));
		if (i==start)
			limits = QString("%1%2%3").arg(QString::number(1), QChar(0x2014), QString::number(9999));
		else if (i==finish)
			limits = QString("%1...").arg(QString::number(i*10000));

		if ((i+1)<10)
			idx = QString("0%1").arg(i+1);
		else
			idx = QString::number(i+1);
		bookmarks[MpcMinorPlanets].insert(QString("MPCAT: Numbered objects (%1)").arg(limits), QString("http://dss.stellarium.org/MPC/mpn-%1.txt").arg(idx));
	}
	bookmarks[MpcComets].insert("MPC's list of observable comets", "https://www.minorplanetcenter.net/iau/Ephemerides/Comets/Soft00Cmt.txt");
	bookmarks[MpcComets].insert("MPCORB: comets", "https://www.minorplanetcenter.net/iau/MPCORB/CometEls.txt");
	bookmarks[MpcComets].insert("Gideon van Buitenen: comets","http://astro.vanbuitenen.nl/cometelements?format=mpc&mag=obs");

	//Try to save them to a file
	saveBookmarks();
}

void MpcImportWindow::loadBookmarksGroup(QVariantMap source, Bookmarks & bookmarkGroup)
{
	if (source.isEmpty())
		return;

	for (auto title : source.keys())
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
			jsonRoot.insert("version", SOLARSYSTEMEDITOR_PLUGIN_VERSION);

			QVariantMap minorPlanetsObject;
			saveBookmarksGroup(bookmarks[MpcMinorPlanets], minorPlanetsObject);
			//qDebug() << minorPlanetsObject.keys();
			jsonRoot.insert("mpcMinorPlanets", minorPlanetsObject);

			QVariantMap cometsObject;
			saveBookmarksGroup(bookmarks[MpcComets], cometsObject);
			jsonRoot.insert("mpcComets", cometsObject);

			StelJsonParser::write(jsonRoot, &bookmarksFile);
			bookmarksFile.close();

			qDebug() << "Bookmarks file saved to" << QDir::toNativeSeparators(bookmarksFilePath);
			return;
		}
		else
		{
			qDebug() << "Unable to write bookmarks file to" << QDir::toNativeSeparators(bookmarksFilePath);
		}
	}
	catch (std::exception & e)
	{
		qDebug() << "Unable to save bookmarks file:" << e.what();
	}
}

void MpcImportWindow::saveBookmarksGroup(Bookmarks & bookmarkGroup, QVariantMap & output)
{
	for (auto title : bookmarkGroup.keys())
	{
		output.insert(title, bookmarkGroup.value(title));
	}
}
