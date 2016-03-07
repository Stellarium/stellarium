/*
 * Stellarium
 * Copyright (C) 2016 Marcos Cardinot
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

#ifndef _DOWNLOADMGR_HPP_
#define _DOWNLOADMGR_HPP_

#include <QFile>
#include <QNetworkReply>
#include <QObject>

#include "AddOn.hpp"
#include "StelAddOnMgr.hpp"
#include "StelProgressController.hpp"

class StelAddOnMgr;

class DownloadMgr : public QObject
{
	Q_OBJECT
public:
	DownloadMgr(StelAddOnMgr* mgr);
	virtual ~DownloadMgr();

	//! Append the add-on in the download queue.
	//! @param addon Add-on
	void download(AddOn* addon);

	//! Check if the add-on is in the queue for downloads.
	//! @param addon Add-on
	//! @return true if it's in the queue
	bool isDownloading(AddOn* addon);

signals:
	void updateTableViews();

public slots:
	//! Update catalog of add-ons.
	void updateCatalog();

private slots:
	//! Slot for readyRead() signal, which is emitted once
	//! every time new data is available for reading.
	void newDataAvailable();

	//! Triggered when the download is complete.
	void downloadFinished();

	//! Check if it's time to update the catalog of add-ons.
	//! This function is triggered automatically by a QTimer.
	void checkInterval();

private:
	StelAddOnMgr* m_pMgr;			//! instace of add-on mgr class (parent).

	QNetworkRequest m_networkRequest;	//! Network request.
	QNetworkReply* m_networkReply;		//! Network reply.
	StelProgressController* m_progressBar;	//! Progress bar.

	AddOn* m_downloadingAddOn;		//! Add-on that is currently being downloaded.
	QFile* m_currentDownloadFile;		//! QFile of current download.
	QList<AddOn*> m_downloadQueue;		//! List of add-ons queued for download.

	//! Try to download the next add-on in the queue.
	void downloadNextAddOn();

	//! Finish the current download.
	void finishCurrentDownload();

	//! Clear queue annd cancell any current download.
	void cancelAllDownloads();
};

#endif // _DOWNLOADMGR_HPP_
