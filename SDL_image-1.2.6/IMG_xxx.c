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

/* This is a generic "format not supported" image framework */

#include <stdio.h>

#include "SDL_image.h"

#ifdef LOAD_XXX

/* See if an image is contained in a data source */
int IMG_isXXX(SDL_RWops *src)
{
	int start;
	int is_XXX;

	if ( !src )
		return 0;
	start = SDL_RWtell(src);
	is_XXX = 0;

	/* Detect the image here */

	SDL_RWseek(src, start, SEEK_SET);
	return(is_XXX);
}

/* Load a XXX type image from an SDL datasource */
SDL_Surface *IMG_LoadXXX_RW(SDL_RWops *src)
{
	int start;
	const char *error = NULL;
	SDL_Surface *surface = NULL;

	if ( !src ) {
		/* The error message has been set in SDL_RWFromFile */
		return NULL;
	}
	start = SDL_RWtell(src);

	/* Load the image here */

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
int IMG_isXXX(SDL_RWops *src)
{
	return(0);
}

/* Load a XXX type image from an SDL datasource */
SDL_Surface *IMG_LoadXXX_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_XXX */
