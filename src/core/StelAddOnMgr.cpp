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
#include <QJsonArray>
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
	, m_downloadingAddOn(NULL)
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

	// creating sub-dirs
	m_dirs.insert(AddOn::CATALOG, m_sAddOnDir % CATEGORY_CATALOG % "/");
	m_dirs.insert(AddOn::LANDSCAPE, m_sAddOnDir % CATEGORY_LANDSCAPE % "/");
	m_dirs.insert(AddOn::LANGUAGEPACK, m_sAddOnDir % CATEGORY_LANGUAGE_PACK % "/");
	m_dirs.insert(AddOn::SCRIPT, m_sAddOnDir % CATEGORY_SCRIPT % "/");
	m_dirs.insert(AddOn::STARLORE, m_sAddOnDir % CATEGORY_SKY_CULTURE % "/");
	m_dirs.insert(AddOn::TEXTURE, m_sAddOnDir % CATEGORY_TEXTURE % "/");
	QHashIterator<AddOn::Category, QString> it(m_dirs);
	while (it.hasNext()) {
		it.next();
		StelFileMgr::makeSureDirExistsAndIsWritable(it.value());
	}

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
		restoreDefaultJsonFile();
	}

	// loading json file
	QFile jsonFile(m_sJsonPath);
	if (jsonFile.open(QIODevice::ReadOnly))
	{
		QJsonObject json(QJsonDocument::fromJson(jsonFile.readAll()).object());
		if (json["name"].toString() != "Add-Ons Catalog" ||
			json["format-version"].toInt() != ADDON_MANAGER_CATALOG_VERSION)
		{
			qWarning()  << "Add-On Mgr: The current addons.json file is not compatible - using default file";
			restoreDefaultJsonFile();
		}
		qDebug() << "Add-On Mgr: loading catalog file:"
			 << QDir::toNativeSeparators(m_sJsonPath);
		readJsonObject(json["add-ons"].toObject());
	}
	else
	{
		qWarning() << "Add-On Mgr: Couldn't open addons.json file!"
			   << QDir::toNativeSeparators(m_sJsonPath);
	}

	// Init database
	m_pStelAddOnDAO->init();

	// Init sub-classes
	m_pStelAddOns.insert(AddOn::CATALOG, new AOCatalog());
	m_pStelAddOns.insert(AddOn::LANDSCAPE, new AOLandscape());
	m_pStelAddOns.insert(AddOn::LANGUAGEPACK, new AOLanguagePack());
	m_pStelAddOns.insert(AddOn::SCRIPT, new AOScript());
	m_pStelAddOns.insert(AddOn::STARLORE, new AOSkyCulture());
	m_pStelAddOns.insert(AddOn::TEXTURE, new AOTexture());

	connect(m_pStelAddOns.value(AddOn::STARLORE), SIGNAL(skyCulturesChanged()),
		this, SIGNAL(skyCulturesChanged()));

	// refresh add-ons statuses (it checks which are installed or not)
	refreshAddOnStatuses();
}

StelAddOnMgr::~StelAddOnMgr()
{
}

