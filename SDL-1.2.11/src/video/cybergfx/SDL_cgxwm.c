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

#include "SDL_version.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#include "SDL_syswm.h"
#include "../../events/SDL_events_c.h"
#include "../SDL_pixels_c.h"
#include "SDL_cgxmodes_c.h"
#include "SDL_cgxwm_c.h"

/* This is necessary for working properly with Enlightenment, etc. */
#define USE_ICON_WINDOW

void CGX_SetIcon(_THIS, SDL_Surface *icon, Uint8 *mask)
{
/* Not yet implemented */
}

void CGX_SetCaption(_THIS, const char *title, const char *icon)
{
	if(SDL_Window)
		SetWindowTitles(SDL_Window,(char *)title,NULL);
}

/* Iconify the window */
int CGX_IconifyWindow(_THIS)
{
/* Not yet implemented */
	return 0;
}

int CGX_GetWMInfo(_THIS, SDL_SysWMinfo *info)
{
	if ( info->version.major <= SDL_MAJOR_VERSION ) {
		return(1);
	} else {
		SDL_SetError("Application not compiled with SDL %d.%d\n",
					SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
		return(-1);
	}
}
