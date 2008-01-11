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
//
// __________________________________________________________________________________________________

// 20060728 Modified to not load bitmap glyphs due to improper rendering

#include <cstddef>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <cassert>
#include <algorithm>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "GLee.h"

#include "typeface.h"



// *************************************************************************************************
//  Data would be hidden inside an anonymous namespace, but msvc 6 can't handle it
//namespace 
//{
static FT_Library ftlibrary_;
static bool FTinitialized=false; 

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Information on a cached glyph
struct CacheEntry
{
    size_t        textureIndex_;   //!< Index into the TypeFace's texture array
    Vec2f topLeftUV_;      //!< Texcoords for the top left corner of the glyph
    Vec2f bottomRightUV_;  //!< Texcoords for the bottom right corner of the glyph
    Vec2size_t    renderSize_;     //!< The size of the glyph in pixels on the render target
    Vec2f advance_;        //!< The character advance for the cached glyph
    Vec2f bitmapPosition_; //!< The offset from the pen to the top left corner of the bitmap
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Information on a cache texture
struct TextureInfo
{
    typedef GLuint Handle; //!< typedef for a texture handle for an OpenGL texture
    Handle     handle_;    //!< The OpenGL texture handle
    Vec2size_t size_;      //!< The size of the texture in texels
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
typedef std::vector<TextureInfo>      Textures;       //!< An array of textures
typedef std::map<FT_UInt, CacheEntry> CharacterCache; //!< The cache maps glyphs to cache entries
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Implementation data for the TypeFace class
struct Data
{
    //! Constructor
    Data(size_t aPointSize, size_t aResolution) :
        pointSize_(aPointSize), resolution_(aResolution) {}

