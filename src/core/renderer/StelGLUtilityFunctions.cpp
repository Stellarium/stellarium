/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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

#include <QDebug>

#include "StelFileMgr.hpp"
#include "StelGLUtilityFunctions.hpp"

//! Utility function that returns a filename with changed file extension.
static QString withFileExtension(const QString& filename, const QString& newExt)
{
	const int lastSep = filename.lastIndexOf('/');
	const int extStart = filename.lastIndexOf('.') + 1;
	// If there's no dot in the filename, just append the extension.
	if(extStart < lastSep) {return filename + "." + newExt;}
	QString result = filename;
	result.replace(extStart, filename.length() - extStart, newExt);
	return result;
}

//! Utility wrapper function to get full path of a texture file.
//!
//! @param filename File name to get full path for.
//! @param outFullPath Will store full path on success.
//! @param outErrorMessage Will store full path on failure.
//!
//! @return true on success, false on failure.
static bool findTextureFile(const QString& filename, QString& outFullPath,
                            QString& outErrorMessage)
{
	try
	{
		outFullPath = StelFileMgr::findFile(filename);
		return true;
	}
	catch (std::runtime_error er)
	{
		outErrorMessage = er.what();
		return false;
	}
}

QString glFileSystemTexturePath(const QString& filename, const bool pvrSupported)
{
	QString result, errorMessage;

	// Compressed PVR versions are preferred if they exist.
	// If the texture is not found, we look for it in "textures/"
	
	if(pvrSupported)
	{
		const QString pvr1 = withFileExtension(filename, "pvr");
		const QString pvr2 = withFileExtension("textures/" + filename, "pvr");
		if(findTextureFile(pvr1, result, errorMessage)) {return result;}
		if(findTextureFile(pvr2, result, errorMessage)) {return result;}
	}

	if(findTextureFile(filename, result, errorMessage)) {return result;}
	if(findTextureFile("textures/" + filename, result, errorMessage)) {return result;}

	qWarning() << "WARNING : Couldn't find texture file " 
	           << filename << ": " << result;
	return QString();
}
