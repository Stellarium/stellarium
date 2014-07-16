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

#include "AOTexture.hpp"

AOTexture::AOTexture(StelAddOnDAO* pStelAddOnDAO)
	: m_pStelAddOnDAO(pStelAddOnDAO)
	, m_sTexturesInstallDir(StelFileMgr::getInstallationDir() % "/textures/")
{
	if (!QDir(m_sTexturesInstallDir).exists())
	{
		qWarning() << "Add-On Texture: Unable to find the textures directory:"
			   << QDir::toNativeSeparators(m_sTexturesInstallDir);
	}
}

AOTexture::~AOTexture()
{
}

QStringList AOTexture::checkInstalledAddOns() const
{
	// TODO: users can change the textures externally,
	// so probably we'll need to compare md5hashes to be able
	// to know which are the installed textures
	return QStringList();
}

bool AOTexture::installFromFile(const QString& idInstall,
				const QString& downloadFilepath) const
{
	QZipReader reader(downloadFilepath);
	if (reader.status() != QZipReader::NoError)
	{
		qWarning() << "Add-On Texture: Unable to open the ZIP archive:"
			   << QDir::toNativeSeparators(downloadFilepath);
		return false;
	}

	QList<QZipReader::FileInfo> infoList = reader.fileInfoList();
	foreach(QZipReader::FileInfo info, infoList)
	{
		if (!info.isFile)
		{
			continue;
		}

		QString installedFilePath = m_sTexturesInstallDir % info.filePath;
		QFile installedFile(installedFilePath);
		if (!installedFile.exists())
		{
			qWarning() << "Add-On Texture: Unable to find the file:"
				   << QDir::toNativeSeparators(installedFilePath);
			continue;
		}

		if (!installedFile.remove())
		{
			qWarning() << "Add-On Texture: Unable to overwrite :"
				   << QDir::toNativeSeparators(installedFilePath);
			continue;
		}

		QByteArray data = reader.fileData(info.filePath);
		QFile out(installedFilePath);
		out.open(QIODevice::WriteOnly);
		out.write(data);
		out.close();

		qWarning() << "Add-On Texture: New texture installed:" << info.filePath;
	}

	return true;
}

bool AOTexture::uninstallAddOn(const QString &idInstall) const
{
	return true;
}
