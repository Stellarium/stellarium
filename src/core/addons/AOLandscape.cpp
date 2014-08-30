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

#include "AOLandscape.hpp"

AOLandscape::AOLandscape()
	: m_pLandscapeMgr(GETSTELMODULE(LandscapeMgr))
	, m_sLandscapeInstallDir(StelFileMgr::getUserDir() % "/landscapes/")
{
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sLandscapeInstallDir);
}

AOLandscape::~AOLandscape()
{
}

QStringList AOLandscape::checkInstalledAddOns() const
{
	return m_pLandscapeMgr->getUserLandscapeIDs();
}

int AOLandscape::installFromFile(const QString& idInstall,
				 const QString& downloadedFilepath,
				 const QStringList& selectedFiles) const
{
	Q_UNUSED(idInstall);
	Q_UNUSED(selectedFiles); // not applicable - always install all files

	// TODO: the method LandscapeMgr::installLandscapeFromArchive must be removed
	//       and this operation have to be done here.
	//       the LANDSCAPEID must be the same as idInstall (from db)
	QString landscapeId = m_pLandscapeMgr->installLandscapeFromArchive(downloadedFilepath);
	if (landscapeId.isEmpty())
	{
		return 0;
	}
	return 2;
}

int AOLandscape::uninstallAddOn(const QString& idInstall,
				const QStringList& selectedFiles) const
{
	Q_UNUSED(selectedFiles); // not applicable - always install all files

	if(!m_pLandscapeMgr->removeLandscape(idInstall))
	{
		return false;
	}
	return true;
}
