// __________________________________________________________________________________________________
//    GLGooey Graphical User Interface for OpenGL
//    Copyright (c) 2004 Niel Waldren
//
// This software is provided 'as-is', without any express or implied warranty. In no event will
// the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//     1. The origin of this software must not be misrepresented; you must not claim that you
//        wrote the original software. If you use this software in a product, an acknowledgment
//        in the product documentation would be appreciated but is not required.
//
//     2. Altered source versions must be plainly marked as such, and must not be misrepresented
//        as being the original software.
//
//     3. This notice may not be removed or altered from any source distribution.
//        ***This version has been modified - see below***
// __________________________________________________________________________________________________
//
// MODIFICATION NOTICE
// This is a modification of "GLGooey Graphical User Interface for OpenGL" (C) 2004 Niel Waldren.A
// For a full list of updates, see SVN log.
// Modification include:
// - (20060728) Modified to not load bitmap glyphs due to improper rendering
// - (various)  Integration with Stellarium codebase
// - (20080221) Adaptation to use QT's QStrings rather than STL strings
// - (20080221) Code formatting to meet Stellarium coding guidelines
// - (20080221) Additional comments for Doxygen
// - (20080221) Improve comments
// __________________________________________________________________________________________________

#ifndef __GOOEY_FONT_TYPE_FACE_H__
#define __GOOEY_FONT_TYPE_FACE_H__

#include "vecmath.h"
#include <QString>
typedef Vector2<size_t> Vec2size_t;

struct Data;
struct CacheEntry;
struct FT_GlyphRec_;
struct FT_BitmapGlyphRec_;

//! Base class for gem type faces
class TypeFace
{
	friend class SFont;

public:
	//! Construction requires the name of the font file and optionally the size and resolution.
	//! @param aFileName the name of the file which contains font data.
	//! @aPointSize the size of the font in points.
	//! @aResolution the font resoltion in DPI.
	explicit TypeFace(const QString& aFileName, size_t aPointSize = 12, size_t aResolution = 100);
	
	virtual ~TypeFace();

public:
	//! Returns the point size of the font.
	size_t pointSize() const;

	//! Sets the point size of the font.
	void setPointSize(size_t aPointSize);

	//! Returns the resolution of the font in DPI.
	size_t resolution() const;

	//! Sets the resolution of the font.
	//! @param aResolution the new resolution in DPI.
	void setResolution(size_t aResolution);

	//! Renders the passed in aString at the passed in position.
	//! @param aString the text to render
	//! @param aPosition a two element vector with the coordinstes for where the test will rendered
	//!        (TODO: what does this refer to - the bottom left of the fitst character?)
	//! @param upsideDown boolean value - if true the font is rendered rotated by 180 degrees
	void render(const QString& aString, const Vec2f& aPosition, bool upsideDown=false);

	//! @brief Returns the ascent of this type face in pixels.
	//! @note This is a positive value representing the distance from the baseline to the
	//! highest point used to place an outline point.
	float ascent() const;

	//! @brief Returns the descent of this type face in pixels.
	//! @note This is a \e positive value representing the distance from the baseline to the lowest
	//! point used to place an outline point.
	float descent() const;

	//! Returns the height of a line rendered in this type face.
	float lineHeight() const;

	//! Returns the total width of the passed in text in pixels, assuming the text is
	//! rendered on one line. 
	//! @param aString the string whose width is to be calculated.
	float width(const QString& aString);

	//! Returns the index of the character at the position determined by the passed in
	//! offset from the beginning of the string.
	//! @param aString the string which is to be tested
	//! @oaram the size of the offset which is used to decide which character index to return.
	int hitCharacterIndex(const QString& aString, float anOffset);

	//! Returns the largest theoretically possible glyph size for this face.
	Vec2size_t maximumGlyphSize() const;


private:
	//! the FreeType library object
	//static FT_Library* ftlibrary_;

	//! Renders the glyph with the passed in index.
	//! @param aGlyphIndex the index of the required glyph
	//! @param aPosition ???
	//! @return How far to advance to the position of the next character
	Vec2f renderGlyph(size_t aGlyphIndex, const Vec2f& aPosition);

	//! Adds a new texture to the array of cache textures.
	//! @param aGlyphSize
	void addNewTexture(const Vec2size_t& aGlyphSize);

	//! Puts a new entry into the glyph cache.
	//! @param aGlyph TODO - document parameters
	//! @param aBitmapGlyph
	//! @param aGlyphIndex
	//! @param aGlyphSize
	void addCacheEntry(FT_GlyphRec_* aGlyph, FT_BitmapGlyphRec_* aBitmapGlyph, size_t aGlyphIndex, const Vec2size_t& aGlyphSize);

	//! Checks that a cache texture is available, adding a new one if necessary.
	void ensureTextureIsAvailable();

	//! Returns the cache entry for the glyph with the passed in index.
	CacheEntry& cachedGlyph(size_t aGlyphIndex);

	//! Returns the kerning vector for the passed in pair of glyphs.
	Vec2f kerning(size_t leftGlyphIndex, size_t rightGlyphIndex) const;

	//! Renders the glyphs of the passed in string.
	void renderGlyphs(const QString& aString, bool useColorSwitch=true);

	//! binds the passed in cache entry's texture.
	void bindTexture(const CacheEntry& aCacheEntry) const;

	//! Caches the glyph with the passed in index.
	void cacheGlyph(size_t aGlyphIndex);

	//! Flushes the string and character cache.
	void flushCache();

	//! All member data pertaining to the font is encapsulated an instance of \c Data
	Data* data_; 

};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



#endif
