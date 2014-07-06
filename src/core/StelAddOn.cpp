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
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringBuilder>

#include "LandscapeMgr.hpp"
#include "StelApp.hpp"
#include "StelAddOn.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelProgressController.hpp"

StelAddOn::StelAddOn()
	: m_db(QSqlDatabase::addDatabase("QSQLITE"))
	, m_progressBar(NULL)
	, m_bDownloading(false)
	, m_pDownloadReply(NULL)
	, m_currentDownloadFile(NULL)
	, dirAddOn(StelFileMgr::getUserDir() % "/addon")
	, dirCatalog(dirAddOn % "/catalog/")
	, dirLandscape(dirAddOn % "/landscape/")
	, dirLanguagePack(dirAddOn % "/language_pack/")
	, dirScript(dirAddOn % "/script/")
	, dirStarlore(dirAddOn % "/language_pack/")
	, dirTexture(dirAddOn % "/texture/")
{
	// creating addon dir
	StelFileMgr::makeSureDirExistsAndIsWritable(dirAddOn);
	// creating sub-dirs
	StelFileMgr::makeSureDirExistsAndIsWritable(dirCatalog);
	StelFileMgr::makeSureDirExistsAndIsWritable(dirLandscape);
	StelFileMgr::makeSureDirExistsAndIsWritable(dirLanguagePack);
	StelFileMgr::makeSureDirExistsAndIsWritable(dirScript);
	StelFileMgr::makeSureDirExistsAndIsWritable(dirStarlore);
	StelFileMgr::makeSureDirExistsAndIsWritable(dirTexture);

	// Init database
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	m_sAddonPath = StelFileMgr::findFile("addon/", flags);
	m_db.setHostName("localhost");
	m_db.setDatabaseName(m_sAddonPath % "addon.sqlite");
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

	// create file to store the last update time
	QString lastUpdate;
	QFile file(m_sAddonPath  % "/lastdbupdate.txt");
	if (file.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		QTextStream txt(&file);
		lastUpdate = txt.readAll();
		if(lastUpdate.isEmpty())
		{
			lastUpdate = "1388966410";
			txt << lastUpdate;
		}
		file.close();
	}
	m_iLastUpdate = lastUpdate.toLong();
}

QString StelAddOn::getDirectory(QString category)
{
	QString dir;
	if (category == LANDSCAPE)
	{
		dir = dirLandscape;
	}
	else if (category == LANGUAGE_PACK)
	{
		dir = dirLanguagePack;
	}
	else if (category == SCRIPT)
	{
		dir = dirScript;
	}
	else if (category == STARLORE)
	{
		dir = dirStarlore;
	}
	else if (category == TEXTURE)
	{
		dir = dirTexture;
	}
	else if (category == PLUGIN_CATALOG || category == STAR_CATALOG)
	{
		dir = dirCatalog;
	}
	return dir;
}

bool StelAddOn::createAddonTables()
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

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_STARLORE % " ("
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

bool StelAddOn::createTableLicense()
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

bool StelAddOn::createTableAuthor()
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

void StelAddOn::setLastUpdate(qint64 time) {
	m_iLastUpdate = time;
	// store value it in the txt file
	QFile file(m_sAddonPath  % "/lastdbupdate.txt");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream txt(&file);
		txt << QString::number(m_iLastUpdate);
		file.close();
	}
}

bool StelAddOn::updateDatabase(QString webresult)
{
	QSqlQuery query(m_db);
	QStringList queries = webresult.split("<br>");
	queries.removeFirst();
	foreach (QString insert, queries)
	{
		query.prepare(insert.simplified());
		if (!query.exec())
		{
			qDebug() << "Add-On Manager : unable to update database."
				 << m_db.lastError();
			return false;
		}
	}

	// check add-ons which are already installed
	// landscapes
	QDir landscapeDestination = GETSTELMODULE(LandscapeMgr)->getLandscapeDir();
	QStringList landscapes = GETSTELMODULE(LandscapeMgr)->getUserLandscapeIDs();
	foreach (QString landscape, landscapes) {
		if (landscapeDestination.cd(landscape))
		{
			updateInstalledAddon(landscape % ".zip",
					     "1.0",
					     landscapeDestination.absolutePath());
			landscapeDestination.cdUp();
		}
	}

	return true;
}

