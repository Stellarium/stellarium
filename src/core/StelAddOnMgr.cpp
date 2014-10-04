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
#include <QJsonDocument>
#include <QPixmap>
#include <QSettings>
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
	, m_pConfig(StelApp::getInstance().getSettings())
	, m_iDownloadingId(0)
	, m_pAddOnNetworkReply(NULL)
	, m_currentDownloadFile(NULL)
	, m_progressBar(NULL)
	, m_iLastUpdate(1388966410)
	, m_iUpdateFrequencyDays(7)
	, m_iUpdateFrequencyHour(12)
	, m_sUrlUpdate("http://cardinot.sourceforge.net/getUpdates.php")
	, m_sAddOnDir(StelFileMgr::getUserDir() % "/addon/")
	, m_sThumbnailDir(m_sAddOnDir % "thumbnail/")
	, m_sJsonPath(m_sAddOnDir % "addons.json")
{
	// creating addon dir
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sAddOnDir);
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sThumbnailDir);

	// Initialize settings in the main config file
	if (m_pConfig->childGroups().contains("AddOn"))
	{
		m_pConfig->beginGroup("AddOn");
		m_iLastUpdate = m_pConfig->value("last_update", m_iLastUpdate).toLongLong();
		m_iUpdateFrequencyDays = m_pConfig->value("upload_frequency_days", m_iUpdateFrequencyDays).toInt();
		m_iUpdateFrequencyHour = m_pConfig->value("upload_frequency_hour", m_iUpdateFrequencyHour).toInt();
		m_sUrlUpdate = m_pConfig->value("url", m_sUrlUpdate).toString();
		m_pConfig->endGroup();
	}
	else // If no settings were found, create it with default values
	{
		qDebug() << "StelAddOnMgr: no AddOn section exists in main config file - creating with defaults";
		m_pConfig->beginGroup("AddOn");
		// delete all existing settings...
		m_pConfig->remove("");
		m_pConfig->setValue("last_update", m_iLastUpdate);
		m_pConfig->setValue("upload_frequency_days", m_iUpdateFrequencyDays);
		m_pConfig->setValue("upload_frequency_hour", m_iUpdateFrequencyHour);
		m_pConfig->setValue("url", m_sUrlUpdate);
		m_pConfig->endGroup();
	}

	// creating Json file
	if (!QFile(m_sJsonPath).exists())
	{
		qDebug() << "Add-On Mgr: addons.json does not exist - copying default file to"
			 << QDir::toNativeSeparators(m_sJsonPath);

		QFile defaultJson(StelFileMgr::getInstallationDir() % "/data/default_addons.json");
		if (defaultJson.copy(m_sJsonPath))
		{
			qDebug() << "Add-On Mgr: default_addons.json was copied to" << m_sJsonPath;
			// The resource is read only, and the new file inherits this...  make sure the new file
			// is writable by the Stellarium process so that updates can be done.
			QFile jsonFile(m_sJsonPath);
			jsonFile.setPermissions(jsonFile.permissions() | QFile::WriteOwner);
			// cleaning last_update var
			m_pConfig->remove("AddOn/last_update");
			m_iLastUpdate = 1388966410;
		}
		else
		{
			qWarning() << "Add-On Mgr: cannot copy JSON resource to"
				   << QDir::toNativeSeparators(m_sJsonPath);
		}
	}

	// loading json file
	QFile jsonFile(m_sJsonPath);
	if (jsonFile.open(QIODevice::ReadOnly))
	{
		qDebug() << "Add-On Mgr: loading catalog file:"
			 << QDir::toNativeSeparators(m_sJsonPath);
		readJson(QJsonDocument::fromJson(jsonFile.readAll()).object());
	}
	else
	{
		qWarning() << "Add-On Mgr: Couldn't open addons.json file!"
			   << QDir::toNativeSeparators(m_sJsonPath);
	}

	// Init database
	m_pStelAddOnDAO->init();

	// creating sub-dirs
	m_dirs.insert(CATEGORY_CATALOG, m_sAddOnDir % CATEGORY_CATALOG % "/");
	m_dirs.insert(CATEGORY_LANDSCAPE, m_sAddOnDir % CATEGORY_LANDSCAPE % "/");
	m_dirs.insert(CATEGORY_LANGUAGE_PACK, m_sAddOnDir % CATEGORY_LANGUAGE_PACK % "/");
	m_dirs.insert(CATEGORY_SCRIPT, m_sAddOnDir % CATEGORY_SCRIPT % "/");
	m_dirs.insert(CATEGORY_SKY_CULTURE, m_sAddOnDir % CATEGORY_SKY_CULTURE % "/");
	m_dirs.insert(CATEGORY_TEXTURE, m_sAddOnDir % CATEGORY_TEXTURE % "/");
	QHashIterator<QString, QString> it(m_dirs);
	while (it.hasNext()) {
		it.next();
		StelFileMgr::makeSureDirExistsAndIsWritable(it.value());
	}

	// Init sub-classes
	m_pStelAddOns.insert(CATEGORY_CATALOG, new AOCatalog());
	m_pStelAddOns.insert(CATEGORY_LANDSCAPE, new AOLandscape());
	m_pStelAddOns.insert(CATEGORY_LANGUAGE_PACK, new AOLanguagePack());
	m_pStelAddOns.insert(CATEGORY_SCRIPT, new AOScript());
	m_pStelAddOns.insert(CATEGORY_SKY_CULTURE, new AOSkyCulture());
	m_pStelAddOns.insert(CATEGORY_TEXTURE, new AOTexture());

	connect(m_pStelAddOns.value(CATEGORY_SKY_CULTURE), SIGNAL(skyCulturesChanged()),
		this, SIGNAL(skyCulturesChanged()));

	// refresh add-ons statuses (it checks which are installed or not)
	refreshAddOnStatuses();
}

