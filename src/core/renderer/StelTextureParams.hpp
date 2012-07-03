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
//! These are e.g. passed to StelTextureMgr to create a texture.
//!
//! This is a builder-style struct. Parameters can be specified like this:
//!
//! @code
//! // Default parameters (no mipmaps, linear filtering, clamp to edge wrap mode).
//! StelTextureParams a;
//! // Generate mipmaps and use repeat wrap mode.
//! StelTextureParams b = StelTextureParams().generateMipmaps().wrap(Repeat);
//! // Generate mipmaps, use nearest-neighbor filtering and use repeat wrap mode.
//! StelTextureParams c = StelTextureParams().generateMipmaps().filtering(Nearest).wrap(Repeat);
//! @endcode
//!
//! @see StelTextureMgr, StelTexture, StelRenderer
struct StelTextureParams 
{
	//! Construct StelTextureParams with default parameters.
	//!
	//! Default parameters are no mipmap generation, linear filtering and clamp to edge wrap mode.
	StelTextureParams()
		:autoGenerateMipmaps(false)
		,filteringMode(TextureFiltering_Linear)
		,wrapMode(TextureWrap_ClampToEdge)
	{
	}

	//! If specified, mipmaps will be generated.
	StelTextureParams& generateMipmaps()
	{
		autoGenerateMipmaps = true;
		return *this;
	}

	//! Set texture filtering mode.
	StelTextureParams& filtering(const TextureFiltering filtering)
	{
		this->filteringMode = filtering;
		return *this;
	}

	//! Set texture wrapping mode.
	StelTextureParams& wrap(const TextureWrap wrap)
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
