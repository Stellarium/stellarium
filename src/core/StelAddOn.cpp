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

#include "StelAddOn.hpp"
#include "StelFileMgr.hpp"

StelAddOn::StelAddOn()
	: m_db(QSqlDatabase::addDatabase("QSQLITE"))
{
	// creating addon dir
	StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/addon");

	// Init database
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString dbPath = StelFileMgr::findFile("addon/", flags);
	m_db.setHostName("localhost");
	m_db.setDatabaseName(dbPath.append("addon.sqlite"));
	bool ok = m_db.open();
	qDebug() << "Add-On Database status:" << m_db.databaseName() << "=" << ok;
	if (m_db.lastError().isValid())
	{
	    qDebug() << "Error loading Add-On database:" << m_db.lastError();
	    exit(-1);
	}

	// creating tables
	if (!createAddonTables() ||
	    !createTableLicense() ||
	    !createTableAuthor())
	{
		exit(-1);
	}
}

bool StelAddOn::createAddonTables()
{
	QStringList addonTables;

	addonTables << "CREATE TABLE IF NOT EXISTS addon ("
			"id INTEGER primary key AUTOINCREMENT, "
			"title TEXT UNIQUE, "
			"description TEXT, "
			"version TEXT, "
			"compatibility TEXT, "
			"directory TEXT, "
			"author1 INTEGER, "
			"author2 INTEGER, "
			"license INTEGER, "
			"url TEXT, "
			"download_size TEXT, "
			"checksum TEXT, "
			"last_update TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS plugin ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS star ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE, "
			"count INTEGER, "
			"mag_range TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS landscape ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE, "
			"thumbnail TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS language_pack ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS script ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS starlore ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS texture ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	QSqlQuery query(m_db);
	foreach (QString table, addonTables) {
		query.prepare(table);
		if (!query.exec())
		{
		  qDebug() << "Add-On Manager : unable to create the addon table." << m_db.lastError();
		  return false;
		}
	}
	return true;
}

bool StelAddOn::createTableLicense()
{
	QSqlQuery query(m_db);
	query.prepare(
		"CREATE TABLE IF NOT EXISTS license ("
			"id INTEGER primary key AUTOINCREMENT, "
			"name TEXT, "
			"url TEXT)"
	);
	if (!query.exec())
	{
	  qDebug() << "Add-On Manager : unable to create the license table." << m_db.lastError();
	  return false;
	}
	return true;
}

bool StelAddOn::createTableAuthor()
{
	QSqlQuery query(m_db);
	query.prepare(
		"CREATE TABLE IF NOT EXISTS author ("
			"id INTEGER primary key AUTOINCREMENT, "
			"name TEXT, "
			"email TEXT, "
			"url TEXT)"
	);
	if (!query.exec())
	{
	  qDebug() << "Add-On Manager : unable to create the author table." << m_db.lastError();
	  return false;
	}
	return true;
}
