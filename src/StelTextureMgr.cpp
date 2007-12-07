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
#include <vector>

#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelApp.hpp"
#include "InitParser.hpp"

extern "C" {
#include <jpeglib.h>
}
#include <png.h>

#include "StelUtils.hpp"

#include "fixx11h.h"
#include <QHttp>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QThread>
				 				 
using namespace std;

// Initialize statics
StelTextureMgr::PngLoader StelTextureMgr::pngLoader;
StelTextureMgr::JpgLoader StelTextureMgr::jpgLoader;

/*************************************************************************
 Constructor for the StelTextureMgr class
*************************************************************************/
StelTextureMgr::StelTextureMgr()
{
	downloader = new QHttp((QObject*)this);

	// Init default values
	setDefaultParams();

	// Add default loaders
	registerImageLoader("png", &pngLoader);
	registerImageLoader("PNG", &pngLoader);
	registerImageLoader("jpeg", &jpgLoader);
	registerImageLoader("JPEG", &jpgLoader);
	registerImageLoader("jpg", &jpgLoader);
	registerImageLoader("JPG", &jpgLoader);
}

/*************************************************************************
 Destructor for the StelTextureMgr class
*************************************************************************/
StelTextureMgr::~StelTextureMgr()
{
}

/*************************************************************************
 Initialize some variable from the openGL context.
 Must be called after the creation of the openGL context.
*************************************************************************/
void StelTextureMgr::init(const InitParser& conf)
{
	// Get whether non-power-of-2 and non square 2D textures are supported on this video card
	isNoPowerOfTwoAllowed = conf.get_boolean("video","non_power_of_two_textures", true);
	isNoPowerOfTwoAllowed = isNoPowerOfTwoAllowed && (GLEE_ARB_texture_non_power_of_two || GLEE_VERSION_2_0);
	
	// Check vendor and renderer
	string glRenderer((char*)glGetString(GL_RENDERER));
	string glVendor((char*)glGetString(GL_VENDOR));
	string glVersion((char*)glGetString(GL_VERSION));

	// Get Maximum Texture Size Supported by the video card
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
}

/*************************************************************************
 Set default parameters for Mipmap mode, wrap mode, min and mag filters
*************************************************************************/
void StelTextureMgr::setDefaultParams()
{
	setMipmapsMode();
	setWrapMode();
	setMinFilter();
	setMagFilter();
	setDynamicRangeMode();
}

/*************************************************************************
 Internal
*************************************************************************/
STextureSP StelTextureMgr::initTex()
{
	STextureSP tex(new STexture());
	// Set parameters than can be set for this texture
	tex->minFilter = ((mipmapsMode==true) ? GL_LINEAR_MIPMAP_NEAREST : minFilter);
	tex->magFilter = magFilter;
	tex->wrapMode = wrapMode;
	tex->mipmapsMode = mipmapsMode;
	tex->dynamicRangeMode = dynamicRangeMode;
	return tex;
}

/*************************************************************************
 Load an image from a file and create a new texture from it.
*************************************************************************/
STextureSP StelTextureMgr::createTexture(const QString& afilename)
{
	STextureSP tex = initTex();

	try
	{
		tex->fullPath = StelApp::getInstance().getFileMgr().findFile(afilename);
	}
	catch (exception e)
	{
		try
		{
			tex->fullPath = StelApp::getInstance().getFileMgr().findFile("textures/" + afilename);
		}
		catch (exception e)
		{
			qWarning() << "WARNING : Can't find texture file " << afilename << ": " << e.what() << endl;
			tex->errorOccured = true;
			return STextureSP();
		}
	}

	tex->downloaded = true;
	
	// Simply load everything
	if (tex->imageLoad() && tex->glLoad())
		return tex;
	else
		return STextureSP();
}



