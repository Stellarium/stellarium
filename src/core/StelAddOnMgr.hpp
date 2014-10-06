/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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
#include <QSqlDatabase>

#include "AOCatalog.hpp"
#include "AOLandscape.hpp"
#include "AOLanguagePack.hpp"
#include "AOScript.hpp"
#include "AOSkyCulture.hpp"
#include "AOTexture.hpp"
#include "StelAddOnDAO.hpp"
#include "addons/AddOn.hpp"

#define ADDON_MANAGER_VERSION "0.0.2"
#define ADDON_MANAGER_CATALOG_VERSION 1

const QString TYPE_CATALOG = "catalog";
const QString TYPE_LANDSCAPE = "landscape";
const QString TYPE_LANGUAGE_PACK = "language_pack";
const QString TYPE_SCRIPT = "script";
const QString TYPE_SKY_CULTURE = "sky_culture";
const QString TYPE_TEXTURE = "texture";

// categories (database column addon.category)
const QString CATEGORY_CATALOG = "catalog";
const QString CATEGORY_LANDSCAPE = "landscape";
const QString CATEGORY_LANGUAGE_PACK = "language_pack";
const QString CATEGORY_SCRIPT = "script";
const QString CATEGORY_SKY_CULTURE = "sky_culture";
const QString CATEGORY_TEXTURE = "texture";

class StelAddOnMgr : public QObject
{
	Q_OBJECT
public:
	StelAddOnMgr();
	virtual ~StelAddOnMgr();

	QString getThumbnailDir() { return m_sThumbnailDir; }
	QString getDirectory(QString category) { return m_dirs.value(category, ""); }
	void installAddOn(const int addonId, const QStringList selectedFiles);
	bool installFromFile(const StelAddOnDAO::AddOnInfo addonInfo,
			     const QStringList selectedFiles);
	void installFromFile(const QString& filePath);
	void removeAddOn(const int addonId, const QStringList files);
	bool isCompatible(QString first, QString last);
	bool updateCatalog(QString webresult);
	void setUpdateFrequencyDays(int days);
	void setUpdateFrequencyHour(int hour);
	void setLastUpdate(qint64 time);
	QString getLastUpdateString()
	{
		return QDateTime::fromMSecsSinceEpoch(m_iLastUpdate*1000)
				.toString("dd MMM yyyy - hh:mm:ss");
	}
	qint64 getLastUpdate() { return m_iLastUpdate; }
	int getUpdateFrequencyDays() { return m_iUpdateFrequencyDays; }
	int getUpdateFrequencyHour() { return m_iUpdateFrequencyHour; }
	QString getUrlForUpdates() { return m_sUrlUpdate; }
	StelAddOnDAO* getStelAddOnDAO() { return m_pStelAddOnDAO; }
	StelAddOn* getStelAddOnInstance(QString category) { return m_pStelAddOns.value(category); }

	//! @enum AddOnMgrMsg
	enum AddOnMgrMsg
	{
		RestartRequired,
		UnableToWriteFiles
	};

	//! @enum AddOnStatus
	//! Used for keeping track of the download/update status
	enum AddOnStatus
	{
		NotInstalled,
		PartiallyInstalled,
		FullyInstalled,
		Installing,
		Corrupted,
		InvalidFormat,
		UnableToWrite,
		UnableToRead,
		UnableToRemove,
		PartiallyRemoved,
		DownloadFailed
	};

signals:
	void addOnMgrMsg(StelAddOnMgr::AddOnMgrMsg);
	void dataUpdated(const QString& category);
	void updateTableViews();
	void skyCulturesChanged();

private slots:
	void downloadThumbnailFinished();
	void downloadAddOnFinished();
	void newDownloadedData();

private:
	QSqlDatabase m_db;
	StelAddOnDAO* m_pStelAddOnDAO;
	QSettings* m_pConfig;

	int m_iDownloadingId;
	QMap<int, QStringList> m_downloadQueue; // <addonId, selectedFiles>
	QNetworkReply* m_pAddOnNetworkReply;
	QFile* m_currentDownloadFile;
	StelAddOnDAO::AddOnInfo m_currentDownloadInfo;

	class StelProgressController* m_progressBar;
	qint64 m_iLastUpdate;
	int m_iUpdateFrequencyDays;
	int m_iUpdateFrequencyHour;
	QString m_sUrlUpdate;

	QNetworkReply* m_pThumbnailNetworkReply;
	QHash<QString, QString> m_thumbnails;
	QStringList m_thumbnailQueue;

	// addon directories
	const QString m_sAddOnDir;
	const QString m_sThumbnailDir;
	QHash<QString, QString> m_dirs;

	typedef QMap<qint64, AddOn*> AddOnMap;
	QString m_sJsonPath;
	QHash<QString, AddOnMap> m_addons;

	// sub-classes
	QHash<QString, StelAddOn*> m_pStelAddOns;

	void restoreDefaultJsonFile();
	void readJsonObject(const QJsonObject& addOns);

	void refreshAddOnStatuses();
	void downloadNextAddOn();
	void finishCurrentDownload();
	void cancelAllDownloads();
	QString calculateMd5(QFile &file) const;

	// download thumbnails
	void downloadNextThumbnail();
};

#endif // _STELADDONMGR_HPP_
