/*
 * Stellarium
 * Copyright (C) 2014-2016 Marcos Cardinot
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

#ifndef _STELADDONMGR_HPP_
#define _STELADDONMGR_HPP_

#include <QDateTime>
#include <QFile>
#include <QNetworkReply>
#include <QObject>
#include <QSettings>

#include "AddOn.hpp"

#define ADDON_MANAGER_CATALOG_VERSION 1

class StelAddOnMgr : public QObject
{
	Q_OBJECT
public:
	//! @enum AddOnMgrMsg
	enum AddOnMgrMsg
	{
		RestartRequired,
		UnableToWriteFiles
	};

	//! @enum update frequency for add-ons catalog
	enum UpdateFrequency
	{
		NEVER,
		ON_STARTUP,
		EVERY_DAY,
		EVERY_THREE_DAYS,
		EVERY_WEEK
	};

	StelAddOnMgr();
	virtual ~StelAddOnMgr();

	QHash<QString, AddOn*> getAddonsAvailable() { return m_addonsAvailable; }
	QHash<QString, AddOn*> getAddonsInstalled() { return m_addonsInstalled; }
	QHash<QString, AddOn*> getAddonsToUpdate() { return m_addonsToUpdate; }
	QString getAddOnDir() { return m_sAddOnDir; }
	QString getThumbnailDir() { return m_sThumbnailDir; }
	AddOn* getAddOnFromZip(QString filePath);
	QList<AddOn*> scanFilesInAddOnDir();
	void installAddOnFromFile(QString filePath);
	void installAddOn(AddOn *addon, bool tryDownload = true);
	void removeAddOn(AddOn *addon);
	void updateAddons(QSet<AddOn *> addons);
	void installAddons(QSet<AddOn*> addons);
	void removeAddons(QSet<AddOn*> addons);
	void setUpdateFrequency(UpdateFrequency freq);
	void setLastUpdate(QDateTime lastUpdate);
	QString getLastUpdateString() { return m_lastUpdate.toString("dd MMM yyyy - hh:mm:ss"); }
	QDateTime getLastUpdate() { return m_lastUpdate; }
	UpdateFrequency getUpdateFrequency() { return m_eUpdateFrequency; }
	QString getUrlForUpdates() { return m_sUrlUpdate; }
	QString getAddonJsonPath() { return m_sAddonJsonPath; }

	QByteArray getUserAgent() { return m_userAgent; }

	void reloadCatalogues();

signals:
	void addOnMgrMsg(StelAddOnMgr::AddOnMgrMsg);
	void dataUpdated(AddOn* addon);
	void updateTableViews();
	void landscapesChanged();
	void scriptsChanged();
	void skyCulturesChanged();

private slots:
	void slotDataUpdated(AddOn* addon);
	void downloadThumbnailFinished();
	void downloadAddOnFinished();
	void newDownloadedData();

private:
	QSettings* m_pConfig;

	AddOn* m_downloadingAddOn;
	QList<AddOn*> m_downloadQueue;
	QNetworkReply* m_pAddOnNetworkReply;
	QFile* m_currentDownloadFile;
	QByteArray m_userAgent;

	// addon directories
	const QString m_sAddOnDir;
	const QString m_sThumbnailDir;

	QString m_sAddonJsonFilename;
	QString m_sAddonJsonPath;
	QString m_sUserAddonJsonPath;
	QString m_sInstalledAddonsJsonPath;
	QHash<QString, AddOn*> m_addonsAvailable;
	QHash<QString, AddOn*> m_addonsInstalled;
	QHash<QString, AddOn*> m_addonsToUpdate;

	QNetworkReply* m_pThumbnailNetworkReply;
	QHash<QString, QString> m_thumbnails;
	QStringList m_thumbnailQueue;

	class StelProgressController* m_progressBar;
	QDateTime m_lastUpdate;
	UpdateFrequency m_eUpdateFrequency;
	QString m_sUrlUpdate;

	void refreshThumbnailQueue();

	QHash<QString, AddOn*> loadAddonCatalog(QString jsonPath) const;
	void restoreDefaultAddonJsonFile();
	void insertAddonInJson(AddOn* addon, QString jsonPath);
	void removeAddonFromJson(AddOn* addon, QString jsonPath);

	void downloadNextAddOn();
	void finishCurrentDownload();
	void cancelAllDownloads();
	QString calculateMd5(QFile &file) const;

	// download thumbnails
	void downloadNextThumbnail();

	void unzip(AddOn& addon);
};

#endif // _STELADDONMGR_HPP_
