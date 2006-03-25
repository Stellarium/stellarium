/*
 * Stellarium
 * This file Copyright (C) 2004 Robert Spearman
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
#include "sky_localizer.h"
#include "translator.h"
#include "init_parser.h"

SkyLocalizer::SkyLocalizer(const string& cultureDir)
{
	struct dirent *entryp;
	DIR *dp;

	if ((dp = opendir(cultureDir.c_str())) == NULL)
	{
		cerr << "Unable to find culture directory:" << cultureDir << endl;
		assert(0);
	}

	while ((entryp = readdir(dp)) != NULL)
	{
		string tmp = entryp->d_name;
		string tmpfic = cultureDir+"/"+tmp+"/info.ini";
		FILE* fic = fopen(tmpfic.c_str(), "r");
		if (fic)
		{
			InitParser conf;
			conf.load(tmpfic);
			dirToNameEnglish[tmp] = conf.get_str("info:name");
			//cout << tmp << " : " << dirToNameEnglish[tmp] << endl;
			fclose(fic);
		}
	}
	
	closedir(dp);
}

SkyLocalizer::~SkyLocalizer(void)
{}

//! returns newline delimited list of human readable culture names in english
string SkyLocalizer::getSkyCultureListEnglish(void)
{
	string cultures;
	for ( stringHashIter_t iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
	{
		cultures += iter->second + "\n";
	}
	return cultures;
}

//! returns newline delimited list of human readable culture names translated to current locale
wstring SkyLocalizer::getSkyCultureListI18(void)
{
	wstring cultures;
	for ( stringHashIter_t iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
	{
		cultures += _(iter->second);
		cultures += L"\n";
	}
	return cultures;
}

string SkyLocalizer::directoryToSkyCultureEnglish(const string& directory)
{
	return dirToNameEnglish[directory];
}

wstring SkyLocalizer::directoryToSkyCultureI18(const string& directory)
{
	return _(dirToNameEnglish[directory]);
}

string SkyLocalizer::skyCultureToDirectory(const wstring& cultureName)
{
	for ( stringHashIter_t iter = dirToNameEnglish.begin(); iter != dirToNameEnglish.end(); ++iter )
	{
		if (_(iter->second) == cultureName) return iter->first;
	}
	return "";
}
