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

#include <QCryptographicHash>
#include <QDirIterator>

#include "AOCatalog.hpp"

AOCatalog::AOCatalog(StelAddOnDAO* pStelAddOnDAO)
	: m_pStelAddOnDAO(pStelAddOnDAO)
	//, m_sStarCatalogInstallDir(StelFileMgr::getUserDir() % "/stars")
{
	//StelFileMgr::makeSureDirExistsAndIsWritable(m_sStarCatalogInstallDir);
}

AOCatalog::~AOCatalog()
{
}

QStringList AOCatalog::checkInstalledAddOns() const
{
	QStringList files;
	QDir::Filters filters(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
	QDirIterator it(StelFileMgr::getUserDir() % "/stars", filters, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		files.append(it.next());
	}
	QDirIterator it2(StelFileMgr::getUserDir() % "/modules", filters, QDirIterator::Subdirectories);
	while (it2.hasNext()) {
		files.append(it2.next());
	}

	QStringList checksums;
	foreach (QString filepath, files) {
		QFile file(filepath);
		if (file.open(QIODevice::ReadOnly)) {
			QCryptographicHash md5(QCryptographicHash::Md5);
			md5.addData(file.readAll());
			checksums.append(md5.result().toHex());
		}
	}

	return checksums;
}

bool AOCatalog::installFromFile(const QString& idInstall,
				const QString& downloadFilepath) const
{
	QString suffix = "." % QFileInfo(downloadFilepath).suffix();
	if (suffix != ".cat" && suffix != ".json")
	{
		qWarning() << "Add-On Catalog: Unable to intall" << idInstall
			   << "The file found is not a .cat or .json";
		return false;
	}

	QString destination = StelFileMgr::getUserDir() % "/" % idInstall % suffix;
	QFile(destination).remove();
	if (!QFile(downloadFilepath).copy(destination))
	{
		qWarning() << "Add-On Catalog: Unable to install" << idInstall;
		return false;
	}

	qDebug() << "Add-On Catalog: New catalog installed:" << idInstall;
	return true;
}

bool AOCatalog::uninstallAddOn(const QString &idInstall) const
{
	QString suffix = ".cat";
	if (idInstall.contains("modules"))
	{
		suffix = ".json";
	}
	QFile file(StelFileMgr::getUserDir() % "/" % idInstall % suffix);
	bool removed = true;
	if (file.exists())
	{
		removed = file.remove();
	}

	if (removed)
	{
		qDebug() << "Add-On Catalog : Successfully removed" << idInstall;
	}
	else
	{
		qWarning() << "Add-On Catalog : Error! " << idInstall
			   << "could not be removed. ";
	}

	return removed;
}
