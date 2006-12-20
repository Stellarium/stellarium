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

#include "StelApp.hpp"
extern "C" {
#include <jpeglib.h>
}
#include <png.h>
#include "SDL_thread.h"

#include "StelTextureMgr.hpp"
#include "StelUtils.hpp"

using namespace std;

// Initialize statics
ManagedSTexture StelTextureMgr::NULL_STEXTURE;
StelTextureMgr::PngLoader StelTextureMgr::pngLoader;
StelTextureMgr::JpgLoader StelTextureMgr::jpgLoader;


void ManagedSTexture::load(void)
{
	if (StelApp::getInstance().getTextureManager().loadImage(this)==false || StelApp::getInstance().getTextureManager().glLoadTexture(this)==false)
	{
		cerr << "Couldn't load texture " << this->fullPath << endl;
		loadState = ManagedSTexture::ERROR;
		return;
	}
}

/*************************************************************************
 Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
 *************************************************************************/
void ManagedSTexture::lazyBind()
{
	if (loadState==ManagedSTexture::LOADED)
	{
		bind();
		return;
	}
	if (loadState==ManagedSTexture::UNLOADED)
	{
		load();
	}
}

//! Return the average texture luminance.
//! @return 0 is black, 1 is white
float ManagedSTexture::getAverageLuminance(void)
{
	if (loadState==ManagedSTexture::LOADED)
	{
		if (avgLuminance<0)
			avgLuminance = STexture::getAverageLuminance();
		return avgLuminance;
	}
	return 0;
};

/*************************************************************************
 Constructor for the StelTextureMgr class
*************************************************************************/
StelTextureMgr::StelTextureMgr(const std::string& atextureDir) : textureDir(atextureDir)
{
	loadQueueMutex = SDL_CreateMutex();
	
	// Init default values
	setDefaultParams();
	
	// Add default loaders
	registerImageLoader("png", &pngLoader);
	registerImageLoader("PNG", &pngLoader);
	registerImageLoader("jpeg", &jpgLoader);
	registerImageLoader("JPEG", &jpgLoader);
	registerImageLoader("jpg", &jpgLoader);
	registerImageLoader("JPG", &jpgLoader);
	
	// Used to correct a bug on some nvidia cards
	isNoPowerOfTwoLUMINANCEAllowed = true;
}

/*************************************************************************
 Destructor for the StelTextureMgr class
*************************************************************************/
StelTextureMgr::~StelTextureMgr()
{
	SDL_DestroyMutex(loadQueueMutex);
	loadQueueMutex = NULL;
}

/*************************************************************************
 Initialize some variable from the openGL context.
 Must be called after the creation of the openGL context.
*************************************************************************/
void StelTextureMgr::init()
{
	// Check vendor and renderer
	string glRenderer((char*)glGetString(GL_RENDERER));
	string glVendor((char*)glGetString(GL_VENDOR));
	string glVersion((char*)glGetString(GL_VERSION));
	
	// cout << "VENDOR=" << glVendor << " RENDERER=" << glRenderer << " VERSION=" << glVersion << endl; 
	
	// Check for extensions
	const GLubyte * strExt = glGetString(GL_EXTENSIONS);
	if (glGetError()!=GL_NO_ERROR)
	{
		cerr << "Error while requesting openGL extensions" << endl;
		return;
	}

	// Get whether floating point textures are supported on this video card
	// This enable for high dynamic range textures to be loaded
	isFloatingPointTexAllowed = gluCheckExtension ((const GLubyte*)"GL_ARB_texture_float", strExt);
	
	// Get whether non-power-of-2 and non square 2D textures are supported on this video card
	isNoPowerOfTwoAllowed = gluCheckExtension ((const GLubyte*)"GL_ARB_texture_non_power_of_two", strExt);
	if (glVersion!="" && glVersion[0]=='2')
		isNoPowerOfTwoAllowed = true;

	if (glVendor=="NVIDIA Corporation" && glRenderer=="Quadro NVS 285/PCI/SSE2" && glVersion=="2.0.2 NVIDIA 87.74")
		isNoPowerOfTwoLUMINANCEAllowed = false;
	else
		isNoPowerOfTwoLUMINANCEAllowed = isNoPowerOfTwoAllowed;
		
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
}

