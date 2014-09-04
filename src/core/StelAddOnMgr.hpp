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

// categories (database column addon.category)
const QString CATALOG = "catalog";
const QString LANDSCAPE = "landscape";
const QString LANGUAGE_PACK = "language_pack";
const QString SCRIPT = "script";
const QString SKY_CULTURE = "sky_culture";
const QString TEXTURE = "texture";

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
	void setLastUpdate(qint64 time);
	QString getLastUpdateString()
	{
		return QDateTime::fromMSecsSinceEpoch(m_iLastUpdate*1000)
				.toString("dd MMM yyyy - hh:mm:ss");
	}
	qint64 getLastUpdate() { return m_iLastUpdate; }
	QString getUrlForUpdates() { return m_sUrlUpdate; }
	StelAddOnDAO* getStelAddOnDAO() { return m_pStelAddOnDAO; }
	StelAddOn* getStelAddOnInstance(QString category) { return m_pStelAddOns.value(category); }

signals:
	void updateTableViews();
	void skyCulturesChanged();

private slots:
	void downloadThumbnailFinished();
	void downloadAddOnFinished();
	void newDownloadedData();

private:
	//! @enum AddOnStatus
	//! Used for keeping track of the download/update status
	enum AddOnStatus
	{
		NotInstalled,
		PartiallyInstalled,
		FullyInstalled,
		Installing
	};

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
	QString m_sUrlUpdate;

	QNetworkReply* m_pThumbnailNetworkReply;
	QHash<QString, QString> m_thumbnails;
	QStringList m_thumbnailQueue;

	// addon directories
	const QString m_sAddOnDir;
	const QString m_sThumbnailDir;
	QHash<QString, QString> m_dirs;

	// sub-classes
	QHash<QString, StelAddOn*> m_pStelAddOns;

	void refreshAddOnStatuses();
	void downloadNextAddOn();
	void finishCurrentDownload();
	void cancelAllDownloads();
	QString calculateMd5(QFile &file) const;

	// download thumbnails
	void downloadNextThumbnail();
};

#endif // _STELADDONMGR_HPP_
