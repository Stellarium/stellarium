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

QStringList AOSkyCulture::checkInstalledAddOns() const
{
	return QStringList();
}

bool AOSkyCulture::installFromFile(const QString& idInstall,
				   const QString& downloadFilepath) const
{
	QZipReader reader(downloadFilepath);
	if (reader.status() != QZipReader::NoError)
	{
		qWarning() << "Add-On SkyCultures: Unable to open the ZIP archive:"
			   << QDir::toNativeSeparators(downloadFilepath);
		return false;
	}

	if (!reader.extractAll(m_sSkyCultureInstallDir)) {
		qWarning() << "Add-On SkyCultures: Unable to install the new sky culture!";
		return false;
	}

	qWarning() << "Add-On SkyCultures: New sky culture installed!";
	return true;
}

bool AOSkyCulture::uninstallAddOn(const QString &idInstall) const
{
	return true;
}