/*************************************************************************
 Internal
*************************************************************************/
ManagedSTexture* StelTextureMgr::initTex(const string& afilename)
{
	string filename;
	if (afilename[0]=='/' || (afilename[0]=='.' && afilename[1]=='/'))
		filename = afilename;
	else
		filename = textureDir + afilename;
	if (!StelUtils::fileExists(filename))
	{
		cerr << "WARNING : Can't find texture file " << filename << "!" << endl;
		return NULL;
	}

	ManagedSTexture* tex = new ManagedSTexture();
	// Set parameters than can be set for this texture
	tex->minFilter = (mipmapsMode==true) ? GL_LINEAR_MIPMAP_NEAREST : minFilter;
	tex->magFilter = magFilter;
	tex->wrapMode = wrapMode;
	tex->fullPath = filename;
	tex->mipmapsMode = mipmapsMode;
	
	return tex;
}

/*************************************************************************
 Load an image from a file and create a new texture from it.
*************************************************************************/
ManagedSTexture& StelTextureMgr::createTexture(const string& afilename, bool lazyLoading)
{
	ManagedSTexture* tex = initTex(afilename);
	if (tex==NULL)
		return NULL_STEXTURE;

	// Load only if lazyLoading is not true, else will load later
	if (lazyLoading==true)
		return *tex;
	
	// Simply load everything
	if (loadImage(tex) && glLoadTexture(tex))
		return *tex;
	else
	{
		delete tex;
		return NULL_STEXTURE;
	}
}

/*************************************************************************
 Structure used to pass parameter to loading threads
*************************************************************************/
struct LoadQueueParam
{
	StelTextureMgr* texMgr;
	std::vector<LoadQueueParam*>* loadQueue;
	SDL_mutex* loadQueueMutex;
	SDL_Thread* thread;
	ManagedSTexture* tex;
	
	// those 2 are used to return the final texture and status. Only access them in the main thread!!
	STexture** outTex;
	bool* status;
};


int loadTextureThread(void* tparam)
{
	LoadQueueParam* param = (LoadQueueParam*)tparam;
	
	cout << "Loading texture in thread " <<  param->tex->fullPath << endl;
	assert(param->tex->threadedLoading==true);
	// Load the image
	if (param->texMgr->loadImage(param->tex)==false)
	{
		param->tex->loadState = ManagedSTexture::ERROR;
	}
	// And add it to 
	SDL_mutexP(param->loadQueueMutex);
	param->loadQueue->push_back(param);
	SDL_mutexV(param->loadQueueMutex);
	return 0;
}

bool StelTextureMgr::createTextureThread(STexture** outTex, const std::string& filename, bool* status)
{
	*status = false;
	ManagedSTexture* tex = initTex(filename);
	if (tex==NULL)
	{
		*status = true;	// indicate error
		outTex = NULL;
		return false;
	}
	
	tex->threadedLoading = true;

	LoadQueueParam* tparam = new LoadQueueParam();
	tparam->texMgr = this;
	tparam->loadQueue = &loadQueue;
	tparam->loadQueueMutex = loadQueueMutex;
	tparam->tex = tex;
	tparam->outTex = outTex;
	tparam->status=status;
	tex->loadState = ManagedSTexture::LOADING_IMAGE;
	SDL_Thread *thread = SDL_CreateThread(loadTextureThread, tparam);
	tparam->thread = thread;
	return true;
}


/*************************************************************************
 Update loading of textures in threads
*************************************************************************/
void StelTextureMgr::update()
{
	// Load the successfully loaded images to openGL, because openGL is not thread safe,
	// this has to be done in the main thread.
	SDL_mutexP(loadQueueMutex);
	std::vector<LoadQueueParam*>::iterator iter = loadQueue.begin();
	for (;iter!=loadQueue.end();++iter)
	{
		SDL_WaitThread((*iter)->thread, NULL);	// Ensure the thread is properly destroyed
		if ((*iter)->tex->loadState==ManagedSTexture::ERROR)
		{
			// There was an error while loading the image
			delete (*iter)->tex;
			(*iter)->tex=NULL;
			*((*iter)->outTex) = NULL;
			*((*iter)->status) = true;
		}
		else
		{
			// Create openGL texture
			if (glLoadTexture((*iter)->tex)==false)
			{
				// There was an error while loading the texture to openGL
				delete (*iter)->tex;
				(*iter)->tex=NULL;
				*((*iter)->outTex) = NULL;
				*((*iter)->status) = true;
			}
			else
			{
				*((*iter)->outTex) = (*iter)->tex;
				*((*iter)->status) = true;
			}
		}
		delete (*iter);
	}
	loadQueue.clear();
	SDL_mutexV(loadQueueMutex);
}


