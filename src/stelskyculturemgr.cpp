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

#include <iostream>
#include <fstream>
#include <dirent.h>
#include "translator.h"
#include "init_parser.h"
#include <cassert>
#include "stelskyculturemgr.h"

StelSkyCultureMgr::StelSkyCultureMgr(const string& cultureDirPath)
{
	struct dirent *entryp;
	DIR *dp;
	
	if ((dp = opendir(cultureDirPath.c_str())) == NULL)
	{
		cerr << "Unable to find culture directory:" << cultureDirPath << endl;
		assert(0);
	}
	
	while ((entryp = readdir(dp)) != NULL)
	{
		string tmp = entryp->d_name;
		string tmpfic = cultureDirPath+"/"+tmp+"/info.ini";
		FILE* fic = fopen(tmpfic.c_str(), "r");
		if (fic)
		{
			InitParser conf;
			conf.load(tmpfic);
			dirToNameEnglish[tmp] = conf.get_str("info:name");
			// cout << tmp << " : " << dirToNameEnglish[tmp] << endl;
			fclose(fic);
		}
	}
	
	closedir(dp);
}


StelSkyCultureMgr::~StelSkyCultureMgr()
{
}


//! @brief Init itself from a config file.
void StelSkyCultureMgr::init(const InitParser& conf)
{
	string tmp = conf.get_str("localization", "sky_culture", "western");
	setSkyCultureDir(tmp);
}

//! Set the current sky culture from the passed directory
bool StelSkyCultureMgr::setSkyCultureDir(const string& cultureDir)
{
	// make sure culture definition exists before attempting or will die
	if(directoryToSkyCultureEnglish(cultureDir) == "")
	{
		cerr << "Invalid sky culture directory: " << cultureDir << endl;
		return false;
	}
	skyCultureDir = cultureDir;
	return true;
}

//! returns newline delimited list of human readable culture names in english
string StelSkyCultureMgr::getSkyCultureListEnglish(void)
{
	string cultures;
	for ( stringHashIter_t iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
	{
		cultures += iter->second + "\n";
	}
	return cultures;
}

//! returns newline delimited list of human readable culture names translated to current locale
wstring StelSkyCultureMgr::getSkyCultureListI18(void)
{
	wstring cultures;
	for ( stringHashIter_t iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
	{
		if (iter != dirToNameEnglish.begin()) cultures += L"\n";
		cultures += _(iter->second);
	}
	//wcout << cultures << endl;
	return cultures;
}

//! returns newline delimited hash of human readable culture names translated to current locale
//! and the directory names
wstring StelSkyCultureMgr::getSkyCultureHash(void) const
{
	wstring cultures;
	for ( stringHashIter_t iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
	{
		
		// weed out invalid hash entries from invalid culture lookups in hash
		// TODO how to keep hash clean in the first place
		if(iter->second == "") continue;
		
		cultures += _(iter->second);
		cultures += wstring(L"\n") + StelUtils::stringToWstring(iter->first) + L"\n";
	}
	// wcout << cultures << endl;
	return cultures;
}

wstring StelSkyCultureMgr::getSkyCulture() const
{
	return directoryToSkyCultureI18(skyCultureDir);
}


string StelSkyCultureMgr::directoryToSkyCultureEnglish(const string& directory)
{
	return dirToNameEnglish[directory];
}

wstring StelSkyCultureMgr::directoryToSkyCultureI18(const string& directory) const
{
	stringHashIter_t i = dirToNameEnglish.find(directory);
	if (i==dirToNameEnglish.end())
		assert(0);
	else
		return _(i->second);
}

string StelSkyCultureMgr::skyCultureToDirectory(const wstring& cultureName)
{
	for ( stringHashIter_t iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
	{
		if (_(iter->second) == cultureName) return iter->first;
	}
	return "";
}
