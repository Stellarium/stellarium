/*
 * Copyright (C) 2003 Fabien Chéreau
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

#include "stel_sdl.h"

stel_sdl::stel_sdl(stel_core * _core)
{
	if (!_core)
	{
		printf("ERROR : In stel_sdl constructor, unvalid core.");
		exit(-1);
	}
	core = _core;
}

stel_sdl::~stel_sdl()
{
}

void stel_sdl::init(void)
{
    Uint32	Vflags;		// Our Video Flags
    Screen = NULL;

	// Init the SDL library, the VIDEO subsystem
    if(SDL_Init(SDL_INIT_VIDEO)<0)
    {
         printf("Unable to open SDL: %s\n", SDL_GetError() );
         exit(-1);
    }

    // Make sure that SDL_Quit will be called in case of exit()
    atexit(SDL_Quit);

	// Might not work TODO check how to handle that
	//SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24);

    // We want a hardware surface
    Vflags = SDL_HWSURFACE|SDL_OPENGL;//|SDL_DOUBLEBUF;
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,1);
	// If fullscreen, set the Flag
    if (core->get_Fullscreen()) Vflags|=SDL_FULLSCREEN;

	// Create the SDL screen surface
    Screen = SDL_SetVideoMode(core->get_screen_W(), core->get_screen_H(), core->get_bppMode(), Vflags);
	if(!Screen)
	{
		printf("sdl: Couldn't set %dx%d video mode: %s!",
		core->get_screen_W(), core->get_screen_H(), SDL_GetError());
		exit(-1);
	}

	// Disable key repeat
    SDL_EnableKeyRepeat(0, 0);

	// Set the window caption
	SDL_WM_SetCaption(APP_NAME, APP_NAME);

	// Set the window icon
	SDL_WM_SetIcon(SDL_LoadBMP((core->get_DataDir() + "icon.bmp").c_str()), NULL);

	// Hold the value of SDL_GetTicks at the program init (set 0 time)
	LastCount = SDL_GetTicks();

}

// Terminate the application
void TerminateApplication(void)
{
	static SDL_Event Q;						// Send a SDL_QUIT event
	Q.type = SDL_QUIT;						// To the SDL event queue
	if(SDL_PushEvent(&Q) == -1)				// Try to send the event
	{
		printf("SDL_QUIT event can't be pushed: %s\n", SDL_GetError() );
		exit(-1);
	}
}

void stel_sdl::start_main_loop(void)
{
    bool AppVisible = true;			// At The Beginning, Our App Is Visible

	// Start the main loop
	while(1)
	{
		if(SDL_PollEvent(&E))	// Fetch The First Event Of The Queue
		{
			switch(E.type)		// And Processing It
			{
				case SDL_QUIT:
					return;
					break;

				case SDL_VIDEORESIZE:
					// Recalculate The OpenGL Scene Data For The New Window
					resize_GL(E.resize.w, E.resize.h);
					break;

				case SDL_ACTIVEEVENT:
					if (E.active.state & SDL_APPACTIVE)
					{
						// Activity level changed (ie. iconified)
						if (E.active.gain) AppVisible = true; // Activity's been gained
						else AppVisible = false;
					}
					break;

				case SDL_MOUSEMOTION:
				  	core->handle_move(E.motion.x,E.motion.y);
					break;

				case SDL_MOUSEBUTTONDOWN:
					core->handle_clic(E.button.x,E.button.y,E.button.button,E.button.state);
					break;

				case SDL_MOUSEBUTTONUP:
					core->handle_clic(E.button.x,E.button.y,E.button.button,E.button.state);
					break;

				case SDL_KEYDOWN:
					// Send the event to the gui and stop if it has been intercepted
					if (!core->handle_keys(E.key.keysym.sym,S_GUI_PRESSED))
					{
						if (E.key.keysym.sym==SDLK_F1) SDL_WM_ToggleFullScreen(Screen); // Try fullscreen
						if (E.key.keysym.sym==SDLK_q && (SDL_GetModState() & KMOD_CTRL))
						{
							TerminateApplication();
						}
					}
					// Rescue escape in case of lock : CTRL + ESC forces brutal quit
					if (E.key.keysym.sym==SDLK_ESCAPE && (SDL_GetModState() & KMOD_CTRL)) TerminateApplication();
					break;

				case SDL_KEYUP:
					core->handle_keys(E.key.keysym.sym,S_GUI_RELEASED);
			}
		}
		else  // No events to poll
		{
			// If the application is not visible
			if(!AppVisible)
			{
				// Leave the CPU alone, don't waste time, simply wait for an event
				SDL_WaitEvent(NULL);
			}
			else
			{

				TickCount = SDL_GetTicks();			// Get present ticks
				core->update(TickCount-LastCount);	// And update the motions and data
				core->draw(TickCount-LastCount);	// Do the drawings!
				LastCount = TickCount;				// Save the present tick probing
				SDL_GL_SwapBuffers();				// And swap the buffers
			}
		}
	}
}


void stel_sdl::resize_GL(int w, int h)
{
    if (!h || !w) return;
	core->set_screen_size(w, h);
}

