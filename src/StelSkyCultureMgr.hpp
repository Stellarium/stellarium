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

#include <map>
#include <string>
#include <QString>

using namespace std;

class InitParser;

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
	//! Gets the default sky culture name from the ini parser object and 
	//! sets that sky culture by calling setSkyCultureDir().
	//! @param conf The ini parser object which contains the default sky 
	//! culture setting.
	void init(const InitParser& conf);
	
	//! Set the sky culture from i18n name.
	//! @return true on success; false and doesn't change if skyculture is invalid.
	bool setSkyCulture(const wstring& cultureName) {return setSkyCultureDir(skyCultureToDirectory(cultureName));}
	
	//! Get the current sky culture i18n name.
	wstring getSkyCulture() const;
	
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
	QString getSkyCultureDir() {return skyCultureDir;}
	
	//! Get a hash of translated culture names and directories.
	//! @return A newline delimited list of translated culture names and directories
	//! e.g. "name1\ndir1\name2\ndir2".
	wstring getSkyCultureHash() const;	
	
	//! Get a list of sky culture names in English.
	//! @return A new-line delimited list of English sky culture names.
	string getSkyCultureListEnglish(void);
	
	//! Get a list of sky culture names in the current language.
	//! @return A new-line delimited list of translated sky culture names.
	wstring getSkyCultureListI18(void);
	
	//! Get the culture name in English associated with a specified directory.
	//! @param directory The directory name.
	//! @return The English name for the culture associated with directory.
	string directoryToSkyCultureEnglish(const QString& directory);
	
	//! Get the culture name translated to current language associated with 
	//! a specified directory.
	//! @param directory The directory name.
	//! @return The translated name for the culture associated with directory.
	wstring directoryToSkyCultureI18(const QString& directory) const;
	
	//! Get the directory associated with a specified translated culture name.
	//! @param cultureName The culture name in the current language.
	//! @return The directory assocuated with cultureName.
	QString skyCultureToDirectory(const wstring& cultureName);
	
private:
	map<QString, string> dirToNameEnglish;
	
	// The directory containing data for the culture used for constellations, etc.. 
	QString skyCultureDir;
};

#endif
