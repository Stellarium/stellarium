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
	};

	AddOnInfo getAddOnInfo(int addonId);
	bool insertOnDatabase(QString insert);
	void updateInstalledAddon(QString filename,
				  QString installedVersion,
				  QString directory);

private:
	QSqlDatabase m_db;

	bool createAddonTables();
	bool createTableLicense();
	bool createTableAuthor();
};

#endif // _STELADDONDAO_HPP_
