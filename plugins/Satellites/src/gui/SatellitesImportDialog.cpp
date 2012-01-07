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
#include "ui_importSatellitesWindow.h"

#include "StelApp.hpp"
#include "StelModuleMgr.hpp" // for the GETSTELMODULE macro :(

#include <QNetworkReply>
#include <QTemporaryFile>

SatellitesImportDialog::SatellitesImportDialog() :
    downloadMgr(0),
    progressBar(0)
{
	ui = new Ui_satellitesImportDialog;
}

SatellitesImportDialog::~SatellitesImportDialog()
{
	delete ui;
	
	// Do I need to explicitly delete this?
	if (progressBar)
	{
		delete progressBar;
		progressBar = 0;
	}
}

void SatellitesImportDialog::languageChanged()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void SatellitesImportDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	connect(ui->closeStelWindow, SIGNAL(clicked()),
	        this, SLOT(close()));
	
	connect(ui->pushButtonGetData, SIGNAL(clicked()),
	        this, SLOT(getData()));
	connect(ui->pushButtonAdd, SIGNAL(clicked()),
	        this, SLOT(acceptNewSatellites()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()),
	        this, SLOT(discardNewSatellites()));
	
	reset();
}

void SatellitesImportDialog::getData()
{
	if (!downloadMgr)
	{
		downloadMgr = StelApp::getInstance().getNetworkAccessManager();
		connect(downloadMgr, SIGNAL(finished(QNetworkReply*)),
		        this, SLOT(receiveDownload(QNetworkReply*)));
	}
	
	Satellites* satMgr = GETSTELMODULE(Satellites);
	sourceUrls = satMgr->getTleSources();
	qDeleteAll(sourceFiles);
	sourceFiles.clear();
	numberDownloadsComplete = 0;
	
	// Reusing some code from Satellites::updateTLEs()
	if (progressBar == 0)
		progressBar = StelApp::getInstance().getGui()->addProgressBar();
	progressBar->setValue(0);
	progressBar->setMaximum(sourceUrls.size());
	progressBar->setVisible(true);
	progressBar->setFormat("TLE download %v/%m");
	
	for (int i = 0; i < sourceUrls.size(); i++)
	{
		QUrl url(sourceUrls.at(i));
		downloadMgr->get(QNetworkRequest(url));
	}
}

void SatellitesImportDialog::receiveDownload(QNetworkReply* networkReply)
{
	Q_ASSERT(networkReply);
	
	// First, check if this one is one of ours
	QString url = networkReply->request().url().toString();
	if (!sourceUrls.contains(url))
	{
		qDebug() << "Received URL not in the source list:" << url;
		return;
	}
	
	// An error is a completed download, isn't it?
	numberDownloadsComplete++;
	if (progressBar)
		progressBar->setValue(numberDownloadsComplete);
	
	// Then, see if there was an error...
	if (networkReply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Satellites: failed to download" << url 
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
		ui->pushButtonGetData->setVisible(false);
		if (progressBar)
		{
			delete progressBar;
			progressBar = 0;
		}
		populateList();
	}
	
	networkReply->deleteLater();
}

void SatellitesImportDialog::acceptNewSatellites()
{
	TleDataList satellitesToAdd;
	for (int i = 0; i < ui->listWidget->count(); i++)
	{
		QListWidgetItem* item = ui->listWidget->item(i);
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

void SatellitesImportDialog::reset()
{
	//Assuming that everything that needs to be stopped is stopped
	ui->pushButtonGetData->setVisible(true);
	ui->groupBoxResults->setVisible(false);
	ui->listWidget->clear();
	
	newSatellites.clear();
	sourceUrls.clear();
	
	qDeleteAll(sourceFiles);
	sourceFiles.clear();
	
	numberDownloadsComplete = 0;
	if (progressBar)
	{
		delete progressBar;
		progressBar = 0;
	}
}

void SatellitesImportDialog::populateList()
{
	newSatellites.clear();
	ui->listWidget->clear();
	Satellites* satMgr = GETSTELMODULE(Satellites);
	
	// Load ALL two-line element sets...
	for (int f = 0; f < sourceFiles.count(); f++)
	{
		QTemporaryFile* file = sourceFiles[f];
		if (file->open())
		{
			satMgr->parseTleFile(*file, newSatellites);
			file->close();
		}
		else
		{
			qDebug() << "Satellites: skipping temporary file"
			         << file->fileName();
		}
	}
	// Clear the disk...
	qDeleteAll(sourceFiles);
	sourceFiles.clear();
	
	QStringList existingIDs = satMgr->getAllIDs();
	QHashIterator<QString,TleData> i(newSatellites);
	while (i.hasNext())
	{
		i.next();
		
		// Skip duplicates
		if (existingIDs.contains(i.key()))
			continue;
		
		TleData tle = i.value();
		QListWidgetItem* newItem = new QListWidgetItem(tle.name);
		newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
		newItem->setCheckState(Qt::Unchecked);
		newItem->setData(Qt::UserRole, tle.id);
		QString text = QString("Catalog Number: %1").arg(tle.id);
		newItem->setToolTip(text);
		ui->listWidget->addItem(newItem);
	}
	existingIDs.clear();
	ui->listWidget->sortItems();
	ui->groupBoxResults->setVisible(true);
}