/*************************************************************************
 Load an image from a file and create a new texture from it in a new thread. 
*************************************************************************/
STextureSP StelTextureMgr::createTextureThread(const QString& url, const QString& fileExtension)
{
	STextureSP tex = initTex();
	if (!url.startsWith("http://"))
	{
		// Assume a local file
		try
		{
			tex->fullPath = StelApp::getInstance().getFileMgr().findFile(url);
		}
		catch (exception e)
		{
			try
			{
				tex->fullPath = StelApp::getInstance().getFileMgr().findFile("textures/" + url);
			}
			catch (exception e)
			{
				qWarning() << "WARNING : Can't find texture file " << url << ": " << e.what() << endl;
				tex->errorOccured = true;
				return STextureSP();
			}
		}
	}
	tex->fileExtension = fileExtension;
	return tex;
}

/*************************************************************************
 Adapt the scaling for the texture. Return true if there was no errors
 This method is re-entrant
*************************************************************************/
bool StelTextureMgr::reScale(STexture* tex)
{
	const unsigned int nbPix = tex->width*tex->height;
	const int bitpix = tex->internalFormat*8;
	// Scale input image according to the dynamic range parameters
	if (tex->format==GL_LUMINANCE)
	{
		switch (tex->dynamicRangeMode)
		{
			case (STextureTypes::LINEAR):
			{
				if (tex->internalFormat==1)
				{
					// Assumes already GLubyte = unsigned char on 8 bits
					return true;
				}
				if (tex->internalFormat==2)
				{
					return true;
				}
				if (tex->internalFormat==4)
				{
					return true;
				}
				if (tex->internalFormat==-4)
				{
					return true;
				}
				// Unsupported format..
				qWarning() << "Internal format: " << tex->internalFormat << " is not supported for LUMINANCE texture " << tex->fullPath;
				return false;
			}
			case (STextureTypes::MINMAX_QUANTILE):
			{
				// Compute the image histogram
				int* histo = (int*)calloc(sizeof(int), 1<<bitpix); 
				
				if (tex->internalFormat==1)
				{
					// We assume unsigned char = GLubyte
					GLubyte* data = (GLubyte*)tex->texels;
					for (unsigned int i = 0; i <nbPix ; ++i)
						++(histo[(int)data[i]]);
				}
				else if (tex->internalFormat==2)
				{
					// We assume short unsigned int = GLushort
					GLushort* data = (GLushort*)tex->texels;
					for (unsigned int i = 0; i <nbPix; ++i)
						++(histo[(int)data[i]]);
				}
				else
				{
					// Unsupported format..
					qWarning() << "Internal format: " << tex->internalFormat << " is not supported for LUMINANCE texture " << tex->fullPath;
					return false;
				}
				
				// From the histogram, compute the Quantile cut at values minQuantile and maxQuantile
				const double minQuantile = 0.01;
				const double maxQuantile = 0.99;
				int minCut=0, maxCut=0;
				int thresh = (int)(minQuantile*nbPix);
				int minI = 0;
				// Start from 1 to ignore zeroed region in the image
				for (int idd=1;idd<1<<bitpix;++idd)
				{
					minI+=histo[idd];
					if (minI>=thresh)
					{
						minCut = idd;
						break;
					}
				}
				
				thresh = (int)((1.-maxQuantile)*nbPix);
				int maxI = 0;
				// Finish at 1 to ignore zeroed region in the image
				for (int idd=1<<bitpix;idd>=1;--idd)
				{
					maxI+=histo[idd];
					if (maxI>=thresh)
					{
						maxCut = idd;
						break;
					}
				}
				free(histo);
				
				cerr << "minCut=" << minCut << " maxCut=" << maxCut << endl;
				
				if (tex->internalFormat==1)
				{
					double scaling = 255./(maxCut-minCut);
					GLubyte* data = (GLubyte*)tex->texels;
					for (unsigned int i=0;i<nbPix;++i)
					{
						data[i] = (GLubyte)MY_MIN((MY_MAX((data[i]-minCut), 0)*scaling), 255);
					}
				}
				else if (tex->internalFormat==2 && tex->type==GL_UNSIGNED_SHORT)
				{
					double scaling = 65535./(maxCut-minCut);
					GLushort* data = (GLushort*)tex->texels;
					for (unsigned int i=0;i<nbPix;++i)
						data[i] = (GLushort)MY_MIN((MY_MAX(data[i]-minCut, 0)*scaling), 65535);
				}
				else if (tex->internalFormat==2 && tex->type==GL_SHORT)
				{
					double scaling = 65535./(maxCut-minCut);
					GLshort* data = (GLshort*)tex->texels;
					for (unsigned int i=0;i<nbPix;++i)
						data[i] = (GLshort)MY_MIN((MY_MAX(data[i]-minCut, -32767)*scaling), 32767);
				}
				else
				{
					// Unsupported format..
					qWarning() << "Internal format: " << tex->internalFormat << " is not supported for LUMINANCE texture " << tex->fullPath;
					return false;
				}
			}
			break;
			default:
				// Unsupported dynamic range mode..
				qWarning() << "Dynamic range mode: " << tex->dynamicRangeMode << " is not supported by the texture manager, texture " << tex->fullPath << " will not be loaded.";
				return false;
		}
		return true;
	}
	else
	{
		// TODO, no scaling is currently done for color images
		return true;
	}
}

