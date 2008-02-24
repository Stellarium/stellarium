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
#include <QDebug>
#include <QFile>
#include <QRegExp>

#include "StelFontMgr.hpp"
#include "SFont.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"

using namespace std;

/*************************************************************************
 Constructor for the StelFontMgr class
*************************************************************************/
StelFontMgr::StelFontMgr()
{
	QString fontMapFile("");
	try 
	{
		fontMapFile = StelApp::getInstance().getFileMgr().findFile("data/fontmap.dat");
	}
	catch(exception& e)
	{
		qWarning() << "ERROR when locating font map file: " << e.what();
	}
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
StelFontMgr::LoadedFont::LoadedFont(const QString& afileName, int asize) 
	: fileName(afileName), size(asize)
{
}

/*************************************************************************
 Get the standard font associated to the given language ISO code.
*************************************************************************/
SFont& StelFontMgr::getStandardFont(const QString& langageName, double size)
{
	FontForLanguage ffl = getFontForLanguage(langageName);
	LoadedFont lf(ffl.fontFileName, (int)(ffl.fontScale*size*10));
	if (loadedFonts.find(lf)!=loadedFonts.end())
	{
		return *(loadedFonts[lf]);
	}
	else
	{
		try
		{
			SFont* font = new SFont((double)lf.size/10, StelApp::getInstance().getFileMgr().findFile("data/"+lf.fileName));
			loadedFonts[lf]=font;
			return *font;
		}
		catch (exception& e)
		{
			qWarning() << "ERROR while trying to load fonts: " << e.what();
			SFont* dummy=NULL;
			return *dummy;
		}
	}
}

/*************************************************************************
 Get the fixed font associated to the given language ISO code.
*************************************************************************/
SFont& StelFontMgr::getFixedFont(const QString &langageName, double size)
{
	FontForLanguage ffl = getFontForLanguage(langageName);
	LoadedFont lf(ffl.fixedFontFileName, (int)(ffl.fixedFontScale*size*10));
	if (loadedFonts.find(lf)!=loadedFonts.end())
	{
		return *(loadedFonts[lf]);
	}
	else
	{
		try {
			SFont* font = new SFont((double)lf.size/10, StelApp::getInstance().getFileMgr().findFile("data/" + lf.fileName));
			loadedFonts[lf]=font;
			return *font;
		}
		catch (exception& e)
		{
			qWarning() << "ERROR loading font " << lf.fileName << ": " << e.what();
		}
	}
	// Unreachable code
	assert(false);
	
	SFont* dummy=NULL;
	return *dummy;
}

/*************************************************************************
 Return the structure describing the fonts and scales to use for a given language
*************************************************************************/
StelFontMgr::FontForLanguage& StelFontMgr::getFontForLanguage(const QString& langageName)
{
	if (fontMapping.find(langageName)==fontMapping.end())
		return fontMapping["*"];
	else return fontMapping[langageName];
}

/*************************************************************************
 Load the associations between langages and font file/scaling
*************************************************************************/
void StelFontMgr::loadFontForLanguage(const QString& fontMapFile)
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
	QFile file(fontMapFile);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "WARNING: Unable to open font map file " << fontMapFile << " resorting to default fonts.";
		return;
	}

	FontForLanguage readFont;
	QRegExp commentRx("^\\s*#.*$");
	while (!file.atEnd()) {
		QByteArray buffer = file.readLine();
		QString line(buffer);
		if (!commentRx.exactMatch(line))
		{
			QTextStream record(&line);
			record >> readFont.langageName >> readFont.fontFileName >> readFont.fontScale >> readFont.fixedFontFileName >> readFont.fixedFontScale;
			try
			{
				StelApp::getInstance().getFileMgr().findFile("data/" + readFont.fontFileName);
				StelApp::getInstance().getFileMgr().findFile("data/" + readFont.fixedFontFileName);
			}
			catch(exception& e)
			{
				continue;
			}

			if (readFont.langageName=="default")
				readFont.langageName="*";
			fontMapping[readFont.langageName]=readFont;
		}
	}

	file.close();
}
