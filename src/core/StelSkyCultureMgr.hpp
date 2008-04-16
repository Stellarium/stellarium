/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef STELSKYCULTUREMGR_H
#define STELSKYCULTUREMGR_H

#include <QMap>
#include <QString>
#include <QStringList>

//! @class StelSkyCulture
//! Store basic info about a sky culture for stellarium.
class StelSkyCulture
{
public:
	//! English name
	QString englishName;
	//! Name of the author
	QString author;
};

//! @class StelSkyCultureMgr
//! Manage sky cultures for stellarium.
//! Different human cultures have used different names for stars, and visualised
//! different constellations in the sky (and in different parts of the sky). The
//! StelSkyCultureMgr class handles the different sky cultures.
//! @author Fabien Chereau
class StelSkyCultureMgr{
public:
	StelSkyCultureMgr();
	~StelSkyCultureMgr();
	
	//! Initialize the StelSkyCultureMgr object.
	//! Gets the default sky culture name from the application's settings,
	//! sets that sky culture by calling setSkyCultureDir().
	void init();
	
	//! Set the sky culture from i18n name.
	//! @return true on success; false and doesn't change if skyculture is invalid.
	bool setSkyCulture(const QString& cultureName) {return setSkyCultureDir(skyCultureI18ToDirectory(cultureName));}
	
	//! Get the current sky culture
	StelSkyCulture getSkyCulture() const {return currentSkyCulture;}
	
	//! Get the current sky culture translated name
	QString getSkyCultureNameI18() const;
	
	//! Set the current sky culture from the passed directory.
	//! In in the installatiom data directory and user data directory, 
	//! we have the "skycultures" sub-directory. Inside this there is
	//! one sub-directory per sky culture. This sub-directory name
	//! is that we refer to here.
	//! @param cultureDir The sub-directory name inside the "skycultures" 
	//! directory.
	//! @return true on success; else false.
	bool setSkyCultureDir(const QString& cultureDir);
	
	//! Get the current sky culture directory name.  
	QString getSkyCultureDir() {return currentSkyCultureDir;}
	
	//! Get a hash of translated culture names and directories.
	//! @return A newline delimited list of translated culture names and directories
	//! e.g. "name1\ndir1\name2\ndir2".
	QString getSkyCultureHash() const;	
	
	//! Get a list of sky culture names in English.
	//! @return A new-line delimited list of English sky culture names.
	QString getSkyCultureListEnglish(void);
	
	//! Get a list of sky culture names in the current language.
	//! @return A new-line delimited list of translated sky culture names.
	QStringList getSkyCultureListI18(void);
	
	//! Get the culture name in English associated with a specified directory.
	//! @param directory The directory name.
	//! @return The English name for the culture associated with directory.
	QString directoryToSkyCultureEnglish(const QString& directory);
	
	//! Get the culture name translated to current language associated with 
	//! a specified directory.
	//! @param directory The directory name.
	//! @return The translated name for the culture associated with directory.
	QString directoryToSkyCultureI18(const QString& directory) const;
	
	//! Get the directory associated with a specified translated culture name.
	//! @param cultureName The culture name in the current language.
	//! @return The directory assocuated with cultureName.
	QString skyCultureI18ToDirectory(const QString& cultureName) const;
	
private:
	QMap<QString, StelSkyCulture> dirToNameEnglish;
	
	// The directory containing data for the culture used for constellations, etc.. 
	QString currentSkyCultureDir;
	StelSkyCulture currentSkyCulture;
};

#endif
