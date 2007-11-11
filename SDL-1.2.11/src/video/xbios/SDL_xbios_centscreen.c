/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/*
	Centscreen extension definitions

	Patrice Mandin
*/

#include <mint/falcon.h>

#include "SDL_xbios.h"
#include "SDL_xbios_centscreen.h"

int SDL_XBIOS_CentscreenInit(_THIS)
{
	centscreen_mode_t	curmode, listedmode;
	unsigned long result;
	int cur_handle;	/* Current Centscreen mode handle */

	/* Reset current mode list */
	if (XBIOS_modelist) {
		SDL_free(XBIOS_modelist);
		XBIOS_nummodes = 0;
		XBIOS_modelist = NULL;
	}

	/* Add Centscreen modes */
	Vread(&curmode);
	cur_handle = curmode.handle;
	curmode.mode = curmode.physx = curmode.physy = curmode.plan =
		curmode.logx = curmode.logy = -1;

	result = Vfirst(&curmode, &listedmode);
	if (result==0) {
		while (result==0) {
			/* Don't add modes with virtual screen */
			if ((listedmode.mode & CSCREEN_VIRTUAL)==0) {
				/* Don't add modes with bpp<8 */
				if (listedmode.plan>=8) {
					SDL_XBIOS_AddMode(this, listedmode.mode, listedmode.physx,
						listedmode.physy, listedmode.plan, SDL_FALSE
					);
				}
			}
			SDL_memcpy(&curmode, &listedmode, sizeof(centscreen_mode_t));
			curmode.mode = curmode.physx = curmode.physy = curmode.plan =
				curmode.logx = curmode.logy = -1;
			result = Vnext(&curmode, &listedmode);
		}		
	} else {
		fprintf(stderr, "No suitable Centscreen modes\n");
	}

	return cur_handle;
}

void SDL_XBIOS_CentscreenSetmode(_THIS, int width, int height, int planes)
{
	centscreen_mode_t	newmode, curmode;
	
	newmode.handle = newmode.mode = newmode.logx = newmode.logy = -1;
	newmode.physx = width;
	newmode.physy = height;
	newmode.plan = planes;
	Vwrite(0, &newmode, &curmode);

	/* Disable screensaver */
	Vread(&newmode);
	newmode.mode &= ~(CSCREEN_SAVER|CSCREEN_ENERGYSTAR);
	Vwrite(0, &newmode, &curmode);
}

void SDL_XBIOS_CentscreenRestore(_THIS, int prev_handle)
{
	centscreen_mode_t	newmode, curmode;

	/* Restore old video mode */
	newmode.handle = prev_handle;
	newmode.mode = newmode.physx = newmode.physy = newmode.plan =
		newmode.logx = newmode.logy = -1;
	Vwrite(0, &newmode, &curmode);
}