void StelAddOnMgr::restoreDefaultJsonFile()
{
	QFile defaultJson(StelFileMgr::getInstallationDir() % "/data/default_addons.json");
	if (defaultJson.copy(m_sJsonPath))
	{
		qDebug() << "Add-On Mgr: default_addons.json was copied to" << m_sJsonPath;
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

void StelAddOnMgr::readJsonObject(const QJsonObject& addOns)
{
	QVariantMap map = addOns.toVariantMap();
	int duplicated = addOns.size() - map.size();
	if (duplicated)
	{
		qWarning() << "Add-On Mgr : Error! The addons.json file has"
			   << duplicated << "duplicated keys!";
	}

	QVariantMap::iterator i;
	for (i = map.begin(); i != map.end(); ++i)
	{
		qint64 addOnId = i.key().toLongLong();
		QVariantMap attributes = i.value().toMap();
		AddOn* addOn(new AddOn(addOnId, attributes));
		if (addOn->isValid())
		{
			AddOnMap amap;
			amap.insert(addOnId, addOn);
			m_addons.insert(addOn->getType(), amap);
			m_addonsByMd5.insert(addOn->getChecksum(), addOn);
		}
	}
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
	QHashIterator<AddOn::Category, StelAddOn*> aos(m_pStelAddOns);
	while (aos.hasNext())
	{
		aos.next();
		QStringList list = aos.value()->checkInstalledAddOns();
		if (list.isEmpty())
		{
			continue;
		}

		if (aos.key() == AddOn::CATALOG || aos.key() == AddOn::LANGUAGEPACK)
		{
			m_pStelAddOnDAO->markAddOnsAsInstalledFromMd5(list);
		}
		else if (aos.key() == AddOn::TEXTURE)
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

void StelAddOnMgr::installAddOn(AddOn* addon, const QStringList selectedFiles)
{
	if (!addon || !addon->isValid() || m_downloadQueue.contains(addon))
	{
		return;
	}

	AddOn::Status s = installFromFile(addon, selectedFiles);
	if (s == AddOn::NotInstalled || s == AddOn::Corrupted)
	{
		// something goes wrong (file not found OR corrupt),
		// try downloading it...
		addon->setStatus(AddOn::Installing);
		emit (dataUpdated(addon->getCategory()));
		m_downloadQueue.insert(addon, selectedFiles);
		downloadNextAddOn();
	}
}

AddOn::Status StelAddOnMgr::installFromFile(AddOn* addon, const QStringList selectedFiles)
{
	QFile file(addon->getDownloadFilepath());
	// checking if we have this file in the add-on dir (local disk)
	if (!file.exists())
	{
		return AddOn::NotInstalled;
	}

	// checking integrity
	AddOn::Status status;
	if (addon->getChecksum() == calculateMd5(file))
	{
		// installing files
		status = m_pStelAddOns.value(addon->getCategory())
				->installFromFile(addon->getInstallId(),
						  addon->getDownloadFilepath(),
						  selectedFiles);
	}
	else
	{
		status = AddOn::Corrupted;
		qWarning() << "Add-On Mgr: Error: File "
			   << addon->getDownloadFilepath()
			   << " is corrupt, MD5 mismatch!";
	}

	if ((status == AddOn::PartiallyInstalled || status == AddOn::FullyInstalled) &&
		(addon->getCategory() == AddOn::LANGUAGEPACK || addon->getCategory() == AddOn::TEXTURE))
	{
		emit (addOnMgrMsg(RestartRequired));
	}

	addon->setStatus(status);
	emit (dataUpdated(addon->getCategory()));
	return status;
}

void StelAddOnMgr::installFromFile(const QString& filePath)
{
	QFile file(filePath);
	AddOn* addon = m_addonsByMd5.value(calculateMd5(file));
	if (!addon || !addon->isValid())
	{
		qWarning() << "Add-On InstallFromFile : Unable to install"
			   << filePath << "File is not compatible!";
		return;
	}
	installFromFile(addon, QStringList());
}

void StelAddOnMgr::removeAddOn(AddOn* addon, const QStringList selectedFiles)
{
	if (!addon || !addon->isValid())
	{
		return;
	}

	addon->setStatus(m_pStelAddOns.value(addon->getCategory())->uninstallAddOn(addon->getInstallId(), selectedFiles));
	emit (dataUpdated(addon->getCategory()));

	if (addon->getCategory() == AddOn::LANGUAGEPACK || addon->getCategory() == AddOn::TEXTURE)
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
	if (m_downloadingAddOn)
	{
		return;
	}

	Q_ASSERT(m_pAddOnNetworkReply==NULL);
	Q_ASSERT(m_currentDownloadFile==NULL);
	Q_ASSERT(m_progressBar==NULL);

	m_downloadingAddOn = m_downloadQueue.firstKey();
	m_currentDownloadFile = new QFile(m_downloadingAddOn->getDownloadFilepath());
	if (!m_currentDownloadFile->open(QIODevice::WriteOnly))
	{
		qWarning() << "Can't open a writable file: "
			   << QDir::toNativeSeparators(m_downloadingAddOn->getDownloadFilepath());
		cancelAllDownloads();
		emit (addOnMgrMsg(UnableToWriteFiles));
		return;
	}

	QNetworkRequest req(m_downloadingAddOn->getDownloadURL());
	req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
	m_pAddOnNetworkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	m_pAddOnNetworkReply->setReadBufferSize(1024*1024*2);
	connect(m_pAddOnNetworkReply, SIGNAL(readyRead()), this, SLOT(newDownloadedData()));
	connect(m_pAddOnNetworkReply, SIGNAL(finished()), this, SLOT(downloadAddOnFinished()));

	m_progressBar = StelApp::getInstance().addProgressBar();
	m_progressBar->setValue(0);
	m_progressBar->setRange(0, m_downloadingAddOn->getDownloadSize()*1024);
	m_progressBar->setFormat(QString("%1: %p%").arg(m_downloadingAddOn->getDownloadFilename()));
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
		installFromFile(m_downloadingAddOn, m_downloadQueue.value(m_downloadingAddOn));
	}
	else
	{
		qWarning() << "Add-on Mgr: FAILED to download" << m_pAddOnNetworkReply->url()
			   << " Error:" << m_pAddOnNetworkReply->errorString();
		m_pStelAddOnDAO->updateAddOnStatus(m_downloadingAddOn->getInstallId(), AddOn::DownloadFailed);
		emit (dataUpdated(m_downloadingAddOn->getCategory()));
		finishCurrentDownload();
	}

	m_downloadQueue.remove(m_downloadingAddOn);
	m_downloadingAddOn = NULL;
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
	m_downloadingAddOn = NULL;
	m_downloadQueue.clear();
	emit(updateTableViews());
}
