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

#include "AOSkyCulture.hpp"

AOSkyCulture::AOSkyCulture(StelAddOnDAO* pStelAddOnDAO)
	: m_pStelAddOnDAO(pStelAddOnDAO)
	, m_sSkyCultureInstallDir(StelFileMgr::getUserDir() % "/skycultures/")
{
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sSkyCultureInstallDir);
}

AOSkyCulture::~AOSkyCulture()
{
}

void AOSkyCulture::checkInstalledAddOns() const
{
}

void AOSkyCulture::installFromFile(const QString& filePath) const
{
	QZipReader reader(filePath);
	if (reader.status() != QZipReader::NoError)
	{
		qWarning() << "Add-On SkyCultures: Unable to open the ZIP archive:"
			   << QDir::toNativeSeparators(filePath);
		return;
	}

	QList<QZipReader::FileInfo> infoList = reader.fileInfoList();
	foreach(QZipReader::FileInfo info, infoList)
	{
		if (!info.isFile)
		{
			continue;
		}

		QString installedFilePath = m_sSkyCultureInstallDir % info.filePath;
		QFile(installedFilePath).remove();

		QByteArray data = reader.fileData(info.filePath);
		QFile out(installedFilePath);
		out.open(QIODevice::WriteOnly);
		out.write(data);
		out.close();

		qWarning() << "Add-On SkyCultures: New starlore installed:" << info.filePath;
	}
	m_pStelAddOnDAO->updateInstalledAddon(QFileInfo(filePath).fileName(), "1.0", "");
}

bool AOSkyCulture::uninstallAddOn(const StelAddOnDAO::AddOnInfo &addonInfo) const
{
	return true;
}
