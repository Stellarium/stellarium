/*
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

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "StelFontMgr.h"
#include "SFont.hpp"
#include "StelApp.h"

using namespace std;

/*************************************************************************
 Constructor for the StelFontMgr class
*************************************************************************/
StelFontMgr::StelFontMgr(const string& fontMapFile)
{
	loadFontForLanguage(fontMapFile);
}

/*************************************************************************
 Dealloc memory for all loaded fonts
*************************************************************************/
StelFontMgr::~StelFontMgr()
{
	std::map<LoadedFont, SFont*, ltLoadedFont>::iterator iter;
	for (iter=loadedFonts.begin();iter!=loadedFonts.end();++iter)
	{
		if (iter->second) delete iter->second;
		iter->second = NULL;
	}
}

/*************************************************************************
 Equality operator for the FontForLanguage class
*************************************************************************/
bool StelFontMgr::FontForLanguage::operator == (const FontForLanguage & f) const
{
	return (f.fixedFontFileName == fixedFontFileName &&
			std::fabs(f.fixedFontScale-fixedFontScale) < 0.01 &&
	        f.fontFileName == fontFileName &&
	        abs(f.fontScale-fontScale) < 0.01);
}

/*************************************************************************
 Constructor for the LoadedFont class
*************************************************************************/
StelFontMgr::LoadedFont::LoadedFont(string afileName, int asize) : 
		fileName(afileName), size(asize)
{
}

/*************************************************************************
 Get the standard font associated to the given language ISO code.
*************************************************************************/
SFont& StelFontMgr::getStandardFont(const string &langageName, double size)
{
	FontForLanguage ffl = getFontForLanguage(langageName);
	LoadedFont lf(ffl.fontFileName, (int)(ffl.fontScale*size*10));
	if (loadedFonts.find(lf)!=loadedFonts.end())
	{
		return *(loadedFonts[lf]);
	}
	else
	{
		SFont* font = new SFont((double)lf.size/10, StelApp::getInstance().getDataFilePath(lf.fileName));
		loadedFonts[lf]=font;
		return *font;
	}
	// Unreachable code
	assert(false);
	
	SFont* dummy;
	return *dummy;
}

/*************************************************************************
 Get the fixed font associated to the given language ISO code.
*************************************************************************/
SFont& StelFontMgr::getFixedFont(const string &langageName, double size)
{
	FontForLanguage ffl = getFontForLanguage(langageName);
	LoadedFont lf(ffl.fixedFontFileName, (int)(ffl.fixedFontScale*size*10));
	if (loadedFonts.find(lf)!=loadedFonts.end())
	{
		return *(loadedFonts[lf]);
	}
	else
	{
		SFont* font = new SFont((double)lf.size/10, StelApp::getInstance().getDataFilePath(lf.fileName));
		loadedFonts[lf]=font;
		return *font;
	}
	// Unreachable code
	assert(false);
	
	SFont* dummy;
	return *dummy;	
}

/*************************************************************************
 Return the structure describing the fonts and scales to use for a given language
*************************************************************************/
StelFontMgr::FontForLanguage& StelFontMgr::getFontForLanguage(const std::string &langageName)
{
	if (fontMapping.find(langageName)==fontMapping.end())
		return fontMapping["*"];
	else return fontMapping[langageName];
}

/*************************************************************************
 Load the associations between langages and font file/scaling
*************************************************************************/
void StelFontMgr::loadFontForLanguage(const string &fontMapFile)
{	
	// Add first default font
	FontForLanguage defaultFont;
	defaultFont.langageName = "*";
	defaultFont.fixedFontFileName = "DejaVuSansMono.ttf";
	defaultFont.fixedFontScale = 1.;
	defaultFont.fontFileName = "DejaVuSans.ttf";
	defaultFont.fontScale = 1.;
	fontMapping["*"] = defaultFont;
	
	// Now see if another font should be used as default or for locale
	ifstream mapFile(fontMapFile.c_str());

	if (!mapFile.is_open())
	{
		cerr << "WARNING: Unable to open font map file " << fontMapFile << " resorting to default fonts." << endl;
		return;
	}

	char buffer[1000];
	FontForLanguage readFont;

	while (mapFile.getline(buffer,999) && !mapFile.eof())
	{
		if( buffer[0] != '#' && buffer[0] != 0)
		{
			istringstream record(buffer);
			record >> readFont.langageName >> readFont.fontFileName >> readFont.fontScale >> readFont.fixedFontFileName >> readFont.fixedFontScale;

			// Test that font files exist
			ifstream fontFile(StelApp::getInstance().getDataFilePath(readFont.fontFileName).c_str());
			if (!fontFile.is_open())
			{
				//cerr << "WARNING: Unable to open " << StelApp::getInstance().getDataFilePath(readFont.fontFileName) << ", will use default font instead." << endl;
				continue;
			}
			fontFile.close();

			// normal font OK, test fixed font
			fontFile.open(StelApp::getInstance().getDataFilePath(readFont.fixedFontFileName).c_str());
			if (!fontFile.is_open())
			{
				//cerr << "WARNING: Unable to open " << StelApp::getInstance().getDataFilePath(readFont.fixedFontFileName) << ", will use default font instead." << endl;
				continue;
			}
			fontFile.close();
			
			if (readFont.langageName=="default")
				readFont.langageName="*";
			fontMapping[readFont.langageName]=readFont;
		}
	}
	mapFile.close();
}
