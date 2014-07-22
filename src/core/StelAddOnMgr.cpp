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
{
	// creating addon dir
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sDirAddOn);

	// Init database
	Q_ASSERT(m_pStelAddOnDAO->init());

	// creating sub-dirs
	m_dirs.insert(CATALOG, m_sDirAddOn % "/" % CATALOG % "/");
	m_dirs.insert(LANDSCAPE, m_sDirAddOn % "/" % LANDSCAPE % "/");
	m_dirs.insert(LANGUAGE_PACK, m_sDirAddOn % "/" % LANGUAGE_PACK % "/");
	m_dirs.insert(SCRIPT, m_sDirAddOn % "/" % SCRIPT % "/");
	m_dirs.insert(SKY_CULTURE, m_sDirAddOn % "/" % SKY_CULTURE % "/");
	m_dirs.insert(TEXTURE, m_sDirAddOn % "/" % TEXTURE % "/");
	QHashIterator<QString, QString> it(m_dirs);
	while (it.hasNext()) {
		it.next();
		StelFileMgr::makeSureDirExistsAndIsWritable(it.value());
	}

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
	m_pStelAddOns.insert(LANDSCAPE, new AOLandscape(m_pStelAddOnDAO));
	m_pStelAddOns.insert(LANGUAGE_PACK, new AOLanguagePack(m_pStelAddOnDAO));
	m_pStelAddOns.insert(SCRIPT, new AOScript(m_pStelAddOnDAO));
	m_pStelAddOns.insert(SKY_CULTURE, new AOSkyCulture(m_pStelAddOnDAO));
	m_pStelAddOns.insert(TEXTURE, new AOTexture(m_pStelAddOnDAO));

	connect(m_pStelAddOns.value(SKY_CULTURE), SIGNAL(skyCulturesChanged()),
		this, SIGNAL(skyCulturesChanged()));

	// refresh add-ons statuses (it checks which are installed or not)
	refreshAddOnStatuses();
}

StelAddOnMgr::~StelAddOnMgr()
{
}

QString StelAddOnMgr::getDirectory(QString category)
{
	return m_dirs.value(category);
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

void StelAddOnMgr::refreshAddOnStatuses()
{
	// mark all add-ons as uninstalled
	m_pStelAddOnDAO->markAllAddOnsAsUninstalled();

	// check add-ons which are already installed
	QHashIterator<QString, StelAddOn*> aos(m_pStelAddOns);
	while (aos.hasNext())
	{
		aos.next();
		if (aos.key() == LANGUAGE_PACK)
		{
			m_pStelAddOnDAO->markAddOnsAsInstalledFromMd5(aos.value()->checkInstalledAddOns());
		}
		else
		{
			m_pStelAddOnDAO->markAddOnsAsInstalled(aos.value()->checkInstalledAddOns());
		}
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

	// check add-ons which are already installed
	refreshAddOnStatuses();

	return true;
}

void StelAddOnMgr::downloadAddOn(const StelAddOnDAO::AddOnInfo addonInfo)
{
	Q_ASSERT(m_pDownloadReply==NULL);
	Q_ASSERT(m_currentDownloadFile==NULL);
	Q_ASSERT(m_progressBar==NULL);

	m_currentDownloadInfo = addonInfo;
	m_currentDownloadFile = new QFile(addonInfo.filepath);
	if (!m_currentDownloadFile->open(QIODevice::WriteOnly))
	{
		qWarning() << "Can't open a writable file: "
			   << QDir::toNativeSeparators(addonInfo.filepath);
		return;
	}

	m_bDownloading = true;
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
	m_progressBar->setRange(0, addonInfo.size*1024);
	m_progressBar->setFormat(QString("%1: %p%").arg(addonInfo.filename));
}

void StelAddOnMgr::newDownloadedData()
{
	Q_ASSERT(m_currentDownloadFile);
	Q_ASSERT(m_pDownloadReply);
	Q_ASSERT(m_progressBar);

	int size = m_pDownloadReply->bytesAvailable();
	m_progressBar->setValue((float)m_progressBar->getValue()+(float)size/1024);
	m_currentDownloadFile->write(m_pDownloadReply->read(size));
}

void StelAddOnMgr::downloadFinished()
{
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
		m_currentDownloadFile->reset();
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

	installFromFile(m_currentDownloadInfo);
	m_currentDownloadInfo = StelAddOnDAO::AddOnInfo();
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
		return installFromFile(addonInfo);
	}

	// download file
	m_downloadQueue.append(addonId);
	if (!m_bDownloading)
	{
		downloadAddOn(addonInfo);
	}
}

void StelAddOnMgr::installFromFile(const StelAddOnDAO::AddOnInfo addonInfo)
{
	bool installed = m_pStelAddOns.value(addonInfo.category)
			->installFromFile(addonInfo.idInstall, addonInfo.filepath);

	m_pStelAddOnDAO->updateAddOnStatus(addonInfo.idInstall, installed);
	emit (updateTableViews());
}

void StelAddOnMgr::removeAddOn(const int addonId)
{
	StelAddOnDAO::AddOnInfo addonInfo = m_pStelAddOnDAO->getAddOnInfo(addonId);
	if (m_pStelAddOns.value(addonInfo.category)->uninstallAddOn(addonInfo.idInstall))
	{
		m_pStelAddOnDAO->updateAddOnStatus(addonInfo.idInstall, false);
		emit (updateTableViews());
	}
}
