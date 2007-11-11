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

#include "SDL_mouse.h"
#include "../../events/SDL_events_c.h"
#include "../SDL_cursor_c.h"
#include "SDL_amigamouse_c.h"


/* The implementation dependent data for the window manager cursor */

typedef void * WMCursor;

void amiga_FreeWMCursor(_THIS, WMcursor *cursor)
{
}

WMcursor *amiga_CreateWMCursor(_THIS,
		Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y)
{
	return (WMcursor *)1; // Amiga has an Hardware cursor, so it's ok to return something unuseful but true
}

int amiga_ShowWMCursor(_THIS, WMcursor *cursor)
{
	/* Don't do anything if the display is gone */
	if ( SDL_Display == NULL) {
		return(0);
	}

	/* Set the Amiga prefs cursor cursor, or blank if cursor is NULL */

	if ( SDL_Window ) {
		SDL_Lock_EventThread();
		if ( cursor == NULL ) {
			if ( SDL_BlankCursor != NULL ) {
// Hide cursor HERE
				SetPointer(SDL_Window,(UWORD *)SDL_BlankCursor,1,1,0,0);
			}
		} else {
// Show cursor
			ClearPointer(SDL_Window);
		}
		SDL_Unlock_EventThread();
	}
	return(1);
}

void amiga_WarpWMCursor(_THIS, Uint16 x, Uint16 y)
{
/* FIXME: Not implemented */
}

/* Check to see if we need to enter or leave mouse relative mode */
void amiga_CheckMouseMode(_THIS)
{
}
