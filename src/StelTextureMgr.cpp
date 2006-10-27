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

#include <cassert>
#include <iostream>
#include <string>
#include <png.h>
#include "StelTextureMgr.h"

using namespace std;

// Initialize statics
STexture2 StelTextureMgr::NULL_STEXTURE;

/*************************************************************************
 Constructor for the StelTextureMgr class
*************************************************************************/
StelTextureMgr::StelTextureMgr(const std::string& atextureDir) : textureDir(atextureDir)
{}

/*************************************************************************
 Destructor for the StelTextureMgr class
*************************************************************************/
StelTextureMgr::~StelTextureMgr()
{}


/*************************************************************************
 Load an image from a file and create a new texture from it.
*************************************************************************/
STexture2& StelTextureMgr::createTexture(const string& afilename)
{
	string filename = textureDir + afilename;
	// Currently only PNG is supported
	FILE * tempFile = fopen(filename.c_str(),"r");
	if (!tempFile)
	{
		cerr << "WARNING : Can't find texture file " << filename << "!" << endl;
		return NULL_STEXTURE;
	}
	fclose(tempFile);

	STexture2* tex = new STexture2();
	readPNGFromFile(filename, *tex);

	if (!tex->texels)
		return NULL_STEXTURE;
	
	tex->minFilter = (mipmapsMode==true) ? GL_LINEAR_MIPMAP_NEAREST : minFilter;
	tex->magFilter = magFilter;
	tex->wrapMode = wrapMode;
	tex->fullPath = filename;
	tex->mipmapsMode = mipmapsMode;
	
	// generate texture
	glGenTextures (1, &tex->id);
	glBindTexture (GL_TEXTURE_2D, tex->id);

	// setup some parameters for texture filters and mipmapping
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->minFilter);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex->magFilter);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->wrapMode);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->wrapMode);
	glTexImage2D (GL_TEXTURE_2D, 0, tex->internalFormat, tex->width, tex->height, 0, tex->format, GL_UNSIGNED_BYTE, tex->texels);
	if (mipmapsMode==true)
		gluBuild2DMipmaps (GL_TEXTURE_2D, tex->internalFormat, tex->width, tex->height, tex->format, GL_UNSIGNED_BYTE, tex->texels);

	// OpenGL has its own copy of texture data
	free (tex->texels);
	tex->texels = NULL;
	
	return *tex;
}


/*************************************************************************
 Load a PNG image from a file.
*************************************************************************/
bool StelTextureMgr::readPNGFromFile(const string& filename, STexture2& texinfo)
{
	png_byte magic[8];
	png_structp png_ptr;
	png_infop info_ptr;
	int bit_depth, color_type;
	FILE *fp = NULL;
	png_bytep *row_pointers = NULL;
	int i;

	/* open image file */
	fp = fopen (filename.c_str(), "rb");
	if (!fp)
	{
		cerr << "error: couldn't open \""<< filename << "\"!\n";
		return false;
	}

	/* read magic number */
	fread (magic, 1, sizeof (magic), fp);

	/* check for valid magic number */
	if (!png_check_sig (magic, sizeof (magic)))
	{
		cerr << "error: \"" << filename << "\" is not a valid PNG image!\n";
		fclose (fp);
		return false;
	}

	/* create a png read struct */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		fclose (fp);
		return false;
	}

	/* create a png info struct */
	info_ptr = png_create_info_struct (png_ptr);
	if (!info_ptr)
	{
		fclose (fp);
		png_destroy_read_struct (&png_ptr, NULL, NULL);
		return false;
	}

	/* initialize the setjmp for returning properly after a libpng
	   error occured */
	if (setjmp (png_jmpbuf (png_ptr)))
	{
		fclose (fp);
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

		if (row_pointers)
			free (row_pointers);

		if (texinfo.texels)
			free (texinfo.texels);
		texinfo.texels = NULL;

		return false;
	}

	/* setup libpng for using standard C fread() function
	   with our FILE pointer */
	png_init_io (png_ptr, fp);

	/* tell libpng that we have already read the magic number */
	png_set_sig_bytes (png_ptr, sizeof (magic));

	/* read png info */
	png_read_info (png_ptr, info_ptr);

	/* get some usefull information from header */
	bit_depth = png_get_bit_depth (png_ptr, info_ptr);
	color_type = png_get_color_type (png_ptr, info_ptr);

	/* convert index color images to RGB images */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb (png_ptr);

	/* convert 1-2-4 bits grayscale images to 8 bits
	   grayscale. */
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_gray_1_2_4_to_8 (png_ptr);

	if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha (png_ptr);

	if (bit_depth == 16)
		png_set_strip_16 (png_ptr);
	else if (bit_depth < 8)
		png_set_packing (png_ptr);

	/* update info structure to apply transformations */
	png_read_update_info (png_ptr, info_ptr);

	/* retrieve updated information */
	png_get_IHDR (png_ptr, info_ptr,
	              (png_uint_32*)(&texinfo.width),
	              (png_uint_32*)(&texinfo.height),
	              &bit_depth, &color_type,
	              NULL, NULL, NULL);

	/* get image format and components per pixel */
	switch (color_type)
	{
	case PNG_COLOR_TYPE_GRAY:
		texinfo.format = GL_LUMINANCE;
		texinfo.internalFormat = 1;
		break;

	case PNG_COLOR_TYPE_GRAY_ALPHA:
		texinfo.format = GL_LUMINANCE_ALPHA;
		texinfo.internalFormat = 2;
		break;

	case PNG_COLOR_TYPE_RGB:
		texinfo.format = GL_RGB;
		texinfo.internalFormat = 3;
		break;

	case PNG_COLOR_TYPE_RGB_ALPHA:
		texinfo.format = GL_RGBA;
		texinfo.internalFormat = 4;
		break;

	default:
		// Badness
		assert(0);
	}

	/* we can now allocate memory for storing pixel data */
	texinfo.texels = (GLubyte *)malloc (sizeof (GLubyte) * texinfo.width
	                                     * texinfo.height * texinfo.internalFormat);

	/* setup a pointer array.  Each one points at the begening of a row. */
	row_pointers = (png_bytep *)malloc (sizeof (png_bytep) * texinfo.height);

	for (i = 0; i < texinfo.height; ++i)
	{
		row_pointers[i] = (png_bytep)(texinfo.texels +
		                              ((texinfo.height - (i + 1)) * texinfo.width * texinfo.internalFormat));
	}

	/* read pixel data using row pointers */
	png_read_image (png_ptr, row_pointers);

	/* finish decompression and release memory */
	png_read_end (png_ptr, NULL);
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

	/* we don't need row pointers anymore */
	free (row_pointers);

	fclose (fp);
	return true;
}
