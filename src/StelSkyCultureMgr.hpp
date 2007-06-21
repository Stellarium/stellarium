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

using namespace std;

class InitParser;

/**
	Manage sky cultures for stellarium.
	@author Fabien Chereau
*/
class StelSkyCultureMgr{
public:
    StelSkyCultureMgr();

    ~StelSkyCultureMgr();
	
	//! @brief Init itself from a config file.
	void init(const InitParser& conf);
	
	//! Set the sky culture from I18 name
	//! Returns false and doesn't change if skyculture is invalid
	bool setSkyCulture(const wstring& cultureName) {return setSkyCultureDir(skyCultureToDirectory(cultureName));}
	
	//! Get the current sky culture I18 name
	wstring getSkyCulture() const;
	
	//! Set the current sky culture from the passed directory
	bool setSkyCultureDir(const string& culturedir);
	
	//! Get the current sky culture directory
	string getSkyCultureDir() {return skyCultureDir;}
	
	//! returns newline delimited hash of translated culture names and directories
	wstring getSkyCultureHash() const;	
	
	//! returns newline delimited list of human readable culture names in english
	string getSkyCultureListEnglish(void);
	
	//! returns newline delimited list of human readable culture names translated to current language
	wstring getSkyCultureListI18(void);
	
	//! Get the culture name in english associated to the passed directory
	string directoryToSkyCultureEnglish(const string& directory);
	
	//! Get the culture name translated to current language associated to the passed directory
	wstring directoryToSkyCultureI18(const string& directory) const;
	
	//! Get the directory associated to the passed translated culture name
	string skyCultureToDirectory(const wstring& cultureName);
	
private:
	map<string, string> dirToNameEnglish;
	
	// The directory containing data for the culture used for constellations, etc.. 
	string skyCultureDir;
};

#endif
