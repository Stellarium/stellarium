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

#ifndef _AOLANGUAGEPACK_HPP_
#define _AOLANGUAGEPACK_HPP_

#include "StelAddOn.hpp"

class AOLanguagePack : public StelAddOn
{
	Q_OBJECT
public:
	AOLanguagePack(StelAddOnDAO* pStelAddOnDAO);
	virtual ~AOLanguagePack();

	// check language packs which are already installed.
	virtual QStringList checkInstalledAddOns() const;

	// install language pack from a .qm file.
	virtual int installFromFile(const QString& idInstall,
				    const QString& downloadedFilepath,
				    const QStringList& selectedFiles) const;

	// uninstall language pack
	virtual bool uninstallAddOn(const QString& idInstall,
				    const QStringList& selectedFiles) const;

private:
	StelAddOnDAO* m_pStelAddOnDAO;
	const QString m_sLocaleInstallDir;
};

#endif // _AOLANGUAGEPACK_HPP_
