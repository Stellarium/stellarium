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

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringBuilder>

#include "StelAddOnDAO.hpp"
#include "StelAddOnMgr.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"

StelAddOnDAO::StelAddOnDAO(QSqlDatabase database)
	: m_db(database)
{
}

bool StelAddOnDAO::init()
{
	// Init database
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString addonPath = StelFileMgr::findFile("addon/", flags);
	m_db.setHostName("localhost");
	m_db.setDatabaseName(addonPath % "addon.sqlite");
	bool ok = m_db.open();
	qDebug() << "Add-On Database status:" << m_db.databaseName() << "=" << ok;
	if (m_db.lastError().isValid())
	{
	    qWarning() << "Error loading Add-On database:" << m_db.lastError();
	    return false;
	}

	// creating tables
	if (!createAddonTables() ||
	    !createTableLicense() ||
	    !createTableAuthor())
	{
		qWarning() << "Add-On DAO : Failed to create tables!" << m_db.lastError();
		return false;
	}

	return true;
}

bool StelAddOnDAO::createAddonTables()
{
	QStringList addonTables;

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_ADDON % " ("
			% COLUMN_ID % " INTEGER primary key AUTOINCREMENT, "
			% COLUMN_ID_INSTALL % " TEXT, "
			% COLUMN_CATEGORY % " TEXT, "
			% COLUMN_TITLE % " TEXT, "
			% COLUMN_DESCRIPTION % " TEXT, "
			% COLUMN_VERSION % " TEXT, "
			% COLUMN_FIRST_STEL % " TEXT, "
			% COLUMN_LAST_STEL % " TEXT, "
			% COLUMN_AUTHOR1 % " INTEGER, "
			% COLUMN_AUTHOR2 % " INTEGER, "
			% COLUMN_LICENSE % " INTEGER, "
			% COLUMN_DOWNLOAD_URL % " TEXT, "
			% COLUMN_DOWNLOAD_FILENAME % " TEXT, "
			% COLUMN_DOWNLOAD_SIZE % " TEXT, "
			% COLUMN_CHECKSUM % " TEXT, "
			% COLUMN_LAST_UPDATE % " TEXT, "
			% COLUMN_INSTALLED % " INT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_CATALOG % " ("
			% COLUMN_ID % " INTEGER primary key AUTOINCREMENT, "
			% COLUMN_ADDONID % " INTEGER UNIQUE, "
			% COLUMN_TYPE % " TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_PLUGIN_CATALOG % " ("
			% COLUMN_ID % " INTEGER primary key AUTOINCREMENT, "
			% COLUMN_CATALOG_ID % " INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_STAR_CATALOG % " ("
			% COLUMN_ID % " INTEGER primary key AUTOINCREMENT, "
			% COLUMN_CATALOG_ID % " INTEGER UNIQUE, "
			% COLUMN_COUNT % " INTEGER, "
			% COLUMN_MINMAG % " FLOAT, "
			% COLUMN_MAXMAG % " FLOAT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_LANDSCAPE % " ("
			% COLUMN_ID % " INTEGER primary key AUTOINCREMENT, "
			% COLUMN_ADDONID % " INTEGER UNIQUE, "
			% COLUMN_THUMBNAIL % " TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_LANGUAGE_PACK % " ("
		       % COLUMN_ID % " INTEGER primary key AUTOINCREMENT, "
		       % COLUMN_ADDONID % " INTEGER UNIQUE, "
		       % COLUMN_TYPE % " TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_SCRIPT % " ("
			% COLUMN_ID % " INTEGER primary key AUTOINCREMENT, "
			% COLUMN_ADDONID % " INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_SKY_CULTURE % " ("
			% COLUMN_ID % " INTEGER primary key AUTOINCREMENT, "
			% COLUMN_ADDONID % " INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_TEXTURE % " ("
			% COLUMN_ID % " INTEGER primary key AUTOINCREMENT, "
			% COLUMN_ADDONID % " INTEGER UNIQUE, "
			% COLUMN_TEXTURES % " TEXT, "
			% COLUMN_INSTALLED_TEXTURES % " TEXT DEFAULT 0)";

	QSqlQuery query(m_db);
	foreach (QString table, addonTables)
	{
		query.prepare(table);
		if (!query.exec())
		{
			qDebug() << "Add-On DAO : unable to create the addon table."
				 << m_db.lastError();
			return false;
		}
	}
	return true;
}

bool StelAddOnDAO::createTableLicense()
{
	QSqlQuery query(m_db);
	query.prepare(
		"CREATE TABLE IF NOT EXISTS " % TABLE_LICENSE % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"name TEXT, "
			"url TEXT)"
	);
	if (!query.exec())
	{
		qDebug() << "Add-On DAO : unable to create the license table."
			 << m_db.lastError();
		return false;
	}
	return true;
}

bool StelAddOnDAO::createTableAuthor()
{
	QSqlQuery query(m_db);
	query.prepare(
		"CREATE TABLE IF NOT EXISTS " % TABLE_AUTHOR % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"name TEXT, "
			"email TEXT, "
			"url TEXT)"
	);
	if (!query.exec())
	{
		qDebug() << "Add-On DAO : unable to create the author table."
			 << m_db.lastError();
		return false;
	}
	return true;
}

bool StelAddOnDAO::insertOnDatabase(QString insert)
{
	QSqlQuery query(m_db);
	query.prepare(insert);
	if (!query.exec())
	{
		qWarning() << "Add-On DAO : unable to update database."
			   << m_db.lastError() << insert;
		return false;
	}

	return true;
}

void StelAddOnDAO::markAllAddOnsAsUninstalled()
{
	QSqlQuery query(m_db);
	query.prepare("UPDATE addon SET installed=0");
	if (!query.exec()) {
		qWarning() << "Add-On DAO : Could not mark all add-ons as uninstalled!";
	}
}

void StelAddOnDAO::markAddOnsAsInstalled(QStringList idInstall)
{
	if (idInstall.isEmpty())
	{
		return;
	}
	QSqlQuery query(m_db);
	QString sQuery = QString("UPDATE addon SET installed=2 "
				 "WHERE id_install IN ('%1')").arg(idInstall.join("','"));
	if (!query.exec(sQuery)) {
		qWarning() << "Add-On DAO : Could not mark add-ons as installed!"
			   << m_db.lastError() << sQuery;
	}
}

void StelAddOnDAO::markAddOnsAsInstalledFromMd5(QStringList checksums)
{
	if (checksums.isEmpty())
	{
		return;
	}
	QSqlQuery query(m_db);
	QString sQuery = QString("UPDATE addon SET installed=2 "
				 "WHERE checksum IN ('%1')").arg(checksums.join("','"));
	if (!query.exec(sQuery)) {
		qWarning() << "Add-On DAO : Could not mark add-ons as installed (from md5)!"
			   << m_db.lastError() << sQuery;
	}
}

void StelAddOnDAO::markTexturesAsInstalled(const QStringList& items)
{
	if (items.isEmpty())
	{
		return;
	}

	// get installIds
	QStringList installIds;
	foreach (QString item, items) {
		QString installId = item.split("/")[0];
		if (!installIds.contains(installId))
			installIds.append(installId);
	}

	// get list of textures from database by each installId
	QSqlQuery query(m_db);
	QString sQuery = QString("SELECT addon, id_install, textures "
				 "FROM texture INNER JOIN addon "
				 "ON texture.addon = addon.id "
				 "WHERE id_install IN ('%1')").arg(installIds.join("','"));
	if (!query.exec(sQuery)) {
		qWarning() << "Add-On DAO : Could not get textures from list of id_install!"
			   << m_db.lastError() << sQuery;
		return;
	}
	QStringList installedTextures;
	while (query.next())
	{
		int addonId = query.value(0).toInt();
		QString idInstall = query.value(1).toString();
		QStringList textures = query.value(2).toString().split(",");
		int countItems = 0;
		int countInstalled = 0;
		foreach (QString texture, textures) {
			countItems++;
			QString item = idInstall % "/" % texture;
			if (items.contains(item)) // texture installed
			{
				countInstalled++;
				installedTextures.append("2");
				continue;
			}
			// texture not installed
			installedTextures.append("0");
		}

		// get addon status (texture set)
		int installed;
		if (countInstalled == 0)
			installed = 0; // not installed
		else if (countInstalled < countItems)
			installed = 1; // partial
		else
			installed = 2; // all textures installed

		// update database
		updateAddOnStatus(idInstall, installed);
		updateTextureStatus(addonId, installedTextures.join(","));
	}
}

void StelAddOnDAO::updateAddOnStatus(QString idInstall, int installed)
{
	 QSqlQuery query(m_db);
	 query.prepare("UPDATE addon "
		       "SET installed=:installed "
		       "WHERE id_install=:id_install");
	 query.bindValue(":installed", installed);
	 query.bindValue(":id_install", idInstall);

	 if (!query.exec()) {
		qWarning() << "Add-On DAO : Unable to update addon status!"
			   << m_db.lastError();
	}
}

void StelAddOnDAO::updateTextureStatus(int addonId, QString installedTextures)
{
	 QSqlQuery query(m_db);
	 QString sQuery = QString("UPDATE texture "
				  "SET installed_textures='%1' "
				  "WHERE addon=%2")
			 .arg(installedTextures)
			 .arg(addonId);
	 if (!query.exec(sQuery)) {
		qWarning() << "Add-On DAO : Unable to update texture status!"
			   << m_db.lastError() << sQuery;
	}
}

StelAddOnDAO::AddOnInfo StelAddOnDAO::getAddOnInfo(int addonId)
{
	if (addonId < 1) {
		return AddOnInfo();
	}

	QSqlQuery query(m_db);
	query.prepare("SELECT id_install, category, installed, checksum, "
		      "download_url, download_filename, download_size "
		      "FROM addon WHERE id=:id");
	query.bindValue(":id", addonId);

	if (!query.exec()) {
		qWarning() << "Add-On DAO :" << m_db.lastError();
		return AddOnInfo();
	}

	QSqlRecord queryRecord = query.record();
	const int idInstallColumn = queryRecord.indexOf("id_install");
	const int categoryColumn = queryRecord.indexOf("category");
	const int urlColumn = queryRecord.indexOf("download_url");
	const int filenameColumn = queryRecord.indexOf("download_filename");
	const int sizeColumn = queryRecord.indexOf("download_size");
	const int installedColumn = queryRecord.indexOf("installed");
	const int checksumColumn = queryRecord.indexOf("checksum");
	if (query.next()) {
		AddOnInfo addonInfo;
		addonInfo.idInstall = query.value(idInstallColumn).toString();
		addonInfo.category =  query.value(categoryColumn).toString();
		addonInfo.url = QUrl(query.value(urlColumn).toString());
		addonInfo.size = query.value(sizeColumn).toFloat();
		addonInfo.filename = query.value(filenameColumn).toString();
		addonInfo.installed = query.value(installedColumn).toInt();
		addonInfo.checksum = query.value(checksumColumn).toString();

		QString categoryDir = StelApp::getInstance().getStelAddOnMgr().getDirectory(addonInfo.category);
		Q_ASSERT(!categoryDir.isEmpty());
		addonInfo.filepath = categoryDir % addonInfo.filename;
		return addonInfo;
	}

	return AddOnInfo();
}

StelAddOnDAO::WidgetInfo StelAddOnDAO::getAddOnWidgetInfo(int addonId)
{
	if (addonId < 1) {
		return WidgetInfo();
	}

	QSqlQuery query(m_db);
	query.prepare("SELECT id_install, description, download_size,"
		      "license.name AS licenseName, license.url AS licenseUrl,"
		      "a1.name AS a1Name, a1.email AS a1Email, a1.url AS a1Url,"
		      "a2.name AS a2Name, a2.email AS a2Email, a2.url AS a2Url "
		      "FROM author a1 LEFT JOIN addon ON a1.id = addon.author1 "
		      "LEFT JOIN author a2 ON a2.id = addon.author2 "
		      "LEFT JOIN license ON license.id = addon.license "
		      "WHERE addon.id=:id");
	query.bindValue(":id", addonId);

	if (!query.exec()) {
		qWarning() << "Add-On DAO : WidgetInfo failed!" << m_db.lastError();
		return WidgetInfo();
	}

	QSqlRecord queryRecord = query.record();
	const int idInstallColumn = queryRecord.indexOf("id_install");
	const int descriptionColumn = queryRecord.indexOf("description");
	const int downloadSizeColumn = queryRecord.indexOf("download_size");
	const int licenseNameColumn = queryRecord.indexOf("licenseName");
	const int licenseUrlColumn = queryRecord.indexOf("licenseUrl");
	const int a1NameColumn = queryRecord.indexOf("a1Name");
	const int a1EmailColumn = queryRecord.indexOf("a1Email");
	const int a1UrlColumn = queryRecord.indexOf("a1Url");
	const int a2NameColumn = queryRecord.indexOf("a2Name");
	const int a2EmailColumn = queryRecord.indexOf("a2Email");
	const int a2UrlColumn = queryRecord.indexOf("a2Url");

	WidgetInfo widgetInfo;
	if (query.next()) {
		widgetInfo.idInstall = query.value(idInstallColumn).toString();
		widgetInfo.description = query.value(descriptionColumn).toString();
		widgetInfo.downloadSize = query.value(downloadSizeColumn).toString();
		widgetInfo.licenseName =  query.value(licenseNameColumn).toString();
		widgetInfo.licenseUrl = query.value(licenseUrlColumn).toString();
		widgetInfo.a1Name = query.value(a1NameColumn).toString();
		widgetInfo.a1Email = query.value(a1EmailColumn).toString();
		widgetInfo.a1Url = query.value(a1UrlColumn).toString();
		widgetInfo.a2Name = query.value(a2NameColumn).toString();
		widgetInfo.a2Email = query.value(a2EmailColumn).toString();
		widgetInfo.a2Url = query.value(a2UrlColumn).toString();
	}
	return widgetInfo;
}

QHash<QString, QString> StelAddOnDAO::getThumbnails()
{
	QString sQuery("SELECT id_install, thumbnail "
		       "FROM landscape INNER JOIN addon "
		       "ON landscape.addon = addon.id");
	QSqlQuery query(m_db);
	if (!query.exec(sQuery)) {
		qWarning() << "Add-On DAO Thumbnails:" << m_db.lastError() << sQuery;
		return QHash<QString, QString>();
	}

	QHash<QString, QString> res;
	while (query.next()) {
		res.insert(query.value(0).toString(), query.value(1).toString());
	}
	return res;
}

QStringList StelAddOnDAO::getListOfTextures(int addonId)
{
	if (addonId < 1) {
		qWarning() << "Add-On DAO ListOfTextures: Invalid addonId!";
		return QStringList();
	}

	QSqlQuery query(m_db);
	query.prepare("SELECT textures FROM texture WHERE addon=:id");
	query.bindValue(":id", addonId);
	if (!query.exec()) {
		qWarning() << "Add-On DAO ListOfTextures:" << m_db.lastError();
		return QStringList();
	}

	QStringList res;
	if (query.next()) {
		res = query.value(0).toString().split(",");
	}
	return res;
}

QString StelAddOnDAO::getLanguagePackType(const QString& checksum)
{
	if (checksum.isEmpty()) {
		return QString();
	}

	QString sQuery("SELECT type FROM " % TABLE_LANGUAGE_PACK %
		       " INNER JOIN addon ON language_pack.addon = addon.id"
		       " WHERE checksum='" % checksum % "'");
	QSqlQuery query(m_db);
	if (!query.exec(sQuery)) {
		qWarning() << "Add-On DAO Language Type:" << m_db.lastError();
		return QString();
	}

	QString res;
	if (query.next()) {
		res = query.value(0).toString();
	}
	return res;
}
