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
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QObject>
#include <QSqlDatabase>
#include <QUrl>

#include "StelAddOnDAO.hpp"

// categories (database column addon.category)
const QString LANDSCAPE = "landscape";
const QString LANGUAGE_PACK = "language_pack";
const QString PLUGIN_CATALOG = "plugin_catalog";
const QString SCRIPT = "script";
const QString STAR_CATALOG = "star_catalog";
const QString STARLORE = "starlore";
const QString TEXTURE = "texture";

class StelAddOnMgr : public QObject
{
	Q_OBJECT
public:
	StelAddOnMgr();

	void installAddOn(const int addonId);
	void installFromFile(QString category, QString filePath);
	void removeAddOn(const int addonId);
	bool updateDatabase(QString webresult);
	void setLastUpdate(qint64 time);
	QString getLastUpdateString()
	{
		return QDateTime::fromMSecsSinceEpoch(m_iLastUpdate*1000)
				.toString("dd MMM yyyy - hh:mm:ss");
	}
	qint64 getLastUpdate()
	{
		return m_iLastUpdate;
	}
signals:
	void updateTableViews();

private slots:
	void downloadFinished();
	void newDownloadedData();

private:
	//! @enum InstallState
	//! Used for keeping track of the download/update status
	enum InstallState
	{
		Installed,
		Complete,
		CompleteUpdates,
		DownloadError,
		OtherError
	};

	struct AddOnInfo {
		QString category;
		double downloadSize;
		QString filename;
		QString filepath;
		QUrl url;
		QDir installedDir;
	};

	QSqlDatabase m_db;
	StelAddOnDAO* m_pStelAddOnDAO;

	bool m_bDownloading;
	QList<int> m_downloadQueue;
	QNetworkReply* m_pDownloadReply;
	QFile* m_currentDownloadFile;
	QString m_currentDownloadPath;
	QString m_currentDownloadCategory;

	class StelProgressController* m_progressBar;
	QString m_sAddonPath;
	qint64 m_iLastUpdate;

	const QString dirAddOn;
	const QString dirCatalog;
	const QString dirLandscape;
	const QString dirLanguagePack;
	const QString dirScript;
	const QString dirStarlore;
	const QString dirTexture;

	AddOnInfo getAddOnInfo(int addonId);
	QString getDirectory(QString category);

	void checkInstalledAddOns();
	void downloadAddOn(const AddOnInfo addonInfo);
	void updateInstalledAddon(QString filename,
				  QString installedVersion,
				  QString directory);
};

#endif // _STELADDONMGR_HPP_
