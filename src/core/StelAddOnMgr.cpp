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
	, m_sAddonJsonFilename("addons_0.14.0.json")
	, m_sAddonJsonPath(m_sAddOnDir % m_sAddonJsonFilename)
	, m_sInstalledAddonsJsonPath(m_sAddOnDir % "installed_addons.json")
	, m_progressBar(NULL)
	, m_iLastUpdate(1388966410)
	, m_iUpdateFrequencyDays(7)
	, m_iUpdateFrequencyHour(12)
	, m_sUrlUpdate("http://cardinot.sourceforge.net/" % m_sAddonJsonFilename)
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
	if (!QFile(m_sAddonJsonPath).exists())
	{
		qWarning() << "Add-On Mgr: The catalog does not exist - copying default file to "
			   << QDir::toNativeSeparators(m_sAddonJsonPath);
		restoreDefaultAddonJsonFile();
	}

	connect(this, SIGNAL(dataUpdated(AddOn*)), this, SLOT(slotDataUpdated(AddOn*)));

	// loading json file
	reloadAddonJsonFile();
}

StelAddOnMgr::~StelAddOnMgr()
{
}

void StelAddOnMgr::reloadAddonJsonFile()
{
	QFile jsonFile(m_sAddonJsonPath);
	if (jsonFile.open(QIODevice::ReadOnly))
	{
		QJsonObject json(QJsonDocument::fromJson(jsonFile.readAll()).object());
		if (json["name"].toString() != "Add-Ons Catalog" ||
			json["format-version"].toInt() != ADDON_MANAGER_CATALOG_VERSION)
		{
			qWarning()  << "Add-On Mgr: The current catalog is not compatible - using default file";
			restoreDefaultAddonJsonFile();
		}
		qDebug() << "Add-On Mgr: loading catalog file:"
			 << QDir::toNativeSeparators(m_sAddonJsonPath);
		readAddonJsonObject(json["add-ons"].toObject());
	}
	else
	{
		qWarning() << "Add-On Mgr: Couldn't open the catalog!"
			   << QDir::toNativeSeparators(m_sAddonJsonPath);
	}

	refreshThumbnailQueue();

	// refresh add-ons statuses (it checks which are installed or not)
	refreshAddOnStatuses();
}

void StelAddOnMgr::restoreDefaultAddonJsonFile()
{
	QFile defaultJson(StelFileMgr::getInstallationDir() % "/data/default_" % m_sAddonJsonFilename);
	if (defaultJson.copy(m_sAddonJsonPath))
	{
		qDebug() << "Add-On Mgr: default_" % m_sAddonJsonFilename % " was copied to " % m_sAddonJsonPath;
		QFile jsonFile(m_sAddonJsonPath);
		jsonFile.setPermissions(jsonFile.permissions() | QFile::WriteOwner);
		// cleaning last_update var
		m_pConfig->remove("AddOn/last_update");
		m_iLastUpdate = 1388966410;
	}
	else
	{
		qWarning() << "Add-On Mgr: cannot copy JSON resource to"
			   << QDir::toNativeSeparators(m_sAddonJsonPath);
	}
}

