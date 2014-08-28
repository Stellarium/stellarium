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

#include "AOLanguagePack.hpp"

AOLanguagePack::AOLanguagePack(StelAddOnDAO* pStelAddOnDAO)
	: m_pStelAddOnDAO(pStelAddOnDAO)
	, m_sLocaleInstallDir(StelFileMgr::getLocaleUserDir())
{
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sLocaleInstallDir % "/stellarium");
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sLocaleInstallDir % "/stellarium-skycultures");
}

AOLanguagePack::~AOLanguagePack()
{
}

QStringList AOLanguagePack::checkInstalledAddOns() const
{
	QDir::Filters filters(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
	QDir dir(m_sLocaleInstallDir % "/stellarium");
	QFileInfoList files = dir.entryInfoList(filters);

	dir = QDir(m_sLocaleInstallDir % "/stellarium-skycultures");
	files += dir.entryInfoList(filters);

	QStringList checksums;
	foreach (QFileInfo fileinfo, files) {
		QFile file(fileinfo.absoluteFilePath());
		if (file.open(QIODevice::ReadOnly)) {
			QCryptographicHash md5(QCryptographicHash::Md5);
			md5.addData(file.readAll());
			checksums.append(md5.result().toHex());
		}
	}

	return checksums;
}

int AOLanguagePack::installFromFile(const QString& idInstall,
				    const QString& downloadedFilepath,
				    const QStringList& selectedFiles) const
{
	Q_UNUSED(selectedFiles); // not applicable - always install all files

	if (!downloadedFilepath.endsWith(".qm"))
	{
		qWarning() << "Add-On Language: Unable to intall" << idInstall
			   << "The file found is not a .qm";
		return 0;
	}

	QString destination = m_sLocaleInstallDir % "/" % idInstall % ".qm";
	QFile(destination).remove();
	if (!QFile(downloadedFilepath).copy(destination))
	{
		qWarning() << "Add-On Language: Unable to install" << idInstall;
		return 0;
	}

	qDebug() << "Add-On Language: New language installed:" << idInstall;
	return 2;
}

bool AOLanguagePack::uninstallAddOn(const QString &idInstall,
				    const QStringList& selectedFiles) const
{
	Q_UNUSED(selectedFiles); // not applicable - always install all files

	QFile file(m_sLocaleInstallDir % "/" % idInstall % ".qm");
	bool removed = true;
	if (file.exists())
	{
		removed = file.remove();
	}

	if (removed)
	{
		qDebug() << "Add-On Language : Successfully removed" << idInstall;
	}
	else
	{
		qWarning() << "Add-On Language : Error! " << idInstall
			   << "could not be removed. ";

	}

	return removed;
}
