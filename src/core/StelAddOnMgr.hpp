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

#include "AOCatalog.hpp"
#include "AOLandscape.hpp"
#include "AOLanguagePack.hpp"
#include "AOScript.hpp"
#include "AOSkyCulture.hpp"
#include "AOTexture.hpp"
#include "addons/AddOn.hpp"

#define ADDON_MANAGER_VERSION "0.0.2"
#define ADDON_MANAGER_CATALOG_VERSION 1

class StelAddOnMgr : public QObject
{
	Q_OBJECT
public:
	typedef QMap<QString, AddOn*> AddOnMap;

	//! @enum AddOnMgrMsg
	enum AddOnMgrMsg
	{
		RestartRequired,
		UnableToWriteFiles
	};

	StelAddOnMgr();
	virtual ~StelAddOnMgr();

	AddOnMap getAddOnMap(AddOn::Type type) { return m_addons.value(type); }
	QHash<AddOn::Type, AddOnMap> getAddOnHash() { return m_addons; }
	QString getAddOnDir() { return m_sAddOnDir; }
	QString getThumbnailDir() { return m_sThumbnailDir; }
	void installAddOn(AddOn *addon, const QStringList selectedFiles);
	AddOn::Status installFromFile(AddOn* addon, const QStringList selectedFiles);
	void installFromFile(const QString& filePath);
	void removeAddOn(AddOn *addon, const QStringList files);
	bool isCompatible(QString first, QString last);
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
	StelAddOn* getStelAddOnInstance(AddOn::Category c) { return m_pStelAddOns.value(c); }
	QString getAddonJsonPath() { return m_sAddonJsonPath; }
	void reloadAddonJsonFile();

signals:
	void addOnMgrMsg(StelAddOnMgr::AddOnMgrMsg);
	void dataUpdated(AddOn* addon);
	void updateTableViews();
	void scriptsChanged();
	void skyCulturesChanged();

private slots:
	void downloadThumbnailFinished();
	void downloadAddOnFinished();
	void newDownloadedData();

private:
	QSettings* m_pConfig;

	AddOn* m_downloadingAddOn;
	QMap<AddOn*, QStringList> m_downloadQueue;
	QNetworkReply* m_pAddOnNetworkReply;
	QFile* m_currentDownloadFile;

	// addon directories
	const QString m_sAddOnDir;
	const QString m_sThumbnailDir;

	// sub-classes
	QHash<AddOn::Category, StelAddOn*> m_pStelAddOns;

	QString m_sAddonJsonFilename;
	QString m_sAddonJsonPath;
	QHash<AddOn::Type, AddOnMap> m_addons;
	QHash<QString, AddOn*> m_addonsByMd5;
	QHash<QString, AddOn*> m_addonsById;

	QNetworkReply* m_pThumbnailNetworkReply;
	QHash<QString, QString> m_thumbnails;
	QStringList m_thumbnailQueue;

	class StelProgressController* m_progressBar;
	qint64 m_iLastUpdate;
	int m_iUpdateFrequencyDays;
	int m_iUpdateFrequencyHour;
	QString m_sUrlUpdate;

	void refreshAddOnStatuses();
	void refreshThumbnailQueue();

	void restoreDefaultAddonJsonFile();
	void readAddonJsonObject(const QJsonObject& addOns);
	void appendAddonToInstalledJson(AddOn* addon);

	void downloadNextAddOn();
	void finishCurrentDownload();
	void cancelAllDownloads();
	QString calculateMd5(QFile &file) const;

	// download thumbnails
	void downloadNextThumbnail();

	AddOn::Status unzip(AddOn* addon, QStringList selectedFiles);
};

#endif // _STELADDONMGR_HPP_
