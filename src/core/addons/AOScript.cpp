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

#include "AOScript.hpp"

AOScript::AOScript(StelAddOnDAO* pStelAddOnDAO)
	: m_pStelAddOnDAO(pStelAddOnDAO)
	, m_sScriptInstallDir(StelFileMgr::getUserDir() % "/scripts/")
{
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sScriptInstallDir);
}

AOScript::~AOScript()
{
}

QStringList AOScript::checkInstalledAddOns() const
{
	return QStringList();
}

bool AOScript::installFromFile(const QString& idInstall,
			       const QString& downloadFilepath) const
{
	QZipReader reader(downloadFilepath);
	if (reader.status() != QZipReader::NoError)
	{
		qWarning() << "Add-On Script: Unable to open the ZIP archive:"
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

		QString installedFilePath = m_sScriptInstallDir % info.filePath;
		QFile(installedFilePath).remove();

		QByteArray data = reader.fileData(info.filePath);
		QFile out(installedFilePath);
		out.open(QIODevice::WriteOnly);
		out.write(data);
		out.close();

		qWarning() << "Add-On Script: New script installed:" << info.filePath;
	}
	return true;
}

bool AOScript::uninstallAddOn(const QString &idInstall) const
{
	return true;
}
