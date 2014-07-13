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
#include <QFile>
#include <QStringBuilder>

#include "LandscapeMgr.hpp"
#include "StelApp.hpp"
#include "StelAddOnMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelProgressController.hpp"

StelAddOnMgr::StelAddOnMgr()
	: m_db(QSqlDatabase::addDatabase("QSQLITE"))
	, m_pStelAddOnDAO(new StelAddOnDAO(m_db))
	, m_bDownloading(false)
	, m_pDownloadReply(NULL)
	, m_currentDownloadFile(NULL)
	, m_progressBar(NULL)
	, m_sDirAddOn(StelFileMgr::getUserDir() % "/addon")
	, m_sDirCatalog(m_sDirAddOn % "/catalog/")
	, m_sDirLandscape(m_sDirAddOn % "/landscape/")
	, m_sDirLanguagePack(m_sDirAddOn % "/language_pack/")
	, m_sDirScript(m_sDirAddOn % "/script/")
	, m_sDirStarlore(m_sDirAddOn % "/language_pack/")
	, m_sDirTexture(m_sDirAddOn % "/texture/")
{
	// creating addon dir
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sDirAddOn);

	// Init database
	Q_ASSERT(m_pStelAddOnDAO->init());

	// creating sub-dirs
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sDirCatalog);
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sDirLandscape);
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sDirLanguagePack);
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sDirScript);
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sDirStarlore);
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sDirTexture);

	// create file to store the last update time
	QString lastUpdate;
	QFile file(m_sDirAddOn  % "/lastdbupdate.txt");
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

	// Init sub-classes
	m_pLandscape = new AOLandscape(m_pStelAddOnDAO);
	m_pScript = new AOScript(m_pStelAddOnDAO);
	m_pTexture = new AOTexture(m_pStelAddOnDAO);

	// check add-ons which are already installed
	m_pLandscape->checkInstalledAddOns();
	m_pScript->checkInstalledAddOns();
	m_pTexture->checkInstalledAddOns();
}

StelAddOnMgr::~StelAddOnMgr()
{
}

QString StelAddOnMgr::getDirectory(QString category)
{
	QString dir;
	if (category == LANDSCAPE)
	{
		dir = m_sDirLandscape;
	}
	else if (category == LANGUAGE_PACK)
	{
		dir = m_sDirLanguagePack;
	}
	else if (category == SCRIPT)
	{
		dir = m_sDirScript;
	}
	else if (category == STARLORE)
	{
		dir = m_sDirStarlore;
	}
	else if (category == TEXTURE)
	{
		dir = m_sDirTexture;
	}
	else if (category == PLUGIN_CATALOG || category == STAR_CATALOG)
	{
		dir = m_sDirCatalog;
	}
	return dir;
}

void StelAddOnMgr::setLastUpdate(qint64 time) {
	m_iLastUpdate = time;
	// store value it in the txt file
	QFile file(m_sDirAddOn  % "/lastdbupdate.txt");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream txt(&file);
		txt << QString::number(m_iLastUpdate);
		file.close();
	}
}

bool StelAddOnMgr::updateCatalog(QString webresult)
{
	QStringList queries = webresult.split("<br>");
	queries.removeFirst();
	foreach (QString insert, queries)
	{
		if (!m_pStelAddOnDAO->insertOnDatabase(insert))
		{
			return false;
		}
	}
	return true;
}

void StelAddOnMgr::downloadAddOn(const StelAddOnDAO::AddOnInfo addonInfo)
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

void StelAddOnMgr::newDownloadedData()
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

void StelAddOnMgr::downloadFinished()
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
		m_currentDownloadFile->close();
		m_currentDownloadFile->deleteLater();
		m_currentDownloadFile = NULL;
		m_pDownloadReply->deleteLater();
		m_pDownloadReply = NULL;
		StelApp::getInstance().removeProgressBar(m_progressBar);
		m_progressBar = NULL;
		return;
	}

	Q_ASSERT(m_pDownloadReply->bytesAvailable()==0);

	const QVariant& redirect = m_pDownloadReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	if (!redirect.isNull())
	{
		// We got a redirection, we need to follow
		m_pDownloadReply->deleteLater();
		QNetworkRequest req(redirect.toUrl());
		req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
		req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
		m_pDownloadReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		m_pDownloadReply->setReadBufferSize(1024*1024*2);
		connect(m_pDownloadReply, SIGNAL(readyRead()), this, SLOT(newDownloadedData()));
		connect(m_pDownloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		return;
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
		downloadAddOn(m_pStelAddOnDAO->getAddOnInfo(m_downloadQueue.first()));
	}
}

void StelAddOnMgr::installAddOn(const int addonId)
{
	if (m_downloadQueue.contains(addonId))
	{
		return;
	}

	StelAddOnDAO::AddOnInfo addonInfo = m_pStelAddOnDAO->getAddOnInfo(addonId);
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

void StelAddOnMgr::installFromFile(QString category, QString filePath)
{
	if (category == LANDSCAPE)
	{
		m_pLandscape->installFromFile(filePath);
	}
	else if (category == SCRIPT)
	{
		m_pScript->installFromFile(filePath);
	}
	else if (category == TEXTURE)
	{
		m_pTexture->installFromFile(filePath);
	}

	if (m_downloadQueue.isEmpty())
	{
		emit (updateTableViews());
	}
}

void StelAddOnMgr::removeAddOn(const int addonId)
{
	bool removed = false;
	StelAddOnDAO::AddOnInfo addonInfo = m_pStelAddOnDAO->getAddOnInfo(addonId);
	if (addonInfo.category == LANDSCAPE)
	{
		removed = m_pLandscape->uninstallAddOn(addonInfo);
	}
	else if (addonInfo.category == TEXTURE)
	{
		removed = m_pTexture->uninstallAddOn(addonInfo);
	}

	if (removed)
	{
		emit (updateTableViews());
	}
}
