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

#include "SDL_cgxvideo.h"

extern int CGX_SetupImage(_THIS, SDL_Surface *screen);
extern void CGX_DestroyImage(_THIS, SDL_Surface *screen);
extern int CGX_ResizeImage(_THIS, SDL_Surface *screen, Uint32 flags);

extern int CGX_AllocHWSurface(_THIS, SDL_Surface *surface);
extern void CGX_FreeHWSurface(_THIS, SDL_Surface *surface);
extern int CGX_LockHWSurface(_THIS, SDL_Surface *surface);
extern void CGX_UnlockHWSurface(_THIS, SDL_Surface *surface);
extern int CGX_FlipHWSurface(_THIS, SDL_Surface *surface);

extern void CGX_RefreshDisplay(_THIS);
