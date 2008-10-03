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

/* This is a XV thumbnail image file loading framework */

#include <stdio.h>
#include <string.h>

#include "SDL_image.h"

#ifdef LOAD_XV

static int get_line(SDL_RWops *src, char *line, int size)
{
	while ( size > 0 ) {
		if ( SDL_RWread(src, line, 1, 1) <= 0 ) {
			return -1;
		}
		if ( *line == '\r' ) {
			continue;
		}
		if ( *line == '\n' ) {
			*line = '\0';
			return 0;
		}
		++line;
		--size;
	}
	/* Out of space for the line */
	return -1;
}

static int get_header(SDL_RWops *src, int *w, int *h)
{
	char line[1024];

	*w = 0;
	*h = 0;

	/* Check the header magic */
	if ( (get_line(src, line, sizeof(line)) < 0) ||
	     (memcmp(line, "P7 332", 6) != 0) ) {
		return -1;
	}

	/* Read the header */
	while ( get_line(src, line, sizeof(line)) == 0 ) {
		if ( memcmp(line, "#BUILTIN:", 9) == 0 ) {
			/* Builtin image, no data */
			break;
		}
		if ( memcmp(line, "#END_OF_COMMENTS", 16) == 0 ) {
			if ( get_line(src, line, sizeof(line)) == 0 ) {
				sscanf(line, "%d %d", w, h);
				if ( *w >= 0 && *h >= 0 ) {
					return 0;
				}
			}
			break;
		}
	}
	/* No image data */
	return -1;
}

/* See if an image is contained in a data source */
int IMG_isXV(SDL_RWops *src)
{
	int start;
	int is_XV;
	int w, h;

	if ( !src )
		return 0;
	start = SDL_RWtell(src);
	is_XV = 0;
	if ( get_header(src, &w, &h) == 0 ) {
		is_XV = 1;
	}
	SDL_RWseek(src, start, SEEK_SET);
	return(is_XV);
}

/* Load a XV thumbnail image from an SDL datasource */
SDL_Surface *IMG_LoadXV_RW(SDL_RWops *src)
{
	int start;
	const char *error = NULL;
	SDL_Surface *surface = NULL;
	int w, h;
	Uint8 *pixels;

	if ( !src ) {
		/* The error message has been set in SDL_RWFromFile */
		return NULL;
	}
	start = SDL_RWtell(src);

	/* Read the header */
	if ( get_header(src, &w, &h) < 0 ) {
		error = "Unsupported image format";
		goto done;
	}

	/* Create the 3-3-2 indexed palette surface */
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0xe0, 0x1c, 0x03, 0);
	if ( surface == NULL ) {
		error = "Out of memory";
		goto done;
	}

	/* Load the image data */
	for ( pixels = (Uint8 *)surface->pixels; h > 0; --h ) {
		if ( SDL_RWread(src, pixels, w, 1) <= 0 ) {
			error = "Couldn't read image data";
			goto done;
		}
		pixels += surface->pitch;
	}

done:
	if ( error ) {
		SDL_RWseek(src, start, SEEK_SET);
		if ( surface ) {
			SDL_FreeSurface(surface);
			surface = NULL;
		}
		IMG_SetError(error);
	}
	return surface;
}

#else

/* See if an image is contained in a data source */
int IMG_isXV(SDL_RWops *src)
{
	return(0);
}

/* Load a XXX type image from an SDL datasource */
SDL_Surface *IMG_LoadXV_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_XV */
