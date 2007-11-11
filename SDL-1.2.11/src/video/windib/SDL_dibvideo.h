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

#ifndef _SDL_dibvideo_h
#define _SDL_dibvideo_h

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* for PDA */
typedef enum
{
	SDL_ORIENTATION_UP,
	SDL_ORIENTATION_DOWN,
	SDL_ORIENTATION_LEFT,
	SDL_ORIENTATION_RIGHT
} SDL_ScreenOrientation;

/* Private display data */
struct SDL_PrivateVideoData {
    HBITMAP screen_bmp;
    HPALETTE screen_pal;

#define NUM_MODELISTS	4		/* 8, 16, 24, and 32 bits-per-pixel */
    int SDL_nummodes[NUM_MODELISTS];
    SDL_Rect **SDL_modelist[NUM_MODELISTS];

	SDL_ScreenOrientation orientation;
#ifdef _WIN32_WCE
	int invert; /* do to remove, used by GAPI driver! */
	char hiresFix; /* using hires mode without defining hires resource */
	int supportRotation; /* for Pocket PC devices */
	DWORD origRotation; /* for Pocket PC devices */
#endif
};
/* Old variable names */
#define screen_bmp		(this->hidden->screen_bmp)
#define screen_pal		(this->hidden->screen_pal)
#define SDL_nummodes		(this->hidden->SDL_nummodes)
#define SDL_modelist		(this->hidden->SDL_modelist)

#endif /* _SDL_dibvideo_h */
