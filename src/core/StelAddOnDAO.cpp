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
	    qDebug() << "Error loading Add-On database:" << m_db.lastError();
	    return false;
	}

	// creating tables
	if (!createAddonTables() ||
	    !createTableLicense() ||
	    !createTableAuthor())
	{
		return false;
	}

	return true;
}

bool StelAddOnDAO::createAddonTables()
{
	QStringList addonTables;

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_ADDON % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"category TEXT, "
			"title TEXT UNIQUE, "
			"description TEXT, "
			"version TEXT, "
			"compatibility TEXT, "
			"author1 INTEGER, "
			"author2 INTEGER, "
			"license INTEGER, "
			"installed TEXT, "
			"directory TEXT, "
			"url TEXT, "
			"filename TEXT, "
			"download_size TEXT, "
			"checksum TEXT, "
			"last_update TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_CATALOG % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE, "
			"type TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_PLUGIN_CATALOG % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"catalog INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_STAR_CATALOG % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"catalog INTEGER UNIQUE, "
			"count INTEGER, "
			"mag_range TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_LANDSCAPE % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE, "
			"thumbnail TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_LANGUAGE_PACK % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_SCRIPT % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_SKY_CULTURE % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_TEXTURE % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	QSqlQuery query(m_db);
	foreach (QString table, addonTables)
	{
		query.prepare(table);
		if (!query.exec())
		{
			qDebug() << "Add-On Manager : unable to create the addon table."
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
		qDebug() << "Add-On Manager : unable to create the license table."
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
		qDebug() << "Add-On Manager : unable to create the author table."
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
		qDebug() << "Add-On Manager : unable to update database."
			 << m_db.lastError();
		return false;
	}

	return true;
}

void StelAddOnDAO::updateInstalledAddon(QString filename,
					QString installedVersion,
					QString directory)
{
	 QSqlQuery query(m_db);
	 query.prepare("UPDATE addon "
		       "SET installed=:installed, directory=:dir "
		       "WHERE filename=:filename");
	 query.bindValue(":installed", installedVersion);
	 query.bindValue(":dir", directory);
	 query.bindValue(":filename", filename);

	 if (!query.exec()) {
		qWarning() << "Add-On Manager :" << m_db.lastError();
	}
}

StelAddOnDAO::AddOnInfo StelAddOnDAO::getAddOnInfo(int addonId)
{
	if (addonId < 1) {
		return AddOnInfo();
	}

	QSqlQuery query(m_db);
	query.prepare("SELECT category, download_size, url, filename, directory "
		      "FROM addon WHERE id=:id");
	query.bindValue(":id", addonId);

	if (!query.exec()) {
		qWarning() << "Add-On Manager :" << m_db.lastError();
		return AddOnInfo();
	}

	QSqlRecord queryRecord = query.record();
	const int categoryColumn = queryRecord.indexOf("category");
	const int downloadSizeColumn = queryRecord.indexOf("download_size");
	const int urlColumn = queryRecord.indexOf("url");
	const int filenameColumn = queryRecord.indexOf("filename");
	const int directoryColumn = queryRecord.indexOf("directory");
	if (query.next()) {
		AddOnInfo addonInfo;
		// info from db
		addonInfo.category =  query.value(categoryColumn).toString();
		addonInfo.downloadSize = query.value(downloadSizeColumn).toDouble();
		addonInfo.url = QUrl(query.value(urlColumn).toString());
		addonInfo.filename = query.value(filenameColumn).toString();
		addonInfo.installedDir = QDir(query.value(directoryColumn).toString());
		QString categoryDir = StelApp::getInstance().getStelAddOnMgr().getDirectory(addonInfo.category);
		Q_ASSERT(!categoryDir.isEmpty());
		addonInfo.filepath = categoryDir % addonInfo.filename;
		return addonInfo;
	}

	return AddOnInfo();
}
