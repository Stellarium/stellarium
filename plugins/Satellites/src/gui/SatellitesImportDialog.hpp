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

#ifndef IMPORTSATELLITESWINDOW_HPP
#define IMPORTSATELLITESWINDOW_HPP

#include "StelDialog.hpp"
#include "Satellites.hpp"

class Ui_satellitesImportDialog;
class QSortFilterProxyModel;
class QStandardItemModel;
class QFile;
class QTemporaryFile;
class QNetworkReply;

class SatellitesImportDialog : public StelDialog
{
	Q_OBJECT
	
public:
	SatellitesImportDialog();
	~SatellitesImportDialog();
	
signals:
	void satellitesAccepted(const TleDataList& newSatellites);
	
public slots:
	void retranslate();
	void setVisible(bool visible = true);
	
private slots:
	void getData();
	void receiveDownload(QNetworkReply* networkReply);
	void abortDownloads();
	void acceptNewSatellites();
	void discardNewSatellites();
	void markAll();
	void markNone();
	
private:
	void createDialogContent();
	Ui_satellitesImportDialog* ui;
	
	void reset();
	void populateList();
	void displayMessage(const QString& message);
	
	//! Set the check state of the currently displayed items.
	//! Note that depending on the search/filter string, these may be
	//! not all available items.
	void setCheckState(Qt::CheckState state);
	
	TleDataHash newSatellites;
	bool isGettingData;
	int numberDownloadsComplete;
	QNetworkAccessManager* downloadMgr;
	QList<QNetworkReply*> activeDownloads;
	QStringList sourceUrls;
	QList<QFile*> sourceFiles;
	QProgressBar* progressBar;
	
	QStandardItemModel* newSatellitesModel;
	QSortFilterProxyModel * filterProxyModel;
};

#endif // IMPORTSATELLITESWINDOW_HPP
