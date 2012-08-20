/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELTEXTUREPARAMS_HPP_
#define _STELTEXTUREPARAMS_HPP_

//! Texture filtering modes.
//!
//! These specify how the texture is enlarged/shrunk as the viewer 
//! gets closer/farther.
enum TextureFiltering
{
	//! Nearest-neighbor (blocky, like in old 1990's 3D games).
	TextureFiltering_Nearest,
	//! Bilinear if there are no mipmaps or trilinear with mipmaps.
	TextureFiltering_Linear
};

//! Texture wrapping modes.
//!
//! These specify how the texture is applied outside its area 
//! (i.e. outside the (0,0, 1,1) texture coord rectangle.)
enum TextureWrap
{
	//! Texture is repeated infinitely.
	TextureWrap_Repeat,
	//! Colors at the edge of the texture are used.
	TextureWrap_ClampToEdge
};

//! Parameters specifying how to construct a texture.
//!
//! These are passed to StelRenderer to create a texture.
//!
//! This is a builder-style struct. Parameters can be specified like this:
//!
//! @code
//! // Default parameters (no mipmaps, linear filtering, clamp to edge wrap mode).
//! TextureParams a;
//! // Generate mipmaps and use repeat wrap mode.
//! TextureParams b = TextureParams().generateMipmaps().wrap(Repeat);
//! // Generate mipmaps, use nearest-neighbor filtering and use repeat wrap mode.
//! TextureParams c = TextureParams().generateMipmaps()
//!                   .filtering(TextureFiltering_Nearest).wrap(TextureWrap_Repeat);
//! @endcode
//!
//! @see StelTextureNew, StelRenderer
struct TextureParams 
{
	//! Construct TextureParams with default parameters.
	//!
	//! Default parameters are no mipmap generation, linear filtering and clamp to edge wrap mode.
	TextureParams()
		:autoGenerateMipmaps(false)
		,filteringMode(TextureFiltering_Linear)
		,wrapMode(TextureWrap_ClampToEdge)
	{
	}

	//! If specified, mipmaps will be generated.
	TextureParams& generateMipmaps()
	{
		autoGenerateMipmaps = true;
		return *this;
	}

	//! Set texture filtering mode.
	TextureParams& filtering(const TextureFiltering filtering)
	{
		this->filteringMode = filtering;
		return *this;
	}

	//! Set texture wrapping mode.
	TextureParams& wrap(const TextureWrap wrap)
	{
		this->wrapMode = wrap;
		return *this;
	}

	//! Automatically generate mipmaps? 
	//!
	//! Note that a StelRenderer backend might ignore this.
	//! (E.g. mipmaps might not be supported)
	bool autoGenerateMipmaps;

	//! Texture filtering mode.
	//!
	//! Note that a StelRenderer backend might ignore this.
	//! (E.g. linear filtering with software renderer might be too slow)
	TextureFiltering filteringMode;

	//! Texture wrapping mode.
	TextureWrap wrapMode;
};

#endif // _STELTEXTUREPARAMS_HPP_
