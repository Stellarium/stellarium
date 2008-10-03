/*
    SDL_image:  An example image loading library for use with SDL
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* This is a BMP image file loading framework */

#include <stdio.h>
#include <string.h>

#include "SDL_image.h"

#ifdef LOAD_BMP

/* See if an image is contained in a data source */
int IMG_isBMP(SDL_RWops *src)
{
	int start;
	int is_BMP;
	char magic[2];

	if ( !src )
		return 0;
	start = SDL_RWtell(src);
	is_BMP = 0;
	if ( SDL_RWread(src, magic, sizeof(magic), 1) ) {
		if ( strncmp(magic, "BM", 2) == 0 ) {
			is_BMP = 1;
		}
	}
	SDL_RWseek(src, start, SEEK_SET);
	return(is_BMP);
}

#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_endian.h"

/* Compression encodings for BMP files */
#ifndef BI_RGB
#define BI_RGB		0
#define BI_RLE8		1
#define BI_RLE4		2
#define BI_BITFIELDS	3
#endif

static int readRlePixels(SDL_Surface * surface, SDL_RWops * src, int isRle8)
{
	/*
	| Sets the surface pixels from src.  A bmp image is upside down.
	*/
	int pitch = surface->pitch;
	int height = surface->h;
	Uint8 * bits = (Uint8 *)surface->pixels + ((height-1) * pitch);
	int ofs = 0;
	Uint8 ch;
	Uint8 needsPad;

	for (;;) {
		if ( !SDL_RWread(src, &ch, 1, 1) ) return 1;
		/*
		| encoded mode starts with a run length, and then a byte
		| with two colour indexes to alternate between for the run
		*/
		if ( ch ) {
			Uint8 pixel;
			if ( !SDL_RWread(src, &pixel, 1, 1) ) return 1;
			if ( isRle8 ) {                 /* 256-color bitmap, compressed */
				do {
					bits[ofs++] = pixel;
				} while (--ch);
			}else {                         /* 16-color bitmap, compressed */
				Uint8 pixel0 = pixel >> 4;
				Uint8 pixel1 = pixel & 0x0F;
				for (;;) {
					bits[ofs++] = pixel0;     /* even count, high nibble */
					if (!--ch) break;
					bits[ofs++] = pixel1;     /* odd count, low nibble */
					if (!--ch) break;
				}
			}
		} else {
			/*
			| A leading zero is an escape; it may signal the end of the bitmap,
			| a cursor move, or some absolute data.
			| zero tag may be absolute mode or an escape
			*/
			if ( !SDL_RWread(src, &ch, 1, 1) ) return 1;
			switch (ch) {
			case 0:                         /* end of line */
				ofs = 0;
				bits -= pitch;               /* go to previous */
				break;
			case 1:                         /* end of bitmap */
				return 0;                    /* success! */
			case 2:                         /* delta */
				if ( !SDL_RWread(src, &ch, 1, 1) ) return 1;
				ofs += ch;
				if ( !SDL_RWread(src, &ch, 1, 1) ) return 1;
				bits -= (ch * pitch);
				break;
			default:                        /* no compression */
				if (isRle8) {
					needsPad = ( ch & 1 );
					do {
						if ( !SDL_RWread(src, bits + ofs++, 1, 1) ) return 1;
					} while (--ch);
				} else {
					needsPad = ( ((ch+1)>>1) & 1 ); /* (ch+1)>>1: bytes size */
					for (;;) {
						Uint8 pixel;
						if ( !SDL_RWread(src, &pixel, 1, 1) ) return 1;
						bits[ofs++] = pixel >> 4;
						if (!--ch) break;
						bits[ofs++] = pixel & 0x0F;
						if (!--ch) break;
					}
				}
				/* pad at even boundary */
				if ( needsPad && !SDL_RWread(src, &ch, 1, 1) ) return 1;
				break;
			}
		}
	}
}