/*************************************************************************
  Load the image in memory by calling the associated ImageLoader
*************************************************************************/
bool StelTextureMgr::loadImage(STexture* tex)
{
	if (tex->fileExtension.isEmpty())
		tex->fileExtension = QFileInfo(tex->fullPath).suffix();
	QMap<QString, ImageLoader*>::iterator loadFuncIter = imageLoaders.find(tex->fileExtension);
	if (loadFuncIter==imageLoaders.end())
	{
		qWarning() << "Unsupported image file extension: " << tex->fileExtension << " for file: " << tex->fullPath;
		tex->texels = NULL;
		return false;
	}

	// Load the image
	if (!loadFuncIter.value()->loadImage(tex->fullPath, *tex))
	{
		qWarning() << "Image loading failed for file: " << tex->fullPath;
		tex->texels = NULL;
		return false;
	}

	if (!tex->texels)
	{
		qWarning() << "Image loading returned empty texels for file: " << tex->fullPath;
		return false;
	}

	// Apply scaling for the image
	if (reScale(tex)==false)
	{
		free(tex->texels);
		tex->texels = NULL;
		return false;
	}

// 	if (tex->computeLuminance==true)
// 	{
// 		
// 	}
	
	// Repair texture size if non power of 2 is not allowed by the video driver
	if (!isNoPowerOfTwoAllowed && (!StelUtils::isPowerOfTwo(tex->height) || !StelUtils::isPowerOfTwo(tex->width)))
	{
		//cerr << "Can't load natively non power of 2 textures for texture: " << tex->fullPath << endl;
		int w = StelUtils::getBiggerPowerOfTwo(tex->width);
		int h = StelUtils::getBiggerPowerOfTwo(tex->height);
		//cerr << "Resize to " << w << "x" << h << endl;
		
		GLubyte* texels2 = (GLubyte *)calloc (sizeof (GLubyte) * tex->internalFormat,  w*h);
		if (texels2==NULL)
		{
			cerr << "Unsufficient memory for image data allocation: need to allocate array of " << w << "x" << h << " with " << sizeof (GLubyte) * tex->internalFormat << " bytes per pixels." << endl;
			free(tex->texels);
			tex->texels = NULL;
			return false;
		}
		
		// Copy data into the power of two buffer
		for (int j=0;j<tex->height;++j)
		{
			memcpy(&(texels2[j*w*tex->internalFormat]), &(tex->texels[j*tex->width*tex->internalFormat]), tex->width*tex->internalFormat);
		}
		// Copy the last line to all extra each lines to prevent problems with interpolation on textures sides
		for (int j=tex->height;j<h;++j)
		{
			memcpy(&(texels2[j*w*tex->internalFormat]), &(texels2[(tex->height-1)*w*tex->internalFormat]), tex->width*tex->internalFormat);
		}
		if (w>tex->width)
		{
			for (int j=0;j<h;++j)
			{
				if (tex->internalFormat==1)
					memset(&texels2[(j*w+tex->width)], texels2[(j*w+tex->width-1)], (w-tex->width));
				else
				{
					for (int l=0;l<w-tex->width;++l)
						memcpy(&texels2[(j*w+tex->width+l)*tex->internalFormat], &texels2[(j*w+tex->width-1)*tex->internalFormat], tex->internalFormat);
				}
			}
		}
		
		// Update the texture coordinates because the new texture does not occupy the whole buffer
		tex->texCoordinates[0].set((double)tex->width/w, 0.);
		tex->texCoordinates[1].set(0., 0.);
		tex->texCoordinates[2].set((double)tex->width/w, (double)tex->height/h);
		tex->texCoordinates[3].set(0., (double)tex->height/h);
		tex->width = w;
		tex->height = h;
		free(tex->texels);
		tex->texels = texels2;
	}

	// Check that the image size is compatible with the hardware 
	if (tex->width>maxTextureSize)
	{
		qWarning() << "Warning: texture " << tex->fullPath << " is larger than " << maxTextureSize << " pixels and might be not supported.";
	}
	
	// The glLoading will be done later in the main thread
	return true;
}

