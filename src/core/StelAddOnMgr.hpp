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
#include <QObject>
#include <QSettings>

#include "AddOn.hpp"
#include "DownloadMgr.hpp"

#define ADDON_CONFIG_PREFIX QString("AddOn")
#define ADDON_CATALOG_VERSION 1

class DownloadMgr;

class StelAddOnMgr : public QObject
{
	Q_OBJECT
public:
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

	//! Get instance of DownloadMgr class.
	//! @return DownloadMgr instace
	DownloadMgr* getDownloadMgr() { return m_pDownloadMgr; }

	//! Set the date and time of last update.
	void setLastUpdate(const QDateTime& lastUpdate);
	QDateTime getLastUpdate() { return m_lastUpdate; }

	//! Set the update frequency.
	void setUpdateFrequency(const UpdateFrequency& freq);

	//! Set the URL for downloading the catalog of add-ons.
	void setUrl(const QString& url);
	QString getUrl() { return m_sUrl; }

	QHash<QString, AddOn*> getAddonsAvailable() { return m_addonsAvailable; }
	QHash<QString, AddOn*> getAddonsInstalled() { return m_addonsInstalled; }
	QHash<QString, AddOn*> getAddonsToUpdate() { return m_addonsToUpdate; }

	AddOn* getAddOnFromZip(QString filePath);
	QList<AddOn*> scanFilesInAddOnDir();
	void installAddOnFromFile(QString filePath);
	void installAddOn(AddOn *addon, bool tryDownload = true);
	void removeAddOn(AddOn *addon);
	void installAddons(QSet<AddOn*> addons);
	void removeAddons(QSet<AddOn*> addons);
	QString getLastUpdateString() { return m_lastUpdate.toString("dd MMM yyyy - hh:mm:ss"); }

	UpdateFrequency getUpdateFrequency() { return m_eUpdateFrequency; }
	QString getAddonJsonPath() { return m_sAddonJsonPath; }

	void reloadCatalogues();

signals:
	//! let's the user know that a restart is required.
	void restartRequired();

	//! let's addonMgr aware that table views should be refreshed.
	void updateTableViews();

	void landscapesChanged();
	void scriptsChanged();
	void skyCulturesChanged();

private:
	QSettings* m_pConfig;		//! instace of main config.ini file
	DownloadMgr* m_pDownloadMgr;	//! instance of DownloadMgr class

	QString m_sAddonJsonPath;
	QString m_sInstalledAddonsJsonPath;
	QString m_sUserAddonJsonPath;
	QHash<QString, AddOn*> m_addonsAvailable;
	QHash<QString, AddOn*> m_addonsInstalled;
	QHash<QString, AddOn*> m_addonsToUpdate;

	QDateTime m_lastUpdate;
	UpdateFrequency m_eUpdateFrequency;
	QString m_sUrl;

	QHash<QString, AddOn*> loadAddonCatalog(QString jsonPath) const;
	void restoreDefaultAddonJsonFile();
	void addonToJson(AddOn* addon, QString jsonPath);
	void removeAddonFromJson(AddOn* addon, QString jsonPath);

	void unzip(AddOn& addon);

	//! Load settings from config.ini.
	void loadConfig();

	//! It will let all table views of 'type' know that they need to refresh.
	//! @param type AddOn::Type
	void refreshType(AddOn::Type type);

	//! Calculate the MD5 hash of a given QFile.
	//! @param file QFile
	//! @return md5 hash
	QString calculateMd5(QFile &file) const;
};

#endif // _STELADDONMGR_HPP_