StelAddOnMgr::~StelAddOnMgr()
{
}

void StelAddOnMgr::readJson(const QJsonObject &json)
{
	// TODO
}

void StelAddOnMgr::setLastUpdate(qint64 time) {
	m_iLastUpdate = time;
	// update config file
	m_pConfig->beginGroup("AddOn");
	m_pConfig->setValue("last_update", m_iLastUpdate);
	m_pConfig->endGroup();
}

void StelAddOnMgr::setUpdateFrequencyDays(int days) {
	m_iUpdateFrequencyDays = days;
	// update config file
	m_pConfig->beginGroup("AddOn");
	m_pConfig->setValue("upload_frequency_days", m_iUpdateFrequencyDays);
	m_pConfig->endGroup();
}

void StelAddOnMgr::setUpdateFrequencyHour(int hour) {
	m_iUpdateFrequencyHour = hour;
	// update config file
	m_pConfig->beginGroup("AddOn");
	m_pConfig->setValue("upload_frequency_hour", m_iUpdateFrequencyHour);
	m_pConfig->endGroup();
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
		QStringList list = aos.value()->checkInstalledAddOns();
		if (list.isEmpty())
		{
			continue;
		}

		if (aos.key() == CATEGORY_CATALOG || aos.key() == CATEGORY_LANGUAGE_PACK)
		{
			m_pStelAddOnDAO->markAddOnsAsInstalledFromMd5(list);
		}
		else if (aos.key() == CATEGORY_TEXTURE)
		{
			m_pStelAddOnDAO->markTexturesAsInstalled(list);
		}
		else
		{
			m_pStelAddOnDAO->markAddOnsAsInstalled(list);
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

	// download thumbnails
	m_thumbnails = m_pStelAddOnDAO->getThumbnails(CATEGORY_LANDSCAPE);
	m_thumbnails = m_thumbnails.unite(m_pStelAddOnDAO->getThumbnails(CATEGORY_SCRIPT));
	m_thumbnails = m_thumbnails.unite(m_pStelAddOnDAO->getThumbnails(CATEGORY_TEXTURE));
	QHashIterator<QString, QString> i(m_thumbnails); // <id_install, url>
	while (i.hasNext()) {
	    i.next();
	    if (!QFile(m_sThumbnailDir % i.key() % ".jpg").exists())
	    {
		    m_thumbnailQueue.append(i.value());
	    }
	}
	downloadNextThumbnail();

	// check add-ons which are already installed
	refreshAddOnStatuses();

	return true;
}

void StelAddOnMgr::downloadNextThumbnail()
{
	if (m_thumbnailQueue.isEmpty()) {
	    return;
	}

	QUrl url(m_thumbnailQueue.first());
	m_pThumbnailNetworkReply = StelApp::getInstance().getNetworkAccessManager()->get(QNetworkRequest(url));
	connect(m_pThumbnailNetworkReply, SIGNAL(finished()), this, SLOT(downloadThumbnailFinished()));
}

void StelAddOnMgr::downloadThumbnailFinished()
{
	if (m_thumbnailQueue.isEmpty() || m_pThumbnailNetworkReply == NULL) {
	    return;
	}

	if (m_pThumbnailNetworkReply->error() == QNetworkReply::NoError) {
	    QPixmap pixmap;
	    if (pixmap.loadFromData(m_pThumbnailNetworkReply->readAll())) {
		    pixmap.save(m_sThumbnailDir % m_thumbnails.key(m_thumbnailQueue.first()) % ".jpg");
	    }
	}

	m_pThumbnailNetworkReply->deleteLater();
	m_pThumbnailNetworkReply = NULL;
	m_thumbnailQueue.removeFirst();
	downloadNextThumbnail();
}

void StelAddOnMgr::installAddOn(const int addonId, const QStringList selectedFiles)
{
	if (m_downloadQueue.contains(addonId) || addonId < 1)
	{
		return;
	}

	StelAddOnDAO::AddOnInfo addonInfo = m_pStelAddOnDAO->getAddOnInfo(addonId);
	if (!installFromFile(addonInfo, selectedFiles))
	{
		// something goes wrong (file not found OR corrupt),
		// try downloading it...
		m_pStelAddOnDAO->updateAddOnStatus(addonInfo.idInstall, Installing);
		emit (dataUpdated(addonInfo.category));
		m_downloadQueue.insert(addonId, selectedFiles);
		downloadNextAddOn();
	}
}

bool StelAddOnMgr::installFromFile(const StelAddOnDAO::AddOnInfo addonInfo,
				   const QStringList selectedFiles)
{
	QFile file(addonInfo.filepath);
	// checking if we have this file in the add-on dir (local disk)
	if (!file.exists())
	{
		return false;
	}

	// checking integrity
	int status;
	if (addonInfo.checksum == calculateMd5(file))
	{
		// installing files
		status = m_pStelAddOns.value(addonInfo.category)
				->installFromFile(addonInfo.idInstall,
						  addonInfo.filepath,
						  selectedFiles);
	}
	else
	{
		status = Corrupted;
		qWarning() << "Add-On Mgr: Error: File "
			   << addonInfo.filename
			   << " is corrupt, MD5 mismatch!";
	}

	if ((status == PartiallyInstalled || status == FullyInstalled) &&
		(addonInfo.category == CATEGORY_LANGUAGE_PACK || addonInfo.category == CATEGORY_TEXTURE))
	{
		emit (addOnMgrMsg(RestartRequired));
	}

	m_pStelAddOnDAO->updateAddOnStatus(addonInfo.idInstall, status);
	emit (dataUpdated(addonInfo.category));
	return status;
}

void StelAddOnMgr::installFromFile(const QString& filePath)
{
	QFile file(filePath);
	int addonId = m_pStelAddOnDAO->getAddOnId(calculateMd5(file));
	StelAddOnDAO::AddOnInfo addonInfo;
	if (addonId > 0)
	{
		addonInfo = m_pStelAddOnDAO->getAddOnInfo(addonId);
	}

	if (!addonInfo.isCompatible)
	{
		qWarning() << "Add-On InstallFromFile : Unable to install"
			   << filePath << "File is not compatible!";
		return;
	}

	installFromFile(addonInfo, QStringList());
}

void StelAddOnMgr::removeAddOn(const int addonId, const QStringList selectedFiles)
{
	if (addonId < 1)
	{
		return;
	}

	StelAddOnDAO::AddOnInfo addonInfo = m_pStelAddOnDAO->getAddOnInfo(addonId);

	int status = m_pStelAddOns.value(addonInfo.category)->uninstallAddOn(addonInfo.idInstall, selectedFiles);
	m_pStelAddOnDAO->updateAddOnStatus(addonInfo.idInstall, status);
	emit (dataUpdated(addonInfo.category));

	if (addonInfo.category == CATEGORY_LANGUAGE_PACK || addonInfo.category == CATEGORY_TEXTURE)
	{
		emit (addOnMgrMsg(RestartRequired));
	}
}

QString StelAddOnMgr::calculateMd5(QFile& file) const
{
	QString checksum;
	if (file.open(QIODevice::ReadOnly)) {
		QCryptographicHash md5(QCryptographicHash::Md5);
		md5.addData(file.readAll());
		checksum = md5.result().toHex();
		file.close();
	}
	return checksum;
}

bool StelAddOnMgr::isCompatible(QString first, QString last)
{
	QStringList c = StelUtils::getApplicationVersion().split(".");
	QStringList f = first.split(".");
	QStringList l = last.split(".");

	if (c.size() < 3 || f.size() < 3 || l.size() < 3) {
		return false; // invalid version
	}

	int currentVersion = QString(c.at(0) % "00" % c.at(1) % "0" % c.at(2)).toInt();
	int firstVersion = QString(f.at(0) % "00" % f.at(1) % "0" % f.at(2)).toInt();
	int lastVersion = QString(l.at(0) % "00" % l.at(1) % "0" % l.at(2)).toInt();

	if (currentVersion < firstVersion || currentVersion > lastVersion)
	{
		return false; // out of bounds
	}

	return true;
}

void StelAddOnMgr::downloadNextAddOn()
{
	if (m_iDownloadingId)
	{
		return;
	}

	Q_ASSERT(m_pAddOnNetworkReply==NULL);
	Q_ASSERT(m_currentDownloadFile==NULL);
	Q_ASSERT(m_progressBar==NULL);

	m_iDownloadingId = m_downloadQueue.firstKey();
	m_currentDownloadInfo = m_pStelAddOnDAO->getAddOnInfo(m_iDownloadingId);
	m_currentDownloadFile = new QFile(m_currentDownloadInfo.filepath);
	if (!m_currentDownloadFile->open(QIODevice::WriteOnly))
	{
		qWarning() << "Can't open a writable file: "
			   << QDir::toNativeSeparators(m_currentDownloadInfo.filepath);
		cancelAllDownloads();
		emit (addOnMgrMsg(UnableToWriteFiles));
		return;
	}

	QNetworkRequest req(m_currentDownloadInfo.url);
	req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
	m_pAddOnNetworkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	m_pAddOnNetworkReply->setReadBufferSize(1024*1024*2);
	connect(m_pAddOnNetworkReply, SIGNAL(readyRead()), this, SLOT(newDownloadedData()));
	connect(m_pAddOnNetworkReply, SIGNAL(finished()), this, SLOT(downloadAddOnFinished()));

	m_progressBar = StelApp::getInstance().addProgressBar();
	m_progressBar->setValue(0);
	m_progressBar->setRange(0, m_currentDownloadInfo.size*1024);
	m_progressBar->setFormat(QString("%1: %p%").arg(m_currentDownloadInfo.filename));
}

void StelAddOnMgr::newDownloadedData()
{
	Q_ASSERT(m_currentDownloadFile);
	Q_ASSERT(m_pAddOnNetworkReply);
	Q_ASSERT(m_progressBar);

	int size = m_pAddOnNetworkReply->bytesAvailable();
	m_progressBar->setValue((float)m_progressBar->getValue()+(float)size/1024);
	m_currentDownloadFile->write(m_pAddOnNetworkReply->read(size));
}

void StelAddOnMgr::downloadAddOnFinished()
{
	Q_ASSERT(m_currentDownloadFile);
	Q_ASSERT(m_pAddOnNetworkReply);
	Q_ASSERT(m_progressBar);

	if (m_pAddOnNetworkReply->error() == QNetworkReply::NoError)
	{
		Q_ASSERT(m_pAddOnNetworkReply->bytesAvailable()==0);

		const QVariant& redirect = m_pAddOnNetworkReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
		if (!redirect.isNull())
		{
			// We got a redirection, we need to follow
			m_currentDownloadFile->reset();
			m_pAddOnNetworkReply->deleteLater();
			QNetworkRequest req(redirect.toUrl());
			req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
			req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
			m_pAddOnNetworkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
			m_pAddOnNetworkReply->setReadBufferSize(1024*1024*2);
			connect(m_pAddOnNetworkReply, SIGNAL(readyRead()), this, SLOT(newDownloadedData()));
			connect(m_pAddOnNetworkReply, SIGNAL(finished()), this, SLOT(downloadAddOnFinished()));
			return;
		}
		finishCurrentDownload();
		installFromFile(m_currentDownloadInfo, m_downloadQueue.value(m_iDownloadingId));
	}
	else
	{
		qWarning() << "Add-on Mgr: FAILED to download" << m_pAddOnNetworkReply->url()
			   << " Error:" << m_pAddOnNetworkReply->errorString();
		m_pStelAddOnDAO->updateAddOnStatus(m_currentDownloadInfo.idInstall, DownloadFailed);
		emit (dataUpdated(m_currentDownloadInfo.category));
		finishCurrentDownload();
	}

	m_currentDownloadInfo = StelAddOnDAO::AddOnInfo();
	m_downloadQueue.remove(m_iDownloadingId);
	m_iDownloadingId = 0;
	if (!m_downloadQueue.isEmpty())
	{
		// next download
		downloadNextAddOn();
	}
}

void StelAddOnMgr::finishCurrentDownload()
{
	if (m_currentDownloadFile)
	{
		m_currentDownloadFile->close();
		m_currentDownloadFile->deleteLater();
		m_currentDownloadFile = NULL;
	}

	if (m_pAddOnNetworkReply)
	{
		m_pAddOnNetworkReply->deleteLater();
		m_pAddOnNetworkReply = NULL;
	}

	if (m_progressBar)
	{
		StelApp::getInstance().removeProgressBar(m_progressBar);
		m_progressBar = NULL;
	}
}

void StelAddOnMgr::cancelAllDownloads()
{
	qDebug() << "Add-On Mgr: Canceling all downloads!";
	finishCurrentDownload();
	m_iDownloadingId = 0;
	m_downloadQueue.clear();
	emit(updateTableViews());
}