/*************************************************************************
  Load the image in memory
*************************************************************************/
bool StelTextureMgr::loadImage(ManagedSTexture* tex)
{
	const string extension = tex->fullPath.substr((tex->fullPath.find_last_of('.', tex->fullPath.size()))+1);\
	std::map<std::string, ImageLoader*>::iterator loadFuncIter = imageLoaders.find(extension);
	if (loadFuncIter==imageLoaders.end())
	{
		cerr << "Unsupported image file extension: " << extension << " for file: " << tex->fullPath << endl;
		tex->loadState = ManagedSTexture::ERROR;	// texture can't be loaded
		return false;
	}

	// Load the image
	loadFuncIter->second->loadImage(tex->fullPath, *tex);

	if (!tex->texels)
	{
		cerr << "Image loading failed for file: " << tex->fullPath << endl;
		tex->loadState = ManagedSTexture::ERROR;	// texture can't be loaded
		return false;
	}

	if ((!isNoPowerOfTwoAllowed || (!isNoPowerOfTwoLUMINANCEAllowed && tex->format==GL_LUMINANCE)) && 
		(!StelUtils::isPowerOfTwo(tex->height) || !StelUtils::isPowerOfTwo(tex->width)))
	{
		//cerr << "Can't load natively non power of 2 textures for texture: " << tex->fullPath << endl;
		int w = StelUtils::getBiggerPowerOfTwo(tex->width);
		int h = StelUtils::getBiggerPowerOfTwo(tex->height);
		//cerr << "Resize to " << w << "x" << h << endl;
		
		GLubyte* texels2 = (GLubyte *)calloc (sizeof (GLubyte) * tex->internalFormat,  w*h);
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
		for (int j=0;j<h;++j)
		{
			memset(&texels2[(j*w+tex->width)*tex->internalFormat], texels2[(j*w+tex->width-1)*tex->internalFormat], w-tex->width);
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
		cerr << "Warning: texture " << tex->fullPath << " is larger than " << maxTextureSize << " pixels and might be not supported." << endl;
	}
	
	// The glLoading will be done later in the main thread
	return true;
}

/*************************************************************************
 Actually load the texture already in the RAM to openGL memory
*************************************************************************/
bool StelTextureMgr::glLoadTexture(ManagedSTexture* tex)
{
	if (!tex->texels)
		return false;
	
	// generate texture
	glGenTextures (1, &(tex->id));
	glBindTexture (GL_TEXTURE_2D, tex->id);

	// setup some parameters for texture filters and mipmapping
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->minFilter);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex->magFilter);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->wrapMode);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->wrapMode);
	glTexImage2D (GL_TEXTURE_2D, 0, tex->internalFormat, tex->width, tex->height, 0, tex->format, GL_UNSIGNED_BYTE, tex->texels);
	if (tex->mipmapsMode==true)
		gluBuild2DMipmaps (GL_TEXTURE_2D, tex->internalFormat, tex->width, tex->height, tex->format, GL_UNSIGNED_BYTE, tex->texels);

	// OpenGL has its own copy of texture data
	free (tex->texels);
	tex->texels = NULL;
	
	tex->loadState = ManagedSTexture::LOADED;	// texture loaded
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
bool StelTextureMgr::PngLoader::loadImage(const string& filename, ManagedSTexture& texinfo)
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
bool StelTextureMgr::JpgLoader::loadImage(const string& filename, ManagedSTexture& texinfo)
{
	FILE *fp = NULL;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW j;
	int i;

	/* open image file */
	fp = fopen (filename.c_str(), "rb");
	if (!fp)
	{
		cerr << "error: couldn't open \"" << filename << "\"!\n";
		return false;
	}

	/* create and configure decompressor */
	jpeg_create_decompress (&cinfo);
	cinfo.err = jpeg_std_error (&jerr);
	jpeg_stdio_src (&cinfo, fp);

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
	jpeg_read_header (&cinfo, TRUE);
	jpeg_start_decompress (&cinfo);

	/* initialize image's member variables */
	texinfo.width = cinfo.image_width;
	texinfo.height = cinfo.image_height;
	texinfo.internalFormat = cinfo.num_components;

	if (cinfo.num_components == 1)
		texinfo.format = GL_LUMINANCE;
	else
		texinfo.format = GL_RGB;

	texinfo.texels = (GLubyte *)malloc (sizeof (GLubyte) * texinfo.width
	                                    * texinfo.height * texinfo.internalFormat);

	/* extract each scanline of the image */
	for (i = 0; i < texinfo.height; ++i)
	{
		j = (texinfo.texels +
		     ((texinfo.height - (i + 1)) * texinfo.width * texinfo.internalFormat));
		jpeg_read_scanlines (&cinfo, &j, 1);
	}

	/* finish decompression and release memory */
	jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);

	fclose (fp);
	return true;
}
