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

#include "stelfontmgr.h"
#include "stelapp.h"

using namespace std;

StelFontMgr::StelFontMgr()
{}


StelFontMgr::~StelFontMgr()
{}

bool StelFontMgr::FontForLanguage::operator == (const FontForLanguage & f) const
{
	return (f.fixedFontFileName == fixedFontFileName &&
			std::fabs(f.fixedFontScale-fixedFontScale) < 0.01 &&
	        f.fontFileName == fontFileName &&
	        abs(f.fontScale-fontScale) < 0.01);
}


//! Return the structure describing the fonts and scales to use for a given language
StelFontMgr::FontForLanguage& StelFontMgr::getFontForLocale(const std::string &langageName)
{
	if (fontMapping.find(langageName)==fontMapping.end())
		return fontMapping["*"];
	else return fontMapping[langageName];
}


//! Load the associations between langages and font file/scaling
void StelFontMgr::loadFontForLanguage(const string &fontMapFile)
{
	// Add first default font
	FontForLanguage defaultFont;
	defaultFont.langageName = "*";
	defaultFont.fixedFontFileName = "DejaVuSansMono.ttf";
	defaultFont.fixedFontScale = 1.;
	defaultFont.fontFileName = "DejaVuSans.ttf";
	defaultFont.fontScale = 1.;

	// Now see if another font should be used as default or for locale
	ifstream mapFile(fontMapFile.c_str());

	if (!mapFile.is_open())
	{
		cerr << "WARNING: Unable to open " << fontMapFile << " resorting to default fonts." << endl;
		return;
	}

	char buffer[1000];
	FontForLanguage readFont;

	while (mapFile.getline (buffer,999) && !mapFile.eof())
	{
		if( buffer[0] != '#' && buffer[0] != 0)
		{
			istringstream record(buffer);
			record >> readFont.langageName >> readFont.fontFileName >> readFont.fontScale >> readFont.fixedFontFileName >> readFont.fixedFontScale;

			// Test that font files exist
			ifstream fontFile(StelApp::getInstance().getDataFilePath(readFont.fontFileName).c_str());
			if (!fontFile.is_open())
			{
				cerr << "WARNING: Unable to open " << StelApp::getInstance().getDataFilePath(readFont.fontFileName) << " resorting to default font." << endl;
			}
			else
			{
				readFont.fontFileName = defaultFont.fontFileName;
				readFont.fontScale = defaultFont.fontScale;
			}
			fontFile.close();

			// normal font OK, test fixed font
			fontFile.open(StelApp::getInstance().getDataFilePath(readFont.fixedFontFileName).c_str());
			if (!fontFile.is_open())
			{
				cerr << "WARNING: Unable to open " << StelApp::getInstance().getDataFilePath(readFont.fontFileName) << " resorting to default font." << endl;
			}
			else
			{
				readFont.fixedFontFileName = defaultFont.fixedFontFileName;
				readFont.fixedFontScale = defaultFont.fixedFontScale;
			}
			fontFile.close();
		}
	}
}
