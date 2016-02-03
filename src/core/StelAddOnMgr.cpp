/*
 * Stellarium
 * Copyright (C) 2014-2016 Marcos Cardinot
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
#include <qzipreader.h>

#include "LandscapeMgr.hpp"
#include "StelApp.hpp"
#include "StelAddOnMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelProgressController.hpp"

StelAddOnMgr::StelAddOnMgr()
	: m_pConfig(StelApp::getInstance().getSettings())
	, m_downloadingAddOn(NULL)
	, m_pAddOnNetworkReply(NULL)
	, m_currentDownloadFile(NULL)
	, m_sAddOnDir(StelFileMgr::getUserDir() % "/addon/")
	, m_sThumbnailDir(m_sAddOnDir % "thumbnail/")
	, m_sInstalledAddonsJsonPath(m_sAddOnDir % "installed_addons.json")
	, m_progressBar(NULL)
	, m_lastUpdate(QDateTime::fromString("2016-01-01", "yyyy-MM-dd"))
	, m_iUpdateFrequencyDays(7)
{
	QStringList v = StelUtils::getApplicationVersion().split('.');
	m_sAddonJsonFilename = QString("addons_%1.%2.json").arg(v.at(0)).arg(v.at(1));
	m_sAddonJsonPath = m_sAddOnDir % m_sAddonJsonFilename;
	m_sUserAddonJsonPath = m_sAddOnDir % "user_" % m_sAddonJsonFilename;

	m_sUrlUpdate = "http://cardinot.sourceforge.net/" % m_sAddonJsonFilename;

	// Set user agent as "Stellarium/$version$ ($platform$)"
	m_userAgent = QString("Stellarium/%1 (%2)")
			.arg(StelUtils::getApplicationVersion())
			.arg(StelUtils::getOperatingSystemInfo())
			.toLatin1();

	// creating addon dir
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sAddOnDir);
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sThumbnailDir);

	// Initialize settings in the main config file
	if (m_pConfig->childGroups().contains("AddOn"))
	{
		m_pConfig->beginGroup("AddOn");
		m_lastUpdate = m_pConfig->value("last_update", m_lastUpdate).toDateTime();
		m_iUpdateFrequencyDays = m_pConfig->value("upload_frequency_days", m_iUpdateFrequencyDays).toInt();
		m_sUrlUpdate = m_pConfig->value("url", m_sUrlUpdate).toString();
		m_pConfig->endGroup();
	}
	else // If no settings were found, create it with default values
	{
		qDebug() << "[Add-on] The main config file does not have an AddOn section - creating with defaults";
		m_pConfig->beginGroup("AddOn");
		// delete all existing settings...
		m_pConfig->remove("");
		m_pConfig->setValue("last_update", m_lastUpdate);
		m_pConfig->setValue("upload_frequency_days", m_iUpdateFrequencyDays);
		m_pConfig->setValue("url", m_sUrlUpdate);
		m_pConfig->endGroup();
	}

	connect(this, SIGNAL(dataUpdated(AddOn*)), this, SLOT(slotDataUpdated(AddOn*)));

	// loading json files
	reloadCatalogues();
}

StelAddOnMgr::~StelAddOnMgr()
{
}

void StelAddOnMgr::reloadCatalogues()
{
	// clear all hashes
	m_addonsInstalled.clear();
	m_addonsAvailable.clear();
	m_addonsToUpdate.clear();

	// load catalog of installed addons (~/.stellarium/installed_addons.json)
	m_addonsInstalled = loadAddonCatalog(m_sInstalledAddonsJsonPath);

	// load oficial catalog ~/.stellarium/addon_x.x.json
	m_addonsAvailable = loadAddonCatalog(m_sAddonJsonPath);
	if (m_addonsAvailable.isEmpty())
	{
		restoreDefaultAddonJsonFile();
		m_addonsAvailable = loadAddonCatalog(m_sAddonJsonPath); // load again
	}

	// load user catalog ~/.stellarium/user_addon_x.x.json
	m_addonsAvailable.unite(loadAddonCatalog(m_sUserAddonJsonPath));

	// removing the installed addons from 'm_addonsAvailable' hash
	QHashIterator<QString, AddOn*> i(m_addonsInstalled);
	while (i.hasNext())
	{
		i.next();
		QString addonId = i.key();
		AddOn* addonInstalled = i.value();
		AddOn* addonAvailable = m_addonsAvailable.value(addonId);
		if (!addonAvailable)
		{
			continue;
		}
		else if (addonInstalled->getChecksum() == addonAvailable->getChecksum() ||
			 addonInstalled->getVersion() >= addonAvailable->getVersion())
		{
			m_addonsAvailable.remove(addonId);
		}
		else if (addonInstalled->getVersion() < addonAvailable->getVersion())
		{
			m_addonsAvailable.remove(addonId);
			m_addonsToUpdate.insert(addonId, addonAvailable);
		}
	}

	// download thumbnails
	refreshThumbnailQueue();

	emit(updateTableViews());
}

QHash<QString, AddOn*> StelAddOnMgr::loadAddonCatalog(QString jsonPath) const
{
	QHash<QString, AddOn*> addons;
	QFile jsonFile(jsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Add-on] Cannot open the catalog!"
			   << QDir::toNativeSeparators(jsonPath);
		return addons;
	}

	QJsonObject json(QJsonDocument::fromJson(jsonFile.readAll()).object());
	jsonFile.close();

	if (json["name"].toString() != "Add-ons Catalog" ||
		json["format"].toInt() != ADDON_MANAGER_CATALOG_VERSION)
	{
		qWarning()  << "[Add-on] The current catalog is not compatible!";
		return addons;
	}

	qDebug() << "[Add-on] loading catalog file:"
		 << QDir::toNativeSeparators(jsonPath);

	QVariantMap map = json["add-ons"].toObject().toVariantMap();
	QVariantMap::iterator i;
	for (i = map.begin(); i != map.end(); ++i)
	{
		AddOn* addon = new AddOn(i.key(), i.value().toMap());
		if (addon && addon->isValid())
		{
			addons.insert(addon->getAddOnId(), addon);
		}
	}

	return addons;
}

void StelAddOnMgr::restoreDefaultAddonJsonFile()
{
	QFile defaultJson(StelFileMgr::getInstallationDir() % "/data/default_" % m_sAddonJsonFilename);
	QFile(m_sAddonJsonPath).remove(); // always overwrite
	if (defaultJson.copy(m_sAddonJsonPath))
	{
		qDebug() << "[Add-on] default_" % m_sAddonJsonFilename % " was copied to " % m_sAddonJsonPath;
		QFile jsonFile(m_sAddonJsonPath);
		jsonFile.setPermissions(jsonFile.permissions() | QFile::WriteOwner);
		// cleaning last_update var
		m_pConfig->remove("AddOn/last_update");
		m_lastUpdate = QDateTime::fromString("2016-01-01", "yyyy-MM-dd");
	}
	else
	{
		qWarning() << "[Add-on] cannot copy JSON resource to"
			   << QDir::toNativeSeparators(m_sAddonJsonPath);
	}
}

void StelAddOnMgr::setLastUpdate(QDateTime lastUpdate)
{
	m_lastUpdate = lastUpdate;
	// update config file
	m_pConfig->beginGroup("AddOn");
	m_pConfig->setValue("last_update", m_lastUpdate);
	m_pConfig->endGroup();
}

void StelAddOnMgr::setUpdateFrequencyDays(int days)
{
	m_iUpdateFrequencyDays = days;
	// update config file
	m_pConfig->beginGroup("AddOn");
	m_pConfig->setValue("upload_frequency_days", m_iUpdateFrequencyDays);
	m_pConfig->endGroup();
}

void StelAddOnMgr::refreshThumbnailQueue()
{
	QHashIterator<QString, AddOn*> aos(m_addonsAvailable);
	while (aos.hasNext())
	{
		aos.next();
		m_thumbnails.insert(aos.key(), aos.value()->getThumbnail());
		if (!QFile(m_sThumbnailDir % aos.key() % ".png").exists())
		{
			m_thumbnailQueue.append(aos.value()->getThumbnail());
		}
	}
	downloadNextThumbnail();
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

void StelAddOnMgr::updateAddons(QSet<AddOn *> addons)
{
	// TODO
}

void StelAddOnMgr::installAddons(QSet<AddOn *> addons)
{
	foreach (AddOn* addon, addons) {
		installAddOn(addon);
	}
}

void StelAddOnMgr::removeAddons(QSet<AddOn *> addons)
{
	foreach (AddOn* addon, addons) {
		removeAddOn(addon);
	}
}

void StelAddOnMgr::installAddOnFromFile(QString filePath)
{
	AddOn* addon = getAddOnFromZip(filePath);
	if (addon == NULL || !addon->isValid())
	{
		return;
	}
	else if (!filePath.startsWith(m_sAddOnDir))
	{
		if (!QFile(filePath).copy(addon->getZipPath()))
		{
			qWarning() << "[Add-on] Unable to copy"
				   << addon->getAddOnId() << "to"
				   << addon->getZipPath();
			return;
		}
	}

	// checking if this addonId is in the catalog
	AddOn* addonInHash = m_addonsAvailable.value(addon->getAddOnId());
	addonInHash = addonInHash ? addonInHash : m_addonsToUpdate.value(addon->getAddOnId());

	if (addonInHash)
	{
		// addonId exists, but file is different!
		// do not install it! addonIds must be unique.
		if (addon->getChecksum() != addonInHash->getChecksum())
		{
			// TODO: asks the user if he wants to overwrite?
			qWarning() << "[Add-on] An addon ("
				   << addon->getTypeString()
				   << ") with the ID"
				   << addon->getAddOnId()
				   << "already exists. Aborting installation!";
			return;
		}
		else
		{
			// same file! just install it!
			installAddOn(addonInHash, false);
		}
	}
	else
	{
		installAddOn(addon, false);
		if (addon->getStatus() == AddOn::FullyInstalled)
		{
			insertAddonInJson(addon, m_sUserAddonJsonPath);
			m_addonsInstalled.insert(addon->getAddOnId(), addon);
		}
	}
}

void StelAddOnMgr::installAddOn(AddOn* addon, bool tryDownload)
{
	if (m_downloadQueue.contains(addon))
	{
		return;
	}
	else if (!addon || !addon->isValid())
	{
		qWarning() << "[Add-on] Unable to install"
			   << QDir::toNativeSeparators(addon->getZipPath())
			   << "AddOn is not compatible!";
		return;
	}

	QFile file(addon->getZipPath());
	// checking if we have this file in the add-on dir (local disk)
	if (!file.exists())
	{
		addon->setStatus(AddOn::NotInstalled);
	}
	// only accept zip archive
	else if (!addon->getZipPath().endsWith(".zip"))
	{
		addon->setStatus(AddOn::InvalidFormat);
		qWarning() << "[Add-on] Error" << addon->getAddOnId()
			   << "The file found is not a .zip archive";
	}
	// checking integrity
	else if (addon->getChecksum() != calculateMd5(file))
	{
		addon->setStatus(AddOn::Corrupted);
		qWarning() << "[Add-on] Error: File "
			   << QDir::toNativeSeparators(addon->getZipPath())
			   << " is corrupt, MD5 mismatch!";
	}
	else
	{
		// installing files
		addon->setStatus(AddOn::Installing);
		emit (dataUpdated(addon));
		unzip(*addon);
		// remove zip archive from ~/.stellarium/addon/
		QFile(addon->getZipPath()).remove();
	}

	// require restart
	if ((addon->getStatus() == AddOn::PartiallyInstalled || addon->getStatus() == AddOn::FullyInstalled) &&
		(addon->getType() == AddOn::PLUGIN_CATALOG || addon->getType() == AddOn::STAR_CATALOG ||
		 addon->getType() == AddOn::LANG_SKYCULTURE || addon->getType() == AddOn::LANG_STELLARIUM ||
		 addon->getType() == AddOn::TEXTURE))
	{
		addon->setStatus(AddOn::Restart);
		emit (addOnMgrMsg(RestartRequired));
	}
	// something goes wrong (file not found OR corrupt).
	// if applicable, try downloading it...
	else if (tryDownload && (addon->getStatus() == AddOn::NotInstalled || addon->getStatus() == AddOn::Corrupted))
	{
		addon->setStatus(AddOn::Installing);
		m_downloadQueue.append(addon);
		downloadNextAddOn();
	}

	reloadCatalogues();
	emit (dataUpdated(addon));
}

void StelAddOnMgr::removeAddOn(AddOn* addon)
{
	if (!addon || !addon->isValid())
	{
		return;
	}

	addon->setStatus(AddOn::NotInstalled);
	QStringList installedFiles = addon->getInstalledFiles();
	foreach (QString filePath, addon->getInstalledFiles()) {
		QFile file(filePath);
		if (!file.exists() || file.remove())
		{
			installedFiles.removeOne(filePath);
			QDir dir(filePath);
			dir.cdUp();
			QString dirName = dir.dirName();
			dir.cdUp();
			dir.rmdir(dirName); // try to remove dir
		}
		else
		{
			addon->setStatus(AddOn::PartiallyInstalled);
			qWarning() << "[Add-on] Unable to remove"
				   << QDir::toNativeSeparators(filePath);
		}
	}

	if (addon->getStatus() == AddOn::NotInstalled)
	{
		removeAddonFromJson(addon, m_sInstalledAddonsJsonPath);
		qDebug() << "[Add-on] Successfully removed: " << addon->getAddOnId();
	}
	else if (addon->getStatus() == AddOn::PartiallyInstalled)
	{
		qWarning() << "[Add-on] Partially removed: " << addon->getAddOnId();
	}
	else
	{
		addon->setStatus(AddOn::UnableToRemove);
		qWarning() << "[Add-on] Unable to remove: " << addon->getAddOnId();
		return; // nothing changed
	}

	addon->setInstalledFiles(installedFiles);

	if (addon->getType() == AddOn::PLUGIN_CATALOG || addon->getType() == AddOn::STAR_CATALOG ||
		addon->getType() == AddOn::LANG_SKYCULTURE || addon->getType() == AddOn::LANG_STELLARIUM ||
		addon->getType() == AddOn::TEXTURE)
	{
		emit (addOnMgrMsg(RestartRequired));
		addon->setStatus(AddOn::Restart);
	}

	reloadCatalogues();
	emit (dataUpdated(addon));
}

AddOn* StelAddOnMgr::getAddOnFromZip(QString filePath)
{
	Stel::QZipReader reader(filePath);
	if (reader.status() != Stel::QZipReader::NoError)
	{
		qWarning() << "StelAddOnMgr: Unable to open the ZIP archive:"
			   << QDir::toNativeSeparators(filePath);
		return NULL;
	}

	foreach(Stel::QZipReader::FileInfo info, reader.fileInfoList())
	{
		if (!info.isFile || !info.filePath.endsWith("info.json"))
		{
			continue;
		}

		QByteArray data = reader.fileData(info.filePath);
		if (!data.isEmpty())
		{
			QJsonObject json(QJsonDocument::fromJson(data).object());
			qDebug() << "[Add-on] loading catalog file:"
				 << QDir::toNativeSeparators(info.filePath);

			QString addonId = json.keys().at(0);
			QVariantMap attributes = json.value(addonId).toObject().toVariantMap();
			QFile zipFile(filePath);
			QString md5sum = calculateMd5(zipFile);
			attributes.insert("checksum", md5sum);
			attributes.insert("download-size", zipFile.size() / 1024.0);
			attributes.insert("download-filename", QFileInfo(zipFile).fileName());

			return new AddOn(addonId, attributes);
		}
	}
	return NULL;
}

QList<AddOn*> StelAddOnMgr::scanFilesInAddOnDir()
{
	// check if there is any zip archives in the ~/.stellarium/addon dir
	QList<AddOn*> addons;
	QDir dir(m_sAddOnDir);
	dir.setFilter(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
	dir.setNameFilters(QStringList("*.zip"));
	foreach (QFileInfo fileInfo, dir.entryInfoList())
	{
		AddOn* addon = getAddOnFromZip(fileInfo.absoluteFilePath());
		if (addon)	// get all addons, even the imcompatibles
		{
			addons.append(addon);
		}
	}
	return addons;
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

void StelAddOnMgr::downloadNextAddOn()
{
	if (m_downloadingAddOn)
	{
		return;
	}

	Q_ASSERT(m_pAddOnNetworkReply==NULL);
	Q_ASSERT(m_currentDownloadFile==NULL);
	Q_ASSERT(m_progressBar==NULL);

	m_downloadingAddOn = m_downloadQueue.first();
	m_currentDownloadFile = new QFile(m_downloadingAddOn->getZipPath());
	if (!m_currentDownloadFile->open(QIODevice::WriteOnly))
	{
		qWarning() << "[Add-on] Can't open a writable file: "
			   << QDir::toNativeSeparators(m_downloadingAddOn->getZipPath());
		cancelAllDownloads();
		emit (addOnMgrMsg(UnableToWriteFiles));
		return;
	}

	QNetworkRequest req(m_downloadingAddOn->getDownloadURL());
	req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, false);
	req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	req.setRawHeader("User-Agent", m_userAgent);
	m_pAddOnNetworkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(m_pAddOnNetworkReply, SIGNAL(readyRead()), this, SLOT(newDownloadedData()));
	connect(m_pAddOnNetworkReply, SIGNAL(finished()), this, SLOT(downloadAddOnFinished()));

	m_progressBar = StelApp::getInstance().addProgressBar();
	m_progressBar->setValue(0);
	m_progressBar->setRange(0, m_downloadingAddOn->getDownloadSize());
	m_progressBar->setFormat(QString("%1: %p%").arg(m_downloadingAddOn->getDownloadFilename()));
}

void StelAddOnMgr::newDownloadedData()
{
	Q_ASSERT(m_currentDownloadFile);
	Q_ASSERT(m_pAddOnNetworkReply);
	Q_ASSERT(m_progressBar);

	float size = m_pAddOnNetworkReply->bytesAvailable();
	m_progressBar->setValue((float)m_progressBar->getValue()+(float)size);
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
		installAddOn(m_downloadingAddOn, false);
	}
	else
	{
		qWarning() << "Add-on Mgr: FAILED to download" << m_pAddOnNetworkReply->url()
			   << " Error:" << m_pAddOnNetworkReply->errorString();
		m_downloadingAddOn->setStatus(AddOn::DownloadFailed);
		emit (dataUpdated(m_downloadingAddOn));
		finishCurrentDownload();
	}

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

	m_downloadQueue.removeOne(m_downloadingAddOn);
}

void StelAddOnMgr::cancelAllDownloads()
{
	qDebug() << "[Add-on] Canceling all downloads!";
	finishCurrentDownload();
	m_downloadingAddOn = NULL;
	m_downloadQueue.clear();
	emit(updateTableViews());
}

void StelAddOnMgr::unzip(AddOn& addon)
{
	Stel::QZipReader reader(addon.getZipPath());
	if (reader.status() != Stel::QZipReader::NoError)
	{
		qWarning() << "StelAddOnMgr: Unable to open the ZIP archive:"
			   << QDir::toNativeSeparators(addon.getZipPath());
		addon.setStatus(AddOn::UnableToRead);
	}

	QStringList installedFiles = addon.getInstalledFiles();
	addon.setStatus(AddOn::FullyInstalled);
	foreach(Stel::QZipReader::FileInfo info, reader.fileInfoList())
	{
		if (!info.isFile || info.filePath.contains("info.json"))
		{
			continue;
		}

		QStringList validDirs;
		validDirs << "landscapes/" << "modules/" << "scripts/" << "skycultures/"
			  << "stars/" << "textures/" << "translations/";
		bool validDir = false;
		foreach (QString dir, validDirs)
		{
			if (info.filePath.startsWith(dir))
			{
				validDir = true;
				break;
			}
		}

		if (!validDir)
		{
			qWarning() << "StelAddOnMgr: Unable to install! Invalid destination"
				   << info.filePath;
			addon.setStatus(AddOn::InvalidDestination);
			return;
		}

		QFileInfo fileInfo(StelFileMgr::getUserDir() % "/" % info.filePath);
		StelFileMgr::makeSureDirExistsAndIsWritable(fileInfo.absolutePath());
		QFile file(fileInfo.absoluteFilePath());

		file.remove(); // overwrite
		QByteArray data = reader.fileData(info.filePath);
		if (!file.open(QIODevice::WriteOnly))
		{
			qWarning() << "StelAddOnMgr: cannot open file"
				   << QDir::toNativeSeparators(info.filePath);
			addon.setStatus(AddOn::UnableToWrite);
			continue;
		}
		file.write(data);
		file.close();
		installedFiles.append(fileInfo.absoluteFilePath());
		qDebug() << "[Add-on] New file installed:" << info.filePath;
	}
	installedFiles.removeDuplicates();
	addon.setInstalledFiles(installedFiles);
	insertAddonInJson(&addon, m_sInstalledAddonsJsonPath);
}

void StelAddOnMgr::insertAddonInJson(AddOn* addon, QString jsonPath)
{
	QFile jsonFile(jsonPath);
	if (jsonFile.open(QIODevice::ReadWrite))
	{
		QJsonObject attributes;
		attributes.insert("type", addon->getTypeString());
		attributes.insert("title", addon->getTitle());
		attributes.insert("description", addon->getDescription());
		attributes.insert("version", addon->getVersion().toString("yyyy.MM.dd"));
		attributes.insert("license", addon->getLicenseName());
		attributes.insert("license-url", addon->getLicenseURL());
		attributes.insert("download-url", addon->getDownloadURL());
		attributes.insert("download-filename", addon->getDownloadFilename());
		attributes.insert("download-size", addon->getDownloadSize());
		attributes.insert("checksum", addon->getChecksum());
		attributes.insert("thumbnail", addon->getThumbnail());
		attributes.insert("textures", addon->getAllTextures().join(","));

		attributes.insert("status", addon->getStatus());
		attributes.insert("installed-files", QJsonArray::fromStringList(addon->getInstalledFiles()));

		QJsonArray authors;
		foreach (AddOn::Authors a, addon->getAuthors())
		{
			QJsonObject author;
			author.insert("name", a.name);
			author.insert("email", a.email);
			author.insert("url", a.url);
			authors.append(author);
		}
		attributes.insert("authors", authors);

		QJsonObject json(QJsonDocument::fromJson(jsonFile.readAll()).object());
		json.insert("name", QString("Add-ons Catalog"));
		json.insert("format", ADDON_MANAGER_CATALOG_VERSION);

		QJsonObject addons = json["add-ons"].toObject();
		addons.insert(addon->getAddOnId(), attributes);
		json.insert("add-ons", addons);

		jsonFile.resize(0);
		jsonFile.write(QJsonDocument(json).toJson());
		jsonFile.close();
	}
	else
	{
		qWarning() << "Add-On Mgr: Couldn't open the user catalog of addons!"
			   << QDir::toNativeSeparators(m_sInstalledAddonsJsonPath);
	}
}

void StelAddOnMgr::removeAddonFromJson(AddOn *addon, QString jsonPath)
{
	QFile jsonFile(jsonPath);
	if (jsonFile.open(QIODevice::ReadWrite))
	{
		QJsonObject json(QJsonDocument::fromJson(jsonFile.readAll()).object());
		QJsonObject addons = json.take("add-ons").toObject();
		addons.remove(addon->getAddOnId());
		json.insert("add-ons", addons);

		jsonFile.resize(0);
		jsonFile.write(QJsonDocument(json).toJson());
		jsonFile.close();
	}
	else
	{
		qWarning() << "[Add-on] Unable to open the catalog: "
			   << QDir::toNativeSeparators(jsonPath);
	}
}

void StelAddOnMgr::slotDataUpdated(AddOn* addon)
{
	AddOn::Type type = addon->getType();
	if (type == AddOn::LANDSCAPE)
	{
		emit (landscapesChanged());
	}
	else if (type == AddOn::SCRIPT)
	{
		emit (scriptsChanged());
	}
	else if (type == AddOn::SKY_CULTURE)
	{
		emit (skyCulturesChanged());
	}
}
