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

#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "Translator.hpp"
#include "InitParser.hpp"
#include "StelApp.hpp"

#include <iostream>
#include <fstream>
#include <dirent.h>
#include <cassert>

StelSkyCultureMgr::StelSkyCultureMgr()
{
	set<string> cultureDirNames;
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	
	try
	{
		cultureDirNames = fileMan.listContents("skycultures",StelFileMgr::DIRECTORY);
	}
	catch(exception& e)
	{
		cerr << "ERROR while trying list sky cultures:" << e.what() << endl;	
	}
	
	for(set<string>::iterator dir=cultureDirNames.begin(); dir!=cultureDirNames.end(); dir++)
	{
		try
		{
			InitParser pd;
			pd.load(fileMan.findFile("skycultures/" + *dir + "/info.ini"));
			dirToNameEnglish[*dir] = pd.get_str("info:name");
		}
		catch (exception& e)
		{
			cerr << "WARNING: unable to successfully read info.ini file from skyculture dir" << *dir << endl;
		}
	}	
}


StelSkyCultureMgr::~StelSkyCultureMgr()
{
}


//! Init itself from a config file.
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
	StelApp::getInstance().updateSkyCulture();
	return true;
}

//! returns newline delimited list of human readable culture names in english
string StelSkyCultureMgr::getSkyCultureListEnglish(void)
{
	string cultures;
	for ( map<string, string>::const_iterator iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
	{
		cultures += iter->second + "\n";
	}
	return cultures;
}

//! returns newline delimited list of human readable culture names translated to current locale
wstring StelSkyCultureMgr::getSkyCultureListI18(void)
{
	wstring cultures;
	for ( map<string, string>::const_iterator iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
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
	for ( map<string, string>::const_iterator iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
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
	map<string, string>::const_iterator i = dirToNameEnglish.find(directory);
	if (i==dirToNameEnglish.end())
	{
		cout << "WARNING: StelSkyCultureMgr::directoryToSkyCultureI18(\""
		     << directory << "\"): could not find directory" << endl;
//johannes: this is not fatal.
//		assert(0);
		return L"";
	}
	else
		return _(i->second);
}

string StelSkyCultureMgr::skyCultureToDirectory(const wstring& cultureName)
{
	for ( map<string, string>::const_iterator iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
	{
		if (_(iter->second) == cultureName) return iter->first;
	}
	return "";
}