static SDL_Surface *LoadBMP_RW (SDL_RWops *src, int freesrc)
{
	int was_error;
	long fp_offset;
	int bmpPitch;
	int i, pad;
	SDL_Surface *surface;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	SDL_Palette *palette;
	Uint8 *bits;
	int ExpandBMP;

	/* The Win32 BMP file header (14 bytes) */
	char   magic[2];
	Uint32 bfSize;
	Uint16 bfReserved1;
	Uint16 bfReserved2;
	Uint32 bfOffBits;

	/* The Win32 BITMAPINFOHEADER struct (40 bytes) */
	Uint32 biSize;
	Sint32 biWidth;
	Sint32 biHeight;
	Uint16 biPlanes;
	Uint16 biBitCount;
	Uint32 biCompression;
	Uint32 biSizeImage;
	Sint32 biXPelsPerMeter;
	Sint32 biYPelsPerMeter;
	Uint32 biClrUsed;
	Uint32 biClrImportant;

	/* Make sure we are passed a valid data source */
	surface = NULL;
	was_error = 0;
	if ( src == NULL ) {
		was_error = 1;
		goto done;
	}

	/* Read in the BMP file header */
	fp_offset = SDL_RWtell(src);
	SDL_ClearError();
	if ( SDL_RWread(src, magic, 1, 2) != 2 ) {
		SDL_Error(SDL_EFREAD);
		was_error = 1;
		goto done;
	}
	if ( strncmp(magic, "BM", 2) != 0 ) {
		SDL_SetError("File is not a Windows BMP file");
		was_error = 1;
		goto done;
	}
	bfSize		= SDL_ReadLE32(src);
	bfReserved1	= SDL_ReadLE16(src);
	bfReserved2	= SDL_ReadLE16(src);
	bfOffBits	= SDL_ReadLE32(src);

	/* Read the Win32 BITMAPINFOHEADER */
	biSize		= SDL_ReadLE32(src);
	if ( biSize == 12 ) {
		biWidth		= (Uint32)SDL_ReadLE16(src);
		biHeight	= (Uint32)SDL_ReadLE16(src);
		biPlanes	= SDL_ReadLE16(src);
		biBitCount	= SDL_ReadLE16(src);
		biCompression	= BI_RGB;
		biSizeImage	= 0;
		biXPelsPerMeter	= 0;
		biYPelsPerMeter	= 0;
		biClrUsed	= 0;
		biClrImportant	= 0;
	} else {
		biWidth		= SDL_ReadLE32(src);
		biHeight	= SDL_ReadLE32(src);
		biPlanes	= SDL_ReadLE16(src);
		biBitCount	= SDL_ReadLE16(src);
		biCompression	= SDL_ReadLE32(src);
		biSizeImage	= SDL_ReadLE32(src);
		biXPelsPerMeter	= SDL_ReadLE32(src);
		biYPelsPerMeter	= SDL_ReadLE32(src);
		biClrUsed	= SDL_ReadLE32(src);
		biClrImportant	= SDL_ReadLE32(src);
	}

	/* Check for read error */
	if ( strcmp(SDL_GetError(), "") != 0 ) {
		was_error = 1;
		goto done;
	}

	/* Expand 1 and 4 bit bitmaps to 8 bits per pixel */
	switch (biBitCount) {
		case 1:
		case 4:
			ExpandBMP = biBitCount;
			biBitCount = 8;
			break;
		default:
			ExpandBMP = 0;
			break;
	}

	/* RLE4 and RLE8 BMP compression is supported */
	Rmask = Gmask = Bmask = Amask = 0;
	switch (biCompression) {
		case BI_RGB:
			/* If there are no masks, use the defaults */
			if ( bfOffBits == (14+biSize) ) {
				/* Default values for the BMP format */
				switch (biBitCount) {
					case 15:
					case 16:
						Rmask = 0x7C00;
						Gmask = 0x03E0;
						Bmask = 0x001F;
						break;
					case 24:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
					        Rmask = 0x000000FF;
					        Gmask = 0x0000FF00;
					        Bmask = 0x00FF0000;
#else
						Rmask = 0x00FF0000;
						Gmask = 0x0000FF00;
						Bmask = 0x000000FF;
#endif
						break;
					case 32:
						Amask = 0xFF000000;
						Rmask = 0x00FF0000;
						Gmask = 0x0000FF00;
						Bmask = 0x000000FF;
						break;
					default:
						break;
				}
				break;
			}
			/* Fall through -- read the RGB masks */

		default:
			switch (biBitCount) {
				case 15:
				case 16:
				case 32:
					Rmask = SDL_ReadLE32(src);
					Gmask = SDL_ReadLE32(src);
					Bmask = SDL_ReadLE32(src);
					Amask = SDL_ReadLE32(src);
					break;
				default:
					break;
			}
			break;
	}

	/* Create a compatible surface, note that the colors are RGB ordered */
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
			biWidth, biHeight, biBitCount, Rmask, Gmask, Bmask, Amask);
	if ( surface == NULL ) {
		was_error = 1;
		goto done;
	}

	/* Load the palette, if any */
	palette = (surface->format)->palette;
	if ( palette ) {
		if ( SDL_RWseek(src, fp_offset+14+biSize, SEEK_SET) < 0 ) {
			SDL_Error(SDL_EFSEEK);
			was_error = 1;
			goto done;
		}

		/*
		| guich: always use 1<<bpp b/c some bitmaps can bring wrong information
		| for colorsUsed
		*/
		/* if ( biClrUsed == 0 ) {  */
		biClrUsed = 1 << biBitCount;
		/* } */
		if ( biSize == 12 ) {
			for ( i = 0; i < (int)biClrUsed; ++i ) {
				SDL_RWread(src, &palette->colors[i].b, 1, 1);
				SDL_RWread(src, &palette->colors[i].g, 1, 1);
				SDL_RWread(src, &palette->colors[i].r, 1, 1);
				palette->colors[i].unused = 0;
			}	
		} else {
			for ( i = 0; i < (int)biClrUsed; ++i ) {
				SDL_RWread(src, &palette->colors[i].b, 1, 1);
				SDL_RWread(src, &palette->colors[i].g, 1, 1);
				SDL_RWread(src, &palette->colors[i].r, 1, 1);
				SDL_RWread(src, &palette->colors[i].unused, 1, 1);
			}	
		}
		palette->ncolors = biClrUsed;
	}

	/* Read the surface pixels.  Note that the bmp image is upside down */
	if ( SDL_RWseek(src, fp_offset+bfOffBits, SEEK_SET) < 0 ) {
		SDL_Error(SDL_EFSEEK);
		was_error = 1;
		goto done;
	}
	if ((biCompression == BI_RLE4) || (biCompression == BI_RLE8)) {
		was_error = readRlePixels(surface, src, biCompression == BI_RLE8);
		if (was_error) SDL_SetError("Error reading from BMP");
		goto done;
	}
	bits = (Uint8 *)surface->pixels+(surface->h*surface->pitch);
	switch (ExpandBMP) {
		case 1:
			bmpPitch = (biWidth + 7) >> 3;
			pad  = (((bmpPitch)%4) ? (4-((bmpPitch)%4)) : 0);
			break;
		case 4:
			bmpPitch = (biWidth + 1) >> 1;
			pad  = (((bmpPitch)%4) ? (4-((bmpPitch)%4)) : 0);
			break;
		default:
			pad  = ((surface->pitch%4) ?
					(4-(surface->pitch%4)) : 0);
			break;
	}
	while ( bits > (Uint8 *)surface->pixels ) {
		bits -= surface->pitch;
		switch (ExpandBMP) {
			case 1:
			case 4: {
			Uint8 pixel = 0;
			int   shift = (8-ExpandBMP);
			for ( i=0; i<surface->w; ++i ) {
				if ( i%(8/ExpandBMP) == 0 ) {
					if ( !SDL_RWread(src, &pixel, 1, 1) ) {
						SDL_SetError(
					"Error reading from BMP");
						was_error = 1;
						goto done;
					}
				}
				*(bits+i) = (pixel>>shift);
				pixel <<= ExpandBMP;
			} }
			break;

			default:
			if ( SDL_RWread(src, bits, 1, surface->pitch)
							 != surface->pitch ) {
				SDL_Error(SDL_EFREAD);
				was_error = 1;
				goto done;
			}
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			/* Byte-swap the pixels if needed. Note that the 24bpp
			   case has already been taken care of above. */
			switch(biBitCount) {
				case 15:
				case 16: {
				        Uint16 *pix = (Uint16 *)bits;
					for(i = 0; i < surface->w; i++)
					        pix[i] = SDL_Swap16(pix[i]);
					break;
				}

				case 32: {
				        Uint32 *pix = (Uint32 *)bits;
					for(i = 0; i < surface->w; i++)
					        pix[i] = SDL_Swap32(pix[i]);
					break;
				}
			}
#endif
			break;
		}
		/* Skip padding bytes, ugh */
		if ( pad ) {
			Uint8 padbyte;
			for ( i=0; i<pad; ++i ) {
				SDL_RWread(src, &padbyte, 1, 1);
			}
		}
	}
done:
	if ( was_error ) {
		if ( src ) {
			SDL_RWseek(src, fp_offset, SEEK_SET);
		}
		if ( surface ) {
			SDL_FreeSurface(surface);
		}
		surface = NULL;
	}
	if ( freesrc && src ) {
		SDL_RWclose(src);
	}
	return(surface);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_RW(SDL_RWops *src)
{
	return(LoadBMP_RW(src, 0));
}

#else

/* See if an image is contained in a data source */
int IMG_isBMP(SDL_RWops *src)
{
	return(0);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_BMP */
