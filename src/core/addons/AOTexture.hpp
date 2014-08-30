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

#ifndef _AOTEXTURE_HPP_
#define _AOTEXTURE_HPP_

#include "StelAddOn.hpp"

class AOTexture : public StelAddOn
{
	Q_OBJECT
public:
	AOTexture();
	virtual ~AOTexture();

	// check textures which are already installed.
	// @return "installId/texture"
	virtual QStringList checkInstalledAddOns() const;

	// install texture from a zip file.
	virtual int installFromFile(const QString& idInstall,
				    const QString& downloadedFilepath,
				    const QStringList& selectedFiles) const;

	// uninstall texture
	virtual int uninstallAddOn(const QString& idInstall,
				    const QStringList& selectedFiles) const;

private:
	const QString m_sTexturesInstallDir;

	// In order to enable us to know which are the installed textures,
	// it will store the texture name and the id_install (enough to identify the source)
	QSettings* m_pInstalledTextures; // .ini file

	int installFromZip(QString idInstall, QString downloadedFilepath, QStringList filesToInstall) const;
	int installFromImg(QString idInstall, QString downloadedFilepath) const;
};

#endif // _AOTEXTURE_HPP_
