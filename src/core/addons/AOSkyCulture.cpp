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
	QDir dir(m_sSkyCultureInstallDir);
	dir.setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
	return dir.entryList();
}

bool AOSkyCulture::installFromFile(const QString& idInstall,
				   const QString& downloadedFilepath,
				   const QStringList& selectedFiles) const
{
	Q_UNUSED(selectedFiles); // not applicable - always install all files

	QZipReader reader(downloadedFilepath);
	if (reader.status() != QZipReader::NoError)
	{
		qWarning() << "Add-On SkyCultures: Unable to open the ZIP archive:"
			   << QDir::toNativeSeparators(downloadedFilepath);
		return false;
	}

	QString destination = m_sSkyCultureInstallDir % idInstall;
	StelFileMgr::makeSureDirExistsAndIsWritable(destination);

	if (!reader.extractAll(destination)) {
		qWarning() << "Add-On SkyCultures: Unable to install the new sky culture!";
		return false;
	}

	qWarning() << "Add-On SkyCultures: New sky culture" << idInstall << "installed!";
	emit(skyCulturesChanged());
	return true;
}

bool AOSkyCulture::uninstallAddOn(const QString &idInstall) const
{
	QDir dir(m_sSkyCultureInstallDir % idInstall);

	if (!dir.removeRecursively())
	{
		qWarning() << "Add-On SkyCultures : Error! " << idInstall
			   << "could not be removed. "
			   << "Some files were deleted, but not all."
			   << endl
			   << "Add-On SkyCultures : You can delete manually"
			   << dir.absolutePath();
		return false;
	}

	qDebug() << "Add-On SkyCultures : Successfully removed" << dir.absolutePath();
	emit(skyCulturesChanged());
	return true;
}
