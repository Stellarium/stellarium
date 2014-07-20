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

#include "AOLanguagePack.hpp"

AOLanguagePack::AOLanguagePack(StelAddOnDAO* pStelAddOnDAO)
	: m_pStelAddOnDAO(pStelAddOnDAO)
	, m_sStellariumLocaleInstallDir(StelFileMgr::getLocaleUserDir() % "/stellarium")
	, m_sStarloreLocaleInstallDir(StelFileMgr::getLocaleUserDir() % "/stellarium-skycultures")
{
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sStellariumLocaleInstallDir);
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sStarloreLocaleInstallDir);
}

AOLanguagePack::~AOLanguagePack()
{
}

QStringList AOLanguagePack::checkInstalledAddOns() const
{
	return QStringList();
}

bool AOLanguagePack::installFromFile(const QString& idInstall,
				     const QString& downloadFilepath) const
{
	// TODO
	return false;
}

bool AOLanguagePack::uninstallAddOn(const QString &idInstall) const
{
	// TODO
	return false;
}
