/*
 * PNG loader library for OpenGL v1.45 (10/07/00)
 * by Ben Wyatt ben@wyatt100.freeserve.co.uk
 * Using LibPNG 1.0.2 and ZLib 1.1.3
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the author be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is hereby granted to use, copy, modify, and distribute this
 * source code, or portions hereof, for any purpose, without fee, subject to
 * the following restrictions:
 *
 * 1. The origin of this source code must not be misrepresented. You must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered versions must be plainly marked as such and must not be
 *    misrepresented as being the original source.
 * 3. This notice must not be removed or altered from any source distribution.
 */

#ifndef _GLPNG_H_
#define _GLPNG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
	#ifdef _DEBUG
		#pragma comment (lib, "glpngd.lib")
	#else
		#pragma comment (lib, "glpng.lib")
	#endif
#endif

/* XXX This is from Win32's <windef.h> */
#ifndef APIENTRY
	#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
		#define APIENTRY    __stdcall
	#else
		#define APIENTRY
	#endif
#endif

/* Mipmapping parameters */
#define PNG_NOMIPMAPS      0 /* No mipmapping                        */
#define PNG_BUILDMIPMAPS  -1 /* Calls a clone of gluBuild2DMipmaps() */
#define PNG_SIMPLEMIPMAPS -2 /* Generates mipmaps without filtering  */

/* Who needs an "S" anyway? */
#define PNG_NOMIPMAP     PNG_NOMIPMAPS
#define PNG_BUILDMIPMAP  PNG_BUILDMIPMAPS
#define PNG_SIMPLEMIPMAP PNG_SIMPLEMIPMAPS

/* Transparency parameters */
#define PNG_CALLBACK  -3 /* Call the callback function to generate alpha   */
#define PNG_ALPHA     -2 /* Use alpha channel in PNG file, if there is one */
#define PNG_SOLID     -1 /* No transparency                                */
#define PNG_STENCIL    0 /* Sets alpha to 0 for r=g=b=0, 1 otherwise       */
#define PNG_BLEND1     1 /* a = r+g+b                                      */
#define PNG_BLEND2     2 /* a = (r+g+b)/2                                  */
#define PNG_BLEND3     3 /* a = (r+g+b)/3                                  */
#define PNG_BLEND4     4 /* a = r*r+g*g+b*b                                */
#define PNG_BLEND5     5 /* a = (r*r+g*g+b*b)/2                            */
#define PNG_BLEND6     6 /* a = (r*r+g*g+b*b)/3                            */
#define PNG_BLEND7     7 /* a = (r*r+g*g+b*b)/4                            */
#define PNG_BLEND8     8 /* a = sqrt(r*r+g*g+b*b)                          */

typedef struct {
	unsigned int Width;
	unsigned int Height;
	unsigned int Depth;
	unsigned int Alpha;
} pngInfo;

typedef struct {
	unsigned int Width;
	unsigned int Height;
	unsigned int Depth;
	unsigned int Alpha;

	unsigned int Components;
	unsigned char *Data;
	unsigned char *Palette;
} pngRawInfo;

extern int APIENTRY pngLoadRaw(const char *filename, pngRawInfo *rawinfo);
extern int APIENTRY pngLoadRawF(FILE *file, pngRawInfo *rawinfo);

extern int APIENTRY pngLoad(const char *filename, int mipmap, int trans, pngInfo *info);
extern int APIENTRY pngLoadF(FILE *file, int mipmap, int trans, pngInfo *info);

extern unsigned int APIENTRY pngBind(const char *filename, int mipmap, int trans, pngInfo *info, int wrapst, int minfilter, int magfilter);
extern unsigned int APIENTRY pngBindF(FILE *file, int mipmap, int trans, pngInfo *info, int wrapst, int minfilter, int magfilter);

extern void APIENTRY pngSetStencil(unsigned char red, unsigned char green, unsigned char blue);
extern void APIENTRY pngSetAlphaCallback(unsigned char (*callback)(unsigned char red, unsigned char green, unsigned char blue));
extern void APIENTRY pngSetViewingGamma(double viewingGamma);
extern void APIENTRY pngSetStandardOrientation(int standardorientation);

#ifdef __cplusplus
}
#endif

#endif
