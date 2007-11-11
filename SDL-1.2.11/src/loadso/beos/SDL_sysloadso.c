/*
    SDL - Simple DirectMedia Layer
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
#include "SDL_config.h"

#ifdef SDL_LOADSO_BEOS

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#include <stdio.h>
#include <be/kernel/image.h>

#include "SDL_loadso.h"

void *SDL_LoadObject(const char *sofile)
{
	void *handle = NULL;
	const char *loaderror = "Unknown error";
	image_id library_id = load_add_on(sofile);
	if ( library_id == B_ERROR ) {
		loaderror = "BeOS error";
	} else {
		handle = (void *)(library_id);
	}

	if ( handle == NULL ) {
		SDL_SetError("Failed loading %s: %s", sofile, loaderror);
	}
	return(handle);
}

void *SDL_LoadFunction(void *handle, const char *name)
{
	void *symbol = NULL;
	const char *loaderror = "Unknown error";
	image_id library_id = (image_id)handle;
	if ( get_image_symbol(library_id,
		name, B_SYMBOL_TYPE_TEXT, &symbol) != B_NO_ERROR ) {
		loaderror = "Symbol not found";
	}

	if ( symbol == NULL ) {
		SDL_SetError("Failed loading %s: %s", name, loaderror);
	}
	return(symbol);
}

void SDL_UnloadObject(void *handle)
{
	image_id library_id;
	if ( handle != NULL ) {
		library_id = (image_id)handle;
		unload_add_on(library_id);
	}
}

#endif /* SDL_LOADSO_BEOS */
