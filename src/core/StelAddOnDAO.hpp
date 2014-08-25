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

#ifndef _STELADDONDAO_HPP_
#define _STELADDONDAO_HPP_

#include <QDir>
#include <QSqlDatabase>
#include <QUrl>

// database table names
const QString TABLE_AUTHOR = "author";
const QString TABLE_LICENSE = "license";
const QString TABLE_ADDON = "addon";
const QString TABLE_CATALOG = "catalog";
const QString TABLE_LANDSCAPE = "landscape";
const QString TABLE_LANGUAGE_PACK = "language_pack";
const QString TABLE_PLUGIN_CATALOG = "plugin_catalog";
const QString TABLE_SCRIPT = "script";
const QString TABLE_STAR_CATALOG = "star_catalog";
const QString TABLE_SKY_CULTURE = "sky_culture";
const QString TABLE_TEXTURE = "texture";

// database column names
const QString COLUMN_ID = "id";
const QString COLUMN_ID_INSTALL = "id_install";
const QString COLUMN_CATEGORY = "category";
const QString COLUMN_TITLE = "title";
const QString COLUMN_DESCRIPTION = "description";
const QString COLUMN_VERSION = "version";
const QString COLUMN_FIRST_STEL = "first_stel";
const QString COLUMN_LAST_STEL = "last_stel";
const QString COLUMN_AUTHOR1 = "author1";
const QString COLUMN_AUTHOR2 = "author2";
const QString COLUMN_LICENSE = "license";
const QString COLUMN_DOWNLOAD_URL = "download_url";
const QString COLUMN_DOWNLOAD_FILENAME = "download_filename";
const QString COLUMN_DOWNLOAD_SIZE = "download_size";
const QString COLUMN_CHECKSUM = "checksum";
const QString COLUMN_LAST_UPDATE = "last_update";
const QString COLUMN_INSTALLED = "installed";

const QString COLUMN_ADDONID = "addon";
const QString COLUMN_TYPE = "type";
const QString COLUMN_CATALOG_ID = "catalog";
const QString COLUMN_COUNT = "count";
const QString COLUMN_MINMAG = "min_mag";
const QString COLUMN_MAXMAG = "max_mag";
const QString COLUMN_THUMBNAIL = "thumbnail";
const QString COLUMN_TEXTURES = "textures";
const QString COLUMN_INSTALLED_TEXTURES = "installed_textures";


class StelAddOnDAO
{
public:
	StelAddOnDAO(QSqlDatabase database);
	~StelAddOnDAO();

	// Init database
	bool init();

	struct AddOnInfo {
		QString idInstall;
		QString category;
		QUrl url;
		float size;
		bool installed;
		QString filename;
		QString filepath;
		QString checksum;
	};

	struct WidgetInfo {
		QString idInstall;
		QString description;
		QString downloadSize;
		QString licenseName;
		QString licenseUrl;
		QString a1Name;
		QString a1Email;
		QString a1Url;
		QString a2Name;
		QString a2Email;
		QString a2Url;
	};

	AddOnInfo getAddOnInfo(int addonId);
	WidgetInfo getAddOnWidgetInfo(int addonId);
	bool insertOnDatabase(QString insert);
	void markAllAddOnsAsUninstalled();
	void markAddOnsAsInstalled(QStringList idInstall);
	void markAddOnsAsInstalledFromMd5(QStringList checksums);
	void markTexturesAsInstalled(const QStringList& items);
	void updateAddOnStatus(QString idInstall, int installed);

	QHash<QString, QString> getThumbnails();
	QStringList getListOfTextures(int addonId);
	QString getLanguagePackType(const QString& checksum);

private:
	QSqlDatabase m_db;

	bool createAddonTables();
	bool createTableLicense();
	bool createTableAuthor();
};

#endif // _STELADDONDAO_HPP_