    size_t         pointSize_;      //!< The size of the type face in points
    size_t         resolution_;     //!< The resolution of the type face in dpi
    FT_Face        face_;           //!< A handle to the FreeType face object
    CharacterCache characterCache_; //!< The glyph cache
    Vec2i offset_;         //!< The top left corner of the next glyph to be cached
    Textures       textures_;       //!< The texture sizes and handles obtained from OpenGL
    bool           hasKerning_;     //!< Set to true if the type face supports kerning
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const size_t glyphPadding = 1;     //!< Glyphs have padding around them to prevent filtering issues
const float OneOver64 = 0.015625f; //!< A frequently used constant
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Returns the value of the next highest power of 2 above \c aValue
size_t
nextHighestPowerOf2(size_t aValue)
{
    size_t v = aValue - 1;
    for(int i = 16; i > 0; i >>= 1) v |= (v >> i);
    return (v + 1);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Returns the maximum texture size permitted by the OpenGL implementation
size_t
maximumTextureSize()
{
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if(maxSize > 512) maxSize = 512;
    return static_cast<size_t>(maxSize);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! returns the width to use for a cache texture
size_t
cacheTextureWidth(size_t aGlyphWidth, size_t numGlyphsToCache)
{
    size_t width = nextHighestPowerOf2( ( (aGlyphWidth + glyphPadding) * numGlyphsToCache) + glyphPadding );
    if(width > maximumTextureSize()) width = maximumTextureSize();
    return width;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Returns the size to use for a cache texture
Vec2size_t
cacheTextureSize(const Vec2size_t& aGlyphSize, size_t numGlyphsToCache)
{
    size_t textureWidth = cacheTextureWidth(aGlyphSize[0], numGlyphsToCache);

    const size_t numGlyphsInRow = (textureWidth - (glyphPadding * 2)) / aGlyphSize[0];
    const size_t requiredHeight = ((numGlyphsToCache / numGlyphsInRow) + 1) * aGlyphSize[1];
    size_t textureHeight = nextHighestPowerOf2(requiredHeight);
    if(textureHeight > maximumTextureSize()) textureHeight = maximumTextureSize();

    return Vec2size_t(textureWidth, textureHeight);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Pushes the pixel storage client attributes and sets them to a standard packing mode
void
setPixelStorage()
{
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    // Enforce a standard packing mode
    glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
    glPixelStorei( GL_UNPACK_LSB_FIRST, GL_FALSE );
    glPixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Pops the pixel storage client attributes off the stack
void
restorePixelStorage()
{
    glPopClientAttrib();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Creates a texture for caching pre-rendered glyphs
TextureInfo
createCacheTexture(const Vec2size_t& aGlyphSize, size_t numGlyphsToCache)
{
    setPixelStorage();
    
    TextureInfo info;
    glGenTextures(1, &info.handle_);

    glBindTexture(GL_TEXTURE_2D, info.handle_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    info.size_ = cacheTextureSize(aGlyphSize, numGlyphsToCache);

    unsigned char* textureData = new unsigned char[info.size_[0]*info.size_[1]];
    memset(textureData, 0, info.size_[0]*info.size_[1]);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, info.size_[0], info.size_[1], 0, GL_ALPHA,
                 GL_UNSIGNED_BYTE, textureData);

    restorePixelStorage();

    delete[] textureData;

    return info;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Blits the glyph represented by the passed in bitmap to the texture described by
//! the passed in texture info.
void
blitGlyphToTexture(FT_Bitmap* aBitmap, const TextureInfo& aTextureInfo,
                   const Vec2i& anOffset, const Vec2size_t& aGlyphSize)
{
    setPixelStorage();

    const int pitch = aBitmap->pitch;
    unsigned char* data = new unsigned char[aGlyphSize[0]*aGlyphSize[1]];

    for(size_t y = 0; y < aGlyphSize[1]; ++y)
    {
        memcpy(data + (y * aGlyphSize[0]), aBitmap->buffer + (y * pitch), aGlyphSize[0]);
    }

    glBindTexture(GL_TEXTURE_2D, aTextureInfo.handle_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, anOffset[0], anOffset[1],
        aGlyphSize[0], aGlyphSize[1], GL_ALPHA, GL_UNSIGNED_BYTE, data);

	// FC: This is a workaround for buggy video cards which don't support glTexSubImage2D
// 	GLint sizeX;
// 	GLint sizeY;
// 	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sizeX);
// 	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sizeY);
// 	unsigned char* previousTex = new unsigned char[sizeX*sizeY];
// 	glGetTexImage(GL_TEXTURE_2D, 0, GL_ALPHA, GL_UNSIGNED_BYTE, previousTex);
// 	for(int y = anOffset[1]; y < anOffset[1]+(int)aGlyphSize[1]; ++y)
// 	{
// 		memcpy(previousTex + (y * sizeX) + anOffset[0], data + ((y-anOffset[1]) * aGlyphSize[0]), aGlyphSize[0]);
// 	}
// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, sizeX, sizeY, 0, GL_ALPHA, GL_UNSIGNED_BYTE, previousTex);
// 	delete[] previousTex;
	
    delete[] data;
	

    restorePixelStorage();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Functor for destroying OpenGL textures
struct DestroyTexture
{
    //! The function that does the destroying
    void
    operator() (const TextureInfo& info)
    {
        glDeleteTextures(1, &info.handle_);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
TypeFace::TypeFace(const std::string& aFaceName, size_t aPointSize, size_t aResolution) :
    data_(new Data(aPointSize, aResolution))
{
	std::ifstream ifs(aFaceName.c_str(), std::ios_base::in);
	if (!ifs.is_open())
	{
		std::cerr << "Error: cannot find TTF font file " << aFaceName << "." << std::endl;
	}
	ifs.close();
	
	if (FTinitialized==false)
	{
		int res = FT_Init_FreeType(&ftlibrary_);
		assert(res==0);
		FTinitialized = true;
	}

	int res = FT_New_Face(ftlibrary_, aFaceName.c_str(), 0, &data_->face_);
    assert(res == 0);

	res = FT_IS_SCALABLE(data_->face_);
    assert(res!=0);

	res = FT_IS_SFNT(data_->face_);
	assert(res!=0);

    if( !data_->face_->charmap) FT_Set_Charmap(data_->face_, data_->face_->charmaps[0]);

	res = FT_Set_Char_Size(data_->face_, 0L, static_cast<FT_F26Dot6>(aPointSize << 6), aResolution, aResolution);
    assert(res == 0);

    data_->hasKerning_ = (FT_HAS_KERNING(data_->face_) != 0);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
TypeFace::~TypeFace()
{
    flushCache();
    FT_Done_Face(data_->face_);
    delete data_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t
TypeFace::pointSize() const
{
    return data_->pointSize_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::setPointSize(size_t aPointSize)
{
    if(data_->pointSize_ != aPointSize)
    {
        data_->pointSize_ = aPointSize;
        const FT_F26Dot6 sz = static_cast<FT_F26Dot6>(aPointSize << 6);
		int res = FT_Set_Char_Size(data_->face_, sz, sz, data_->resolution_, data_->resolution_);
        assert(res == 0);
        flushCache();
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t
TypeFace::resolution() const
{
    return data_->resolution_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::setResolution(size_t aResolution)
{
    if(data_->resolution_ != aResolution)
    {
        data_->resolution_ = aResolution;
        flushCache();
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float TypeFace::ascent() const
{
    return float(data_->face_->size->metrics.y_ppem) - descent();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float TypeFace::descent() const
{
    return static_cast<float>(-data_->face_->size->metrics.descender) * OneOver64;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float TypeFace::lineHeight() const
{
    return (ascent() + descent());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::flushCache()
{
    data_->characterCache_.clear();
    std::for_each(data_->textures_.begin(), data_->textures_.end(), DestroyTexture());
    data_->textures_.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::addNewTexture(const Vec2size_t& aGlyphSize)
{
    const size_t numUncachedGlyphs = data_->face_->num_glyphs - data_->characterCache_.size();
    data_->textures_.push_back(createCacheTexture(aGlyphSize, numUncachedGlyphs));
    data_->offset_.set(glyphPadding, glyphPadding);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::render(const std::wstring& aString, const Vec2f& aPosition, bool upsideDown)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glTranslatef(aPosition[0], aPosition[1], 0.0f);
	if (upsideDown)
	{
		glScalef(1.f, -1.f, 1.f);
		glTranslatef(0.f, lineHeight(), 0.f);
	}

    renderGlyphs(aString);

    glPopMatrix();

    glPopAttrib();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
TypeFace::width(const std::wstring& aString)
{
    std::wstring::const_iterator it = aString.begin();
    float ret = 0;
    size_t leftChar = 0;
    while(it != aString.end())
    {
        size_t rightChar = FT_Get_Char_Index(data_->face_, *it);
        ret += kerning(leftChar, rightChar)[0];
        ret += cachedGlyph(rightChar).advance_[0];
        leftChar = rightChar;
        ++it;
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t TypeFace::hitCharacterIndex(const std::wstring& aString, float anOffset)
{
    bool found = false;

    size_t characterIndex = 0;
    if(anOffset >= 0.0f)
    {
        std::wstring::const_iterator it = aString.begin();
        float oldOffset = 0;
        size_t leftChar = 0;
        while(it != aString.end())
        {
            float newOffset = oldOffset;
            size_t rightChar = FT_Get_Char_Index(data_->face_, *it);
            newOffset += kerning(leftChar, rightChar)[0];
            newOffset += cachedGlyph(rightChar).advance_[0];

            if( (newOffset >= anOffset) && (oldOffset <= anOffset) )
            {
                found = true;
                break;
            }

            leftChar = rightChar;
            oldOffset = newOffset;
            ++characterIndex;
            ++it;
        }
    }

    if(!found) characterIndex = std::wstring::npos;

    return characterIndex;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::addCacheEntry(FT_GlyphRec_* aGlyph, FT_BitmapGlyphRec_* aBitmapGlyph, size_t aGlyphIndex, const Vec2size_t& aGlyphSize)
{
    const TextureInfo& textureInfo = *(data_->textures_.rbegin());

    const Vec2i bottomRight = data_->offset_ + Vec2i(aGlyphSize[0], aGlyphSize[1]);

    const float widthF  = static_cast<float>(textureInfo.size_[0]);
    const float heightF = static_cast<float>(textureInfo.size_[1]);

    CacheEntry entry;
    entry.topLeftUV_[0]=static_cast<float>(data_->offset_[0]) / widthF;
    entry.topLeftUV_[1]=static_cast<float>(data_->offset_[1]) / heightF;
    entry.bottomRightUV_[0]=static_cast<float>(bottomRight[0]) / widthF;
    entry.bottomRightUV_[1]=static_cast<float>(bottomRight[1]) / heightF;
    entry.textureIndex_ = data_->textures_.size() - 1;
    entry.renderSize_ = aGlyphSize;
    entry.advance_ = Vec2f(float(aGlyph->advance.x >> 16), float(aGlyph->advance.y >> 16));
    entry.bitmapPosition_ = Vec2f(float(aBitmapGlyph->left), float(-aBitmapGlyph->top));

    data_->characterCache_[aGlyphIndex] = entry;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Vec2size_t
TypeFace::maximumGlyphSize() const
{
    FT_BBox& bounds = data_->face_->bbox;
    float maxGlyphWidthF = static_cast<float>(bounds.xMax - bounds.xMin);
    maxGlyphWidthF *= ( (float)data_->face_->size->metrics.x_ppem / (float)data_->face_->units_per_EM);

    float maxGlyphHeightF = static_cast<float>(bounds.yMax - bounds.yMin);
    maxGlyphHeightF *= ( (float)data_->face_->size->metrics.y_ppem / (float)data_->face_->units_per_EM);

    return Vec2size_t(static_cast<size_t>(maxGlyphWidthF), static_cast<size_t>(maxGlyphHeightF));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::ensureTextureIsAvailable()
{
    const Vec2size_t maxGlyphSize = maximumGlyphSize();

    if(data_->textures_.size() == 0)
    {
        addNewTexture(maxGlyphSize);
    }
    else
    {
        const TextureInfo& textureInfo = *(data_->textures_.rbegin());
        if(data_->offset_[0] > int(textureInfo.size_[0]) - int(maxGlyphSize[0]) - int(glyphPadding))
        {
            data_->offset_.set(glyphPadding, data_->offset_[1] + maxGlyphSize[1]);

            if(data_->offset_[1] > int(textureInfo.size_[1]) - int(maxGlyphSize[1]) - int(glyphPadding))
            {
                addNewTexture(maxGlyphSize);
            }
        }
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::cacheGlyph(size_t aGlyphIndex)
{
    ensureTextureIsAvailable();

	// 20060728 Modified to not load bitmap glyps - Stellarium patch 1519491
    const FT_Error loadError = FT_Load_Glyph(data_->face_, aGlyphIndex, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
    assert(loadError == 0);
    if(loadError == 0)
    {
        FT_Glyph glyph;
        const FT_Error getError = FT_Get_Glyph( data_->face_->glyph, &glyph);
        if(getError == 0)
        {
            const FT_Error bitmapError = FT_Glyph_To_Bitmap( &glyph, FT_RENDER_MODE_NORMAL, 0, 1);
            if( (bitmapError == 0) && (glyph->format == FT_GLYPH_FORMAT_BITMAP) )
            {
                FT_BitmapGlyph bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
                FT_Bitmap* bitmap = &bitmapGlyph->bitmap;

                const Vec2size_t glyphSize(bitmap->width, bitmap->rows);

                if(glyphSize[0]*glyphSize[1] > 0)
                    blitGlyphToTexture(bitmap, *(data_->textures_.rbegin()), data_->offset_, glyphSize);

                addCacheEntry(glyph, bitmapGlyph, aGlyphIndex, glyphSize);
                data_->offset_[0]=data_->offset_[0] + glyphSize[0] + glyphPadding;
            }
        }
        FT_Done_Glyph(glyph);
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
CacheEntry&
TypeFace::cachedGlyph(size_t aGlyphIndex)
{
    CharacterCache::iterator cacheIterator = data_->characterCache_.find(aGlyphIndex);
    if(cacheIterator == data_->characterCache_.end())
    {
        cacheGlyph(aGlyphIndex);
        cacheIterator = data_->characterCache_.find(aGlyphIndex);
        assert(cacheIterator != data_->characterCache_.end());
    }
    return cacheIterator->second;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Vec2f
TypeFace::renderGlyph(size_t aGlyphIndex, const Vec2f& aPosition)
{
    CacheEntry& entry = cachedGlyph(aGlyphIndex);

    bindTexture(entry);

    const Vec2f position = aPosition + entry.bitmapPosition_;

    const Vec2f topLeftUV     = entry.topLeftUV_;
    const Vec2f bottomRightUV = entry.bottomRightUV_;
    const float glyphWidth  = static_cast<float>(entry.renderSize_[0]);
    const float glyphHeight = static_cast<float>(entry.renderSize_[1]);

    glBegin(GL_QUADS);
    {
        glTexCoord2f( topLeftUV[0], topLeftUV[1] );
        glVertex2f( position[0], position[1] );

        glTexCoord2f( bottomRightUV[0], topLeftUV[1] );
        glVertex2f( position[0] + glyphWidth, position[1] );

        glTexCoord2f( bottomRightUV[0], bottomRightUV[1] );
        glVertex2f( position[0] + glyphWidth, position[1] + glyphHeight );

        glTexCoord2f( topLeftUV[0], bottomRightUV[1] );
        glVertex2f( position[0], position[1] + glyphHeight );
    }
    glEnd();

    return entry.advance_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Vec2f
TypeFace::kerning(size_t leftGlyphIndex, size_t rightGlyphIndex) const
{
    Vec2f ret;
    if(data_->hasKerning_ && (leftGlyphIndex > 0) && (rightGlyphIndex > 0))
    {
        FT_Vector kerningVector;
        FT_Error ftError = FT_Get_Kerning(data_->face_, leftGlyphIndex, rightGlyphIndex, FT_KERNING_DEFAULT, &kerningVector);
        if(!ftError)
        {
            ret = Vec2f(static_cast<float>(kerningVector.x),
                                static_cast<float>(kerningVector.y)) * OneOver64;
        }
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::bindTexture(const CacheEntry& aCacheEntry) const
{
    assert(aCacheEntry.textureIndex_ < data_->textures_.size());
    const TextureInfo& info = data_->textures_[aCacheEntry.textureIndex_];

    GLint currentTextureID = -1;
    glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &currentTextureID);
    if(static_cast<GLuint>(currentTextureID) != info.handle_)
    {
        glBindTexture(GL_TEXTURE_2D, info.handle_);
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::renderGlyphs(const std::wstring& aString, bool useColorSwitch)
{
    Vec2f pos(0.0f, 0.0f);
    size_t leftChar = 0;
    std::wstring::const_iterator it = aString.begin();
    while(it != aString.end())
    {
		if (useColorSwitch)
		{
			// Manage color highlight here
			if( *it==L'\21' )
			{
				glColor3f(0.5,1,0.5);  // normal
				++it;
				continue;
			}
			if( *it==L'\22' )
			{
				glColor3f(1,1,1);    // hilight
				++it;
				continue;
			}
		}
        size_t rightChar = FT_Get_Char_Index(data_->face_, *it);
        pos += kerning(leftChar, rightChar);
        pos += renderGlyph(rightChar, pos);
        leftChar = rightChar;
        ++it;
    }
}