void StelAddOn::updateInstalledAddon(QString filename,
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

StelAddOn::AddOnInfo StelAddOn::getAddOnInfo(int addonId)
{
	if (addonId < 1) {
		return AddOnInfo();
	}

	QSqlQuery query(m_db);
	query.prepare("SELECT category, download_size, url "
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
	if (query.next()) {
		AddOnInfo addonInfo;
		// info from db
		addonInfo.category =  query.value(categoryColumn).toString();
		addonInfo.downloadSize = query.value(downloadSizeColumn).toDouble();
		addonInfo.url = QUrl(query.value(urlColumn).toString());
		// handling url to get filename and filepath
		QStringList splitedURL = addonInfo.url.toString().split("/");
		addonInfo.filename = splitedURL.last();
		if (addonInfo.filename.isEmpty())
		{
			addonInfo.filename = splitedURL.at(splitedURL.length() - 2);
		}
		QString dir = getDirectory(addonInfo.category);
		Q_ASSERT(!dir.isEmpty());
		addonInfo.filepath = dir % addonInfo.filename;
		return addonInfo;
	}

	return AddOnInfo();
}

void StelAddOn::downloadAddOn(const AddOnInfo addonInfo)
{
	Q_ASSERT(m_pDownloadReply==NULL);
	Q_ASSERT(m_currentDownloadFile==NULL);
	Q_ASSERT(m_currentDownloadCategory.isEmpty());
	Q_ASSERT(m_currentDownloadPath.isEmpty());
	Q_ASSERT(m_progressBar==NULL);

	m_currentDownloadPath = addonInfo.filepath;
	m_currentDownloadFile = new QFile(addonInfo.filepath);
	if (!m_currentDownloadFile->open(QIODevice::WriteOnly))
	{
		qWarning() << "Can't open a writable file: "
			   << QDir::toNativeSeparators(addonInfo.filepath);
		return;
	}

	m_bDownloading = true;
	m_currentDownloadCategory = addonInfo.category;
	QNetworkRequest req(addonInfo.url);
	req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
	m_pDownloadReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	m_pDownloadReply->setReadBufferSize(1024*1024*2);
	connect(m_pDownloadReply, SIGNAL(readyRead()), this, SLOT(newDownloadedData()));
	connect(m_pDownloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));

	m_progressBar = StelApp::getInstance().addProgressBar();
	m_progressBar->setValue(0);
	m_progressBar->setRange(0, addonInfo.downloadSize*1024);
	m_progressBar->setFormat(QString("%1: %p%").arg(addonInfo.filename));
}

void StelAddOn::newDownloadedData()
{
	Q_ASSERT(!m_currentDownloadCategory.isEmpty());
	Q_ASSERT(!m_currentDownloadPath.isEmpty());
	Q_ASSERT(m_currentDownloadFile);
	Q_ASSERT(m_pDownloadReply);
	Q_ASSERT(m_progressBar);

	int size = m_pDownloadReply->bytesAvailable();
	m_progressBar->setValue((float)m_progressBar->getValue()+(float)size/1024);
	m_currentDownloadFile->write(m_pDownloadReply->read(size));
}

void StelAddOn::downloadFinished()
{
	Q_ASSERT(!m_currentDownloadCategory.isEmpty());
	Q_ASSERT(!m_currentDownloadPath.isEmpty());
	Q_ASSERT(m_currentDownloadFile);
	Q_ASSERT(m_pDownloadReply);
	Q_ASSERT(m_progressBar);

	if (m_pDownloadReply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Add-on Mgr: FAILED to download" << m_pDownloadReply->url()
			   << " Error:" << m_pDownloadReply->errorString();
	}

	m_currentDownloadFile->close();
	m_currentDownloadFile->deleteLater();
	m_currentDownloadFile = NULL;
	m_pDownloadReply->deleteLater();
	m_pDownloadReply = NULL;
	StelApp::getInstance().removeProgressBar(m_progressBar);
	m_progressBar = NULL;

	installFromFile(m_currentDownloadCategory, m_currentDownloadPath);
	m_currentDownloadPath.clear();
	m_currentDownloadCategory.clear();
	m_bDownloading = false;
	m_downloadQueue.removeFirst();
	if (!m_downloadQueue.isEmpty())
	{
		// next download
		downloadAddOn(getAddOnInfo(m_downloadQueue.first()));
	}
}

void StelAddOn::installAddOn(const int addonId)
{
	if (m_downloadQueue.contains(addonId))
	{
		return;
	}

	AddOnInfo addonInfo = getAddOnInfo(addonId);
	// checking if we have this file in add-on path (disk)
	if (QFile(addonInfo.filepath).exists())
	{
		return installFromFile(addonInfo.category, addonInfo.filepath);
	}

	// download file
	m_downloadQueue.append(addonId);
	if (!m_bDownloading)
	{
		downloadAddOn(addonInfo);
	}
}

void StelAddOn::installFromFile(QString category, QString filePath)
{
	if (category == LANDSCAPE)
	{
		QString ref = GETSTELMODULE(LandscapeMgr)->installLandscapeFromArchive(filePath);
		if(!ref.isEmpty())
		{
			qWarning() << "FAILED to install " << filePath;
		}
		// update database
		QDir landscapeDestination = GETSTELMODULE(LandscapeMgr)->getLandscapeDir();
		if (landscapeDestination.cd(ref))
		{
			updateInstalledAddon(ref % ".zip", "1.0",
					     landscapeDestination.absolutePath());
		}
	}
}