/*************************************************************************
 Load a PNG image from a file.
 Code borrowed from David HENRY with the following copyright notice:
 * png.c -- png texture loader
 * last modification: feb. 5, 2006
 *
 * Copyright (c) 2005-2006 David HENRY
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*************************************************************************/
bool StelTextureMgr::PngLoader::loadImage(const QString& filename, STexture& texinfo)
{
	png_byte magic[8];
	png_structp png_ptr;
	png_infop info_ptr;
	int bit_depth, color_type;
	FILE *fp = NULL;
	png_bytep *row_pointers = NULL;
	int i;

	/* open image file */
	fp = fopen (QFile::encodeName(filename).constData(), "rb");
	if (!fp)
	{
		qWarning() << "error: couldn't open \""<< filename << "\"!";
		return false;
	}

	/* read magic number */
	fread (magic, 1, sizeof (magic), fp);

	/* check for valid magic number */
	if (!png_check_sig (magic, sizeof (magic)))
	{
		qWarning() << "error: \"" << filename << "\" is not a valid PNG image!";
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

		qWarning() << "There was an error while loading PNG image: " << texinfo.fullPath;
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
	// johannes: never make pointer casts between pointers to int and long,
	// because long is 64 bit on AMD64:
	//   (png_uint_32*)(&texinfo.width)
	// is evel, and does not work for AMD64
	png_uint_32 width,height;
	png_get_IHDR (png_ptr, info_ptr,
	              &width,
	              &height,
	              &bit_depth, &color_type,
	              NULL, NULL, NULL);
	texinfo.width = width;
	texinfo.height = height;

	/* get image format and components per pixel */
	switch (color_type)
	{
	case PNG_COLOR_TYPE_GRAY:
		texinfo.format = GL_LUMINANCE;
		texinfo.internalFormat = 1;
		texinfo.type = GL_UNSIGNED_BYTE;
		break;

	case PNG_COLOR_TYPE_GRAY_ALPHA:
		texinfo.format = GL_LUMINANCE_ALPHA;
		texinfo.internalFormat = 2;
		texinfo.type = GL_UNSIGNED_BYTE;
		break;

	case PNG_COLOR_TYPE_RGB:
		texinfo.format = GL_RGB;
		texinfo.internalFormat = 3;
		texinfo.type = GL_UNSIGNED_BYTE;
		break;

	case PNG_COLOR_TYPE_RGB_ALPHA:
		texinfo.format = GL_RGBA;
		texinfo.internalFormat = 4;
		texinfo.type = GL_UNSIGNED_BYTE;
		break;

	default:
		// Badness
		assert(0);
	}
	
	/* we can now allocate memory for storing pixel data */
	texinfo.texels = (GLubyte *)malloc (sizeof (GLubyte) * texinfo.width
	                                     * texinfo.height * texinfo.internalFormat);
	if (!texinfo.texels)
	{
		qWarning() << "Not enough memory to allocate the PNG image texture " << texinfo.fullPath;
		fclose (fp);
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
		return false;
	}
	
	/* setup a pointer array.  Each one points at the begining of a row. */
	row_pointers = (png_bytep *)malloc (sizeof (png_bytep) * texinfo.height);
	if (!row_pointers)
	{
		qWarning() << "Not enough memory to create the PNG image texture " << texinfo.fullPath;
		fclose (fp);
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
		return false;
	}
	
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

/*************************************************************************
 Load a JPG image from a file.
 Code borrowed from David HENRY with the following copyright notice:
 * jpeg.c -- jpeg texture loader
 * last modification: feb. 9, 2006
 *
 * Copyright (c) 2005-2006 David HENRY
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* jpeg error manager */
struct my_error_mgr
{
	/* "public" fields */
	struct jpeg_error_mgr pub;

	/* for return to caller */
	jmp_buf setjmp_buffer;
};

void err_exit(j_common_ptr cinfo)
{
	/* get error manager */
	my_error_mgr* jerr = (my_error_mgr*)(cinfo->err);

	/* display error message */
	(*cinfo->err->output_message) (cinfo);

	/* return control to the setjmp point */
	longjmp (jerr->setjmp_buffer, 1);
}


bool StelTextureMgr::JpgLoader::loadImage(const QString& filename, STexture& texinfo)
{
	FILE *fp = NULL;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	/* open image file */
	fp = fopen(QFile::encodeName(filename).constData(), "rb");
	if (!fp)
	{
		qWarning() << "Error: couldn't open \"" << filename << "\"!\n";
		return false;
	}

	/* create and configure decompressor */
	jpeg_create_decompress (&cinfo);
	cinfo.err = jpeg_std_error (&jerr.pub);
	jpeg_stdio_src (&cinfo, fp);

	/* configure error manager */
	jerr.pub.error_exit = err_exit;
	texinfo.texels = NULL;
	
	if (setjmp(jerr.setjmp_buffer))
	{
		qWarning() << "Error: couldn't read jpeg file \"" << filename << "\"!";
		jpeg_destroy_decompress(&cinfo);
		if (texinfo.texels)
		{
			free(texinfo.texels);
			texinfo.texels = NULL;
		}
		fclose(fp);
		return false;
	}

	/*
	 * NOTE: this is the simplest "readJpegFile" function. There
	 * is no advanced error handling.  It would be a good idea to
	 * setup an error manager with a setjmp/longjmp mechanism.
	 * In this function, if an error occurs during reading the JPEG
	 * file, the libjpeg aborts the program.
	 * See jpeg_mem.c (or RTFM) for an advanced error handling which
	 * prevent this kind of behavior (http://tfc.duke.free.fr)
	 */

	/* read header and prepare for decompression */
	jpeg_read_header(&cinfo, TRUE);

	jpeg_start_decompress (&cinfo);

	/* initialize image's member variables */
	texinfo.width = cinfo.image_width;
	texinfo.height = cinfo.image_height;
	texinfo.internalFormat = cinfo.num_components;
	texinfo.type = GL_UNSIGNED_BYTE;
	if (cinfo.num_components == 1)
		texinfo.format = GL_LUMINANCE;
	else
		texinfo.format = GL_RGB;

	texinfo.texels = (GLubyte *)malloc (sizeof (GLubyte) * texinfo.width
	                                    * texinfo.height * texinfo.internalFormat);

	/* extract each scanline of the image */
	for (int i = 0; i < texinfo.height; ++i)
	{
		JSAMPROW j = (texinfo.texels +
		     ((texinfo.height - (i + 1)) * texinfo.width * texinfo.internalFormat));
		jpeg_read_scanlines (&cinfo, &j, 1);
	}

	/* finish decompression and release memory */
	jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);

	fclose(fp);
	return true;
}