void StelAddOnMgr::readAddonJsonObject(const QJsonObject& addOns)
{
	QVariantMap map = addOns.toVariantMap();
	int duplicated = addOns.size() - map.size();
	if (duplicated)
	{
		qWarning() << "Add-On Mgr : Error! The catalog has"
			   << duplicated << "duplicated keys!";
	}

	QVariantMap::iterator i;
	for (i = map.begin(); i != map.end(); ++i)
	{
		AddOn* addOn(new AddOn(i.key(), i.value().toMap()));
		if (addOn->isValid())
		{
			AddOnMap amap;
			if (m_addons.contains(addOn->getType()))
			{
				amap = m_addons.value(addOn->getType());
			}
			amap.insert(addOn->getAddOnId(), addOn);
			m_addons.insert(addOn->getType(), amap);
			m_addonsByMd5.insert(addOn->getChecksum(), addOn);
			m_addonsById.insert(addOn->getAddOnId(), addOn);
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
	// check add-ons which are already installed (installed_addons.json)
	QFile jsonFile(m_sInstalledAddonsJsonPath);
	if (jsonFile.open(QIODevice::ReadOnly))
	{
		QJsonObject object(QJsonDocument::fromJson(jsonFile.readAll()).object());
		QVariantMap map = object.toVariantMap();
		QVariantMap::iterator i;
		for (i = map.begin(); i != map.end(); ++i)
		{
			AddOn* addOn = m_addonsById.value(i.key());
			QVariantMap attributes(i.value().toMap());
			if (addOn && addOn->getChecksum() == attributes.value("checksum").toString())
			{
				addOn->setInstalledFiles(attributes.value("installed-files").toStringList());
				addOn->setStatus((AddOn::Status)attributes.value("status").toInt());
			}
		}
	}
	else
	{
		qWarning() << "Add-On Mgr: Couldn't open the catalog of installed addons!"
			   << QDir::toNativeSeparators(m_sAddonJsonPath);
	}
}

void StelAddOnMgr::refreshThumbnailQueue()
{
	QHashIterator<QString, AddOn*> aos(m_addonsById);
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

void StelAddOnMgr::installAddOn(AddOn* addon, const QStringList selectedFiles)
{
	if (!addon || !addon->isValid() || m_downloadQueue.contains(addon))
	{
		return;
	}

	installFromFile(addon, selectedFiles);
	if (addon->getStatus() == AddOn::NotInstalled || addon->getStatus() == AddOn::Corrupted)
	{
		// something goes wrong (file not found OR corrupt),
		// try downloading it...
		addon->setStatus(AddOn::Installing);
		emit (dataUpdated(addon));
		m_downloadQueue.insert(addon, selectedFiles);
		downloadNextAddOn();
	}
}

void StelAddOnMgr::installFromFile(AddOn* addon, const QStringList selectedFiles)
{
	QFile file(addon->getDownloadFilepath());
	// checking if we have this file in the add-on dir (local disk)
	if (!file.exists())
	{
		addon->setStatus(AddOn::NotInstalled);
		return;
	}

	// only accept zip archive
	if (!addon->getDownloadFilepath().endsWith(".zip"))
	{
		qWarning() << "Add-On Mgr: Error" << addon->getAddOnId()
			   << "The file found is not a .zip archive";
		addon->setStatus(AddOn::InvalidFormat);
		return;
	}

	// checking integrity
	if (addon->getChecksum() == calculateMd5(file))
	{
		// installing files
		unzip(*addon, selectedFiles);
	}
	else
	{
		addon->setStatus(AddOn::Corrupted);
		qWarning() << "Add-On Mgr: Error: File "
			   << addon->getDownloadFilepath()
			   << " is corrupt, MD5 mismatch!";
	}

	if ((addon->getStatus() == AddOn::PartiallyInstalled || addon->getStatus() == AddOn::FullyInstalled) &&
		(addon->getCategory() == AddOn::CATALOG || addon->getCategory() == AddOn::LANGUAGEPACK
			|| addon->getCategory() == AddOn::TEXTURE))
	{
		emit (addOnMgrMsg(RestartRequired));
		addon->setStatus(AddOn::Restart);
	}

	emit (dataUpdated(addon));
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

void StelAddOnMgr::removeAddOn(AddOn* addon, QStringList selectedFiles)
{
	if (!addon || !addon->isValid())
	{
		return;
	}

	if (selectedFiles.isEmpty()) // remove all files
	{
		selectedFiles = addon->getInstalledFiles();
	}

	QStringList installedFiles = addon->getInstalledFiles();
	foreach (QString filePath, selectedFiles) {
		QFile file(filePath);
		if (file.remove())
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
			qWarning() << "Add-On Mgr : Unable to remove"
				   << QDir::toNativeSeparators(filePath);
		}

	}

	if (installedFiles.size() == 0)
	{
		qDebug() << "Add-On Mgr : Successfully removed" << addon->getAddOnId();
		addon->setStatus(AddOn::NotInstalled);
	}
	else if (addon->getInstalledFiles().size() > installedFiles.size())
	{
		qWarning() << "Add-On Mgr : Partially removed" << addon->getAddOnId();
		addon->setStatus(AddOn::PartiallyInstalled);
	}
	else
	{
		addon->setStatus(AddOn::UnableToRemove);
		return; // nothing changed
	}

	addon->setInstalledFiles(installedFiles);
	updateInstalledAddonsJson(addon);

	if (addon->getCategory() == AddOn::CATALOG || addon->getCategory() == AddOn::LANGUAGEPACK
			|| addon->getCategory() == AddOn::TEXTURE)
	{
		emit (addOnMgrMsg(RestartRequired));
		addon->setStatus(AddOn::Restart);
	}
	emit (dataUpdated(addon));
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
	if (first.isEmpty() && last.isEmpty()) {
		return true;
	}

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
		installFromFile(m_downloadingAddOn, m_downloadQueue.value(m_downloadingAddOn));
	}
	else
	{
		qWarning() << "Add-on Mgr: FAILED to download" << m_pAddOnNetworkReply->url()
			   << " Error:" << m_pAddOnNetworkReply->errorString();
		m_downloadingAddOn->setStatus(AddOn::DownloadFailed);
		emit (dataUpdated(m_downloadingAddOn));
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

void StelAddOnMgr::unzip(AddOn& addon, QStringList selectedFiles)
{
	QZipReader reader(addon.getDownloadFilepath());
	if (reader.status() != QZipReader::NoError)
	{
		qWarning() << "StelAddOnMgr: Unable to open the ZIP archive:"
			   << QDir::toNativeSeparators(addon.getDownloadFilepath());
		addon.setStatus(AddOn::UnableToRead);
	}

	QStringList installedFiles = addon.getInstalledFiles();
	addon.setStatus(AddOn::FullyInstalled);
	foreach(QZipReader::FileInfo info, reader.fileInfoList())
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

		// when selectedFiles is empty, extract all files
		if (!selectedFiles.isEmpty())
		{
			if (!selectedFiles.contains(fileInfo.filePath()) && !file.exists())
			{
				addon.setStatus(AddOn::PartiallyInstalled);
				continue;
			}
		}

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
		qDebug() << "StelAddOnMgr: New file installed:" << info.filePath;
	}
	installedFiles.removeDuplicates();
	addon.setInstalledFiles(installedFiles);
	updateInstalledAddonsJson(&addon);
}

void StelAddOnMgr::updateInstalledAddonsJson(AddOn* addon)
{
	QFile jsonFile(m_sInstalledAddonsJsonPath);
	if (jsonFile.open(QIODevice::ReadWrite))
	{
		QJsonObject object(QJsonDocument::fromJson(jsonFile.readAll()).object());
		if (addon->getStatus() == AddOn::NotInstalled)
		{
			object.remove(addon->getAddOnId());
		}
		else
		{
			QJsonObject attributes;
			attributes.insert("checksum", addon->getChecksum());
			attributes.insert("status", addon->getStatus());
			attributes.insert("installed-files", QJsonArray::fromStringList(addon->getInstalledFiles()));
			object.insert(addon->getAddOnId(), attributes);
		}
		jsonFile.resize(0);
		jsonFile.write(QJsonDocument(object).toJson());
		jsonFile.close();
	}
	else
	{
		qWarning() << "Add-On Mgr: Couldn't open the catalog of installed addons!"
			   << QDir::toNativeSeparators(m_sInstalledAddonsJsonPath);
	}
}

void StelAddOnMgr::slotDataUpdated(AddOn* addon)
{
	AddOn::Category cat = addon->getCategory();
	if (cat == AddOn::LANDSCAPE)
	{
		emit (landscapesChanged());
	}
	else if (cat == AddOn::SCRIPT)
	{
		emit (scriptsChanged());
	}
	else if (cat == AddOn::STARLORE)
	{
		emit (skyCulturesChanged());
	}
}
