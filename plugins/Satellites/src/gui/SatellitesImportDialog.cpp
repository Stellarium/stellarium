/*
 * Stellarium Satellites Plug-in: satellites import feature
 * Copyright (C) 2012 Bogdan Marinov
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "SatellitesImportDialog.hpp"
#include "ui_satellitesImportDialog.h"

#include "StelApp.hpp"
#include "StelMainView.hpp" //for the QFileDialog? Why?
#include "StelModuleMgr.hpp" // for the GETSTELMODULE macro :(
#include "StelTranslator.hpp"
#include "StelProgressController.hpp"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QNetworkReply>
#include <QProgressBar>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTemporaryFile>
#include <QDir>

SatellitesImportDialog::SatellitesImportDialog()
	: StelDialog("SatellitesImport")
	, isGettingData(false)
	, numberDownloadsComplete(0)
	, downloadMgr(Q_NULLPTR)
	, progressBar(Q_NULLPTR)
	, filterProxyModel(Q_NULLPTR)
{
	ui = new Ui_satellitesImportDialog;
	newSatellitesModel = new QStandardItemModel(this);
}

SatellitesImportDialog::~SatellitesImportDialog()
{
	delete ui;
	
	// Do I need to explicitly delete this?
	if (progressBar)
	{
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = Q_NULLPTR;
	}
	
	if (newSatellitesModel)
	{
		newSatellitesModel->clear();
		delete newSatellitesModel;
		newSatellitesModel = Q_NULLPTR;
	}
}

void SatellitesImportDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void SatellitesImportDialog::setVisible(bool visible)
{
	StelDialog::setVisible(visible);
	if (!isGettingData)
		getData();
}

void SatellitesImportDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// Kinetic scrolling
	kineticScrollingList << ui->listView;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	connect(ui->closeStelWindow, SIGNAL(clicked()),
	        this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)),
		this, SLOT(handleMovedTo(QPoint)));

	connect(ui->pushButtonGetData, SIGNAL(clicked()),
	        this, SLOT(getData()));
	connect(ui->pushButtonAbort, SIGNAL(clicked()),
	        this, SLOT(abortDownloads()));
	connect(ui->pushButtonAdd, SIGNAL(clicked()),
	        this, SLOT(acceptNewSatellites()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()),
	        this, SLOT(discardNewSatellites()));
	connect(ui->pushButtonMarkAll, SIGNAL(clicked()),
	        this, SLOT(markAll()));
	connect(ui->pushButtonMarkNone, SIGNAL(clicked()),
	        this, SLOT(markNone()));
	
	filterProxyModel = new QSortFilterProxyModel(this);
	filterProxyModel->setSourceModel(newSatellitesModel);
	filterProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->listView->setModel(filterProxyModel);
	connect(ui->lineEditSearch, SIGNAL(textChanged(const QString&)),
	        filterProxyModel, SLOT(setFilterFixedString(const QString&)));
	
	reset();
}

void SatellitesImportDialog::getData()
{
	if (isGettingData)
		return;
	isGettingData = true;
	
	if (!downloadMgr)
	{
		downloadMgr = StelApp::getInstance().getNetworkAccessManager();
		connect(downloadMgr, SIGNAL(finished(QNetworkReply*)),
		        this, SLOT(receiveDownload(QNetworkReply*)));
	}
	Satellites* satMgr = GETSTELMODULE(Satellites);
	
	if (satMgr->getUpdatesEnabled())
	{
		sourceUrls = satMgr->getTleSources();		
		qDeleteAll(sourceFiles);
		sourceFiles.clear();
		numberDownloadsComplete = 0;
		
		// Reusing some code from Satellites::updateTLEs()
		if (progressBar == nullptr)
			progressBar = StelApp::getInstance().addProgressBar();
		progressBar->setValue(0);
		progressBar->setRange(0, sourceUrls.size());
		progressBar->setFormat("TLE download %v/%m");
		
		ui->pushButtonGetData->setVisible(false);
		ui->pushButtonAbort->setVisible(true);
		ui->groupBoxWorking->setTitle(q_("Downloading data..."));
		displayMessage(q_("Stellarium is downloading satellite data from the update sources. Please wait..."));
		
		for (int i = 0; i < sourceUrls.size(); i++)
		{
			QString urlData = sourceUrls.at(i);
			QUrl url(urlData.remove("1,", Qt::CaseInsensitive));
			QNetworkReply* reply = downloadMgr->get(QNetworkRequest(url));
			activeDownloads.append(reply);
		}
	}
	else
	{
		QStringList sourceFilePaths;
		// XXX: we should check that there is at least one home location.
		QString homeDirPath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0];
		sourceFilePaths = QFileDialog::getOpenFileNames(
				      Q_NULLPTR,
		                      q_("Select TLE source file(s)..."),
		                      homeDirPath, "*.*");
		if (sourceFilePaths.isEmpty())
			return;
		for (auto filePath : sourceFilePaths)
		{
			QFileInfo fileInfo(filePath);
			if (fileInfo.exists() && fileInfo.isReadable())
			{
				QFile* file = new QFile(filePath);
				sourceFiles.append(file);
			}
		}
		ui->pushButtonGetData->setVisible(false);
		ui->pushButtonAbort->setVisible(false);
		ui->groupBoxWorking->setTitle(q_("Processing data..."));
		displayMessage(q_("Processing data..."));
		populateList();
	}
}

void SatellitesImportDialog::receiveDownload(QNetworkReply* networkReply)
{
	Q_ASSERT(networkReply);
	
	// First, check if this one is one of ours
	QString url = networkReply->request().url().toString();
	if (!activeDownloads.contains(networkReply))
	{
		qDebug() << "Satellites: Received URL not in the source list:" << url;
		return;
	}
	
	// An error is a completed download, isn't it?
	activeDownloads.removeAll(networkReply);
	numberDownloadsComplete++;
	if (progressBar)
		progressBar->setValue(numberDownloadsComplete);
	
	// Then, see if there was an error...
	if (networkReply->error() != QNetworkReply::NoError || networkReply->bytesAvailable()==0)
	{
		qWarning() << "Satellites: failed to download " << url
		           << networkReply->errorString();
		return;
	}
	
	QTemporaryFile* tmpFile = new QTemporaryFile();
	if (tmpFile->open())
	{
		tmpFile->write(networkReply->readAll());
		tmpFile->close();
		sourceFiles.append(tmpFile);
	}
	else
	{
		qWarning() << "Satellites: could not save to file" << url;
	}
	
	if (numberDownloadsComplete >= sourceUrls.count())
	{
		if (progressBar)
		{
			StelApp::getInstance().removeProgressBar(progressBar);
			progressBar = Q_NULLPTR;
		}
		
		if (sourceFiles.isEmpty())
		{
			reset();
			displayMessage(q_("No data could be downloaded. Try again later."));
		}
		else
		{
			ui->pushButtonAbort->setVisible(false);
			ui->groupBoxWorking->setTitle(q_("Processing data..."));
			displayMessage(q_("Processing data..."));
			populateList();
		}
	}
	
	networkReply->deleteLater();
}

void SatellitesImportDialog::abortDownloads()
{
	for (int i = 0; i < activeDownloads.count(); i++)
	{
		activeDownloads[i]->abort();
		activeDownloads[i]->deleteLater();
	}
	reset();
	displayMessage(q_("Download aborted."));
}

void SatellitesImportDialog::acceptNewSatellites()
{
	TleDataList satellitesToAdd;
	for (int row = 0; row < newSatellitesModel->rowCount(); row++)
	{
		QStandardItem* item = newSatellitesModel->item(row);
		if (item->checkState() == Qt::Checked)
		{
			QString id = item->data(Qt::UserRole).toString();
			satellitesToAdd.append(newSatellites.value(id));
		}
	}
	emit satellitesAccepted(satellitesToAdd);
	reset();
	close();
}

void SatellitesImportDialog::discardNewSatellites()
{
	reset();
	close();
}

void SatellitesImportDialog::markAll()
{
	setCheckState(Qt::Checked);
}

void SatellitesImportDialog::markNone()
{
	setCheckState(Qt::Unchecked);
}

void SatellitesImportDialog::reset()
{
	// Assuming that everything that needs to be stopped is stopped
	isGettingData = false;
	ui->stackedWidget->setCurrentIndex(0);
	ui->pushButtonGetData->setVisible(true);
	ui->pushButtonAbort->setVisible(false);
	ui->labelMessage->setVisible(false);
	ui->labelMessage->clear();
	ui->groupBoxWorking->setTitle(q_("Get data"));
	newSatellitesModel->clear();
	ui->lineEditSearch->clear();
	
	newSatellites.clear();
	sourceUrls.clear();
	
	qDeleteAll(activeDownloads);
	activeDownloads.clear();
	
	qDeleteAll(sourceFiles);
	sourceFiles.clear();
	
	numberDownloadsComplete = 0;
	if (progressBar)
	{
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = Q_NULLPTR;
	}
}

void SatellitesImportDialog::populateList()
{
	newSatellites.clear();
	newSatellitesModel->clear();
	Satellites* satMgr = GETSTELMODULE(Satellites);
	
	// Load ALL two-line element sets...
	for (int f = 0; f < sourceFiles.count(); f++)
	{
		bool open = false;
		QTemporaryFile* tempFile = dynamic_cast<QTemporaryFile*>(sourceFiles[f]);
		if (tempFile)
			open = tempFile->open();
		else
			open = sourceFiles[f]->open(QFile::ReadOnly);
		if (open)
		{
			satMgr->parseTleFile(*sourceFiles[f], newSatellites);
			sourceFiles[f]->close();
		}
		else
		{
			qDebug() << "Satellites: cannot open file"
				 << QDir::toNativeSeparators(sourceFiles[f]->fileName());
		}
	}
	// Clear the disk...
	qDeleteAll(sourceFiles);
	sourceFiles.clear();
	
	QStringList existingIDs = satMgr->listAllIds();
	QHashIterator<QString,TleData> i(newSatellites);
	while (i.hasNext())
	{
		i.next();
		
		// Skip duplicates
		if (existingIDs.contains(i.key()))
			continue;
		
		TleData tle = i.value();		
		QStandardItem* newItem = new QStandardItem(QString("%1 (NORAD %2)").arg(tle.name, tle.id)); // Available to search via NORAD number
		newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
		newItem->setCheckState(Qt::Unchecked);
		newItem->setData(tle.id, Qt::UserRole);		
		newItem->setToolTip(QString(q_("Catalog Number: %1")).arg(tle.id));
		newSatellitesModel->appendRow(newItem);
	}
	existingIDs.clear();
	newSatellitesModel->sort(0);
	ui->listView->scrollToTop();
	ui->stackedWidget->setCurrentIndex(1);
}

void SatellitesImportDialog::displayMessage(const QString& message)
{
	if (message.isEmpty())
		return;
	
	ui->labelMessage->setText(message);
	ui->labelMessage->setVisible(true);
}

void SatellitesImportDialog::setCheckState(Qt::CheckState state)
{
	Q_ASSERT(filterProxyModel);
	
	int rowCount = filterProxyModel->rowCount();
	if (rowCount < 1)
		return;

	for (int row = 0; row < rowCount; row++)
	{
		QModelIndex proxyIndex = filterProxyModel->index(row, 0);
		QModelIndex index = filterProxyModel->mapToSource(proxyIndex);
		QStandardItem * item = newSatellitesModel->itemFromIndex(index);
		if (item)
		{
			item->setCheckState(state);
		}
	}
}
