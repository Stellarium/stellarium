/*
 * Copyright (C) 2003 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// Class which manages SDL in stellarium

#ifndef _STEL_SDL_H_
#define _STEL_SDL_H_

#include "SDL.h"
#include "stellarium.h"
#include "stel_core.h"

class stel_sdl
{
public:
    stel_sdl(stel_core * core);
    virtual ~stel_sdl();
	void init(void);
	void start_main_loop(void);

private:
	stel_core * core;

	// Function to call when the window size changes
	void resize_GL(int w, int h);
	static SDL_Cursor *create_cursor(const char *image[]);

	// SDL managment variables
	SDL_Surface *Screen;// The Screen
    SDL_Event	E;		// And Event Used In The Polling Process
    Uint32	TickCount;	// Used For The Tick Counter
    Uint32	LastCount;	// Used For The Tick Counter
    SDL_Cursor *Cursor;

};

#endif // _STEL_SDL_H_
