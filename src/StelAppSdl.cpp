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

#ifdef WIN32
#include "shlobj.h"
#endif

#include <config.h>
#include "stellarium.h"
#include "StelAppSdl.hpp"
#include "StelCore.hpp"
#include "Projector.hpp"
#include "StelFileMgr.hpp"

#ifdef HAVE_SDL_SDL_MIXER_H
#include <SDL/SDL_mixer.h>
#endif

#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

using namespace std;

/*************************************************************************
 *  SDL dependent part
*************************************************************************/
#ifdef USE_SDL 

#include "GLee.h"
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"

void StelAppSdl::setResizable(bool resizable)
{
	Uint32 Vflags = Screen->flags;
	if (resizable) Vflags |= SDL_RESIZABLE;
	else Vflags &= (~SDL_RESIZABLE);
	Screen = SDL_SetVideoMode(Screen->w,Screen->h,
						      Screen->format->BitsPerPixel,
						      Vflags);
	assert(Screen);
}

void StelAppSdl::initOpenGL(int w, int h, int bbpMode, bool fullScreen,
                            string iconFile)
{
	screenW = w;
	screenH = h;

	Screen = NULL;
	Uint32	Vflags;		// Our Video Flags
#ifdef HAVE_SDL_SDL_MIXER_H

	// Init the SDL library, the VIDEO subsystem
	// Tony - added timer
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER)<0)
	{
		// couldn't init audio, so try without
		fprintf(stderr, "Error: unable to open SDL with audio: %s\n", SDL_GetError() );

		if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER)<0)
		{
			fprintf(stderr, "Error: unable to open SDL: %s\n", SDL_GetError() );
			exit(-1);
		}
    } 
	else
	{
		/*
		// initialized with audio enabled
		// TODO: only init audio if config option allows and script needs
		if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers))
		{
			printf("Unable to open audio!\n");
		}

		*/
	}
#else
	// SDL_mixer is not available - no audio
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER)<0)
	{
		fprintf(stderr, "Unable to open SDL: %s\n", SDL_GetError() );
		exit(-1);
	}
#endif

    // Make sure that SDL_Quit will be called in case of exit()
    atexit(SDL_Quit);

	// Might not work TODO check how to handle that
	//SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24);

    // We want a hardware surface
    Vflags = SDL_HWSURFACE|SDL_OPENGL|SDL_RESIZABLE;//|SDL_DOUBLEBUF;
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,1);
	// If fullscreen, set the Flag
    if (fullScreen) Vflags|=SDL_FULLSCREEN;

	// Create the SDL screen surface
    Screen = SDL_SetVideoMode(w, h, bbpMode, Vflags);
	if(!Screen)
	{
		printf("Warning: Couldn't set %dx%d video mode (%s), retrying with stencil size 0\n", w, h, SDL_GetError());
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
		Screen = SDL_SetVideoMode(w, h, bbpMode, Vflags);

		if(!Screen)
		{
			fprintf(stderr, "Error: Couldn't set %dx%d video mode: %s!\n", w, h, SDL_GetError());
			exit(-1);
		}	
	}

	// Disable key repeat
    SDL_EnableKeyRepeat(0, 0);
	SDL_EnableUNICODE(1);

	// set mouse cursor 
static const char *arrow[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "              XXX               ",
  "              X.X               ",
  "              X.X               ",
  "              X.X               ",
  "              X.X               ",
  "              X.X               ",
  "              X.X               ",
  "              XXX               ",
  "                                ",
  "                                ",
  "                                ",
  "   XXXXXXXX         XXXXXXXX    ",
  "   X......X         X......X    ",
  "   XXXXXXXX         XXXXXXXX    ",
  "                                ",
  "                                ",
  "                                ",
  "              XXX               ",
  "              X.X               ",
  "              X.X               ",
  "              X.X               ",
  "              X.X               ",
  "              X.X               ",
  "              X.X               ",
  "              XXX               ",
  "                                ",
  "                                ",
  "15,17"
};

	Cursor = create_cursor(arrow);
	SDL_SetCursor(Cursor);

	// Set the window caption
	SDL_WM_SetCaption(APP_NAME, APP_NAME);

	// Set the window icon
	SDL_Surface *icon = SDL_LoadBMP((iconFile).c_str());
	SDL_WM_SetIcon(icon, NULL);
	SDL_FreeSurface(icon);
	
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);	
}

string StelAppSdl::getVideoModeList(void) const
{
    SDL_Rect **modes;
	int i;
	/* Get available fullscreen/hardware modes */
	modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);
	/* Check is there are any modes available */
	if(modes == (SDL_Rect **)0)
	{
		return "No modes available!\n";
	}
	/* Check if our resolution is restricted */
	if(modes == (SDL_Rect **)-1)
	{
		return "All resolutions available.\n";
	}
	else
	{
		/* Print valid modes */
		ostringstream modesstr;
		for(i=0;modes[i];++i)
			modesstr << modes[i]->w << "x" << modes[i]->h << endl;
		return modesstr.str();
	}
}

// from an sdl wiki
SDL_Cursor* StelAppSdl::create_cursor(const char *image[])
{
  int i, row, col;
  Uint8 data[4*32];
  Uint8 mask[4*32];
  int hot_x, hot_y;

  i = -1;
  for ( row=0; row<32; ++row ) {
    for ( col=0; col<32; ++col ) {
      if ( col % 8 ) {
        data[i] <<= 1;
        mask[i] <<= 1;
      } else {
        ++i;
        data[i] = mask[i] = 0;
      }
      switch (image[4+row][col]) {
        case 'X':
          data[i] |= 0x01;
          mask[i] |= 0x01;
          break;
        case '.':
          mask[i] |= 0x01;
          break;
        case ' ':
          break;
      }
    }
  }
  sscanf(image[4+row], "%d,%d", &hot_x, &hot_y);
  return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}

// Terminate the application
void StelAppSdl::terminateApplication(void)
{
	static SDL_Event Q;						// Send a SDL_QUIT event
	Q.type = SDL_QUIT;						// To the SDL event queue
	if(SDL_PushEvent(&Q) == -1)				// Try to send the event
	{
		cerr << "SDL_QUIT event can't be pushed: " << SDL_GetError() << endl;
		exit(-1);
	}
}

// Set mouse cursor display
void StelAppSdl::showCursor(bool b)
{
	SDL_ShowCursor(b);
}

// DeInit SDL related stuff
void StelAppSdl::deInit()
{
	SDL_FreeCursor(Cursor);
}

//! Swap GL buffer, should be called only for special condition
void StelAppSdl::swapGLBuffers()
{
	SDL_GL_SwapBuffers();
}
	
//! Return the time since when stellarium is running in second
double StelAppSdl::getTotalRunTime() const
{
	return ((double)SDL_GetTicks()+0.5)/1000;
}
	
enum {
	USER_EVENT_TICK
};


Uint32 mytimer_callback(Uint32 interval, void *param)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user.type = SDL_USEREVENT;
	event.user.code = USER_EVENT_TICK;
	event.user.data1 = NULL;
	event.user.data2 = NULL;
	if(SDL_PushEvent(&event) == -1)
	{
		printf("User tick event can't be pushed: %s\n", SDL_GetError() );
		exit(-1);
	}

	// End this timer.
	return 0;
}

void StelAppSdl::startMainLoop()
{
    bool AppVisible = true;			// At The Beginning, Our App Is Visible
    Uint32 last_event_time = SDL_GetTicks();
    // How fast the objects on-screen are moving, in pixels/millisecond.
    double animationSpeed = 0;

    // Hold the value of SDL_GetTicks at start of main loop (set 0 time)
    LastCount = SDL_GetTicks();

	// Start the main loop
	SDL_Event	E;		// And Event Used In The Polling Process
	while(1)
	{
		if(SDL_PollEvent(&E))	// Fetch The First Event Of The Queue
		{
			if (E.type != SDL_USEREVENT) {
				last_event_time = SDL_GetTicks();
			}

			switch(E.type)		// And Processing It
			{
				case SDL_QUIT:
					return;
					break;

				case SDL_VIDEORESIZE:
					// Recalculate The OpenGL Scene Data For The New Window
					if (E.resize.h && E.resize.w)
					{
						Screen = SDL_SetVideoMode(E.resize.w,E.resize.h,
						                          Screen->format->BitsPerPixel,
						                          Screen->flags);
						assert(Screen);
						screenH = E.resize.h;
						screenW = E.resize.w;
						glWindowHasBeenResized(E.resize.w,E.resize.h);
					}
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
				  	handleMove(E.motion.x,E.motion.y, SDLmodToStelMod(SDL_GetModState()));
					break;
				
				case SDL_MOUSEBUTTONDOWN:
					handleClick(E.button.x,E.button.y,E.button.button,Stel_MOUSEBUTTONDOWN, SDLmodToStelMod(SDL_GetModState()));
					break;

				case SDL_MOUSEBUTTONUP:
					handleClick(E.button.x,E.button.y,E.button.button,Stel_MOUSEBUTTONUP, SDLmodToStelMod(SDL_GetModState()));
					break;

				case SDL_KEYDOWN:
					// Send the event to the gui and stop if it has been intercepted
					// use unicode translation, since not keyboard dependent
					// however, for non-printing keys must revert to just keysym... !
					if ((E.key.keysym.unicode && !handleKeys(SDLKeyToStelKey(E.key.keysym.sym),SDLmodToStelMod(E.key.keysym.mod),E.key.keysym.unicode,Stel_KEYDOWN)) ||
						(!E.key.keysym.unicode && !handleKeys(SDLKeyToStelKey(E.key.keysym.sym),SDLmodToStelMod(E.key.keysym.mod),SDLKeyToStelKey(E.key.keysym.sym),Stel_KEYDOWN)))
					{
						if (E.key.keysym.sym==SDLK_F1)
							toggleFullScreen(); // Try fullscreen
					}
					// Rescue escape in case of lock : CTRL + ESC forces brutal quit
					if (E.key.keysym.sym==SDLK_ESCAPE && (SDL_GetModState() & KMOD_CTRL)) terminateApplication();
					break;
					
				case SDL_KEYUP:
					handleKeys(SDLKeyToStelKey(E.key.keysym.sym),SDLmodToStelMod(E.key.keysym.mod),SDLKeyToStelKey(E.key.keysym.sym),Stel_KEYUP);
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
				// Compute how many fps we should run at to get 1 pixel movement each frame.
				double frameRate = 1000. * animationSpeed;
				// If there was user action in the last 2.5 seconds, shoot for the max framerate.
				if (SDL_GetTicks() - last_event_time < 2500 || frameRate > maxfps)
				{
					frameRate = maxfps;
				}
				if (frameRate < minfps)
				{
					frameRate = minfps;
				}

				TickCount = SDL_GetTicks();			// Get present ticks
				// Wait a while if drawing a frame right now would exceed our preferred framerate.
				if (TickCount-LastCount < 1000./frameRate)
				{
					unsigned int delay = (unsigned int) (1000./frameRate) - (TickCount-LastCount);
//					printf("delay=%d\n", delay);
					if (delay < 15) {
						// Less than 15ms, just do a dumb wait.
						SDL_Delay(delay);
					} else {
						// A longer delay. Use this timer song and dance so
						// that the app is still responsive if the user does something.
						if (!SDL_AddTimer(delay, mytimer_callback, NULL))
						{
							cerr << "Error: couldn't create an SDL timer: " << SDL_GetError() << endl;
						}
						SDL_WaitEvent(NULL);
					}
				}

				TickCount = SDL_GetTicks();			// Get present ticks
				this->update(TickCount-LastCount);	// And update the motions and data
				double squaredDistance = this->draw(TickCount-LastCount);	// Do the drawings!
				animationSpeed = sqrt(squaredDistance) / (TickCount-LastCount);
				LastCount = TickCount;				// Save the present tick probing
				SDL_GL_SwapBuffers();				// And swap the buffers
			}
		}
	}
}

void StelAppSdl::saveScreenShot() const
{
	boost::filesystem::path shotdir;

#if defined(WIN32) || defined(MACOSX)
	shotdir = getFileMgr().getDesktopDir();
#else
	shotdir = string(getenv("HOME"));
#endif

#ifdef __x86_64__
	const char *extension = ".ppm";
#else
	const char *extension = ".bmp";
#endif

	boost::filesystem::path shotPath;
	for(int j=0; j<1000; ++j)
	{
		stringstream oss;
		oss << setfill('0') << setw(3) << j;
		shotPath = shotdir / (string("stellarium") + oss.str() + extension);
		if (!boost::filesystem::exists(shotPath))
			break;
	}
	// TODO - if no more filenames available, don't just overwrite the last one
	// we should at least warn the user, perhaps prompt her, "do you want to overwrite?"

	SDL_Surface * temp = SDL_CreateRGBSurface(SDL_SWSURFACE, Screen->w, Screen->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
	0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
	);
	if (temp == NULL) exit(-1);

	unsigned char * pixels = (unsigned char *) malloc(3 * Screen->w * Screen->h);
	if (pixels == NULL)
	{
		SDL_FreeSurface(temp);
		exit(-1);
	}

	glReadPixels(0, 0, Screen->w, Screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	for (int i=0; i<Screen->h; i++)
	{
		memcpy(((char *) temp->pixels) + temp->pitch * i,
				pixels + 3*Screen->w * (Screen->h-i-1), Screen->w*3);
	}
	free(pixels);

#ifdef __x86_64__
	  // workaround because SDL_SaveBMP is buggy on x86_64
	FILE *f = fopen(shotPath.string().c_str(),"wb");
	if (f) {
		fprintf(f,"P6\n# stellarium screenshot\n%d %d\n255\n",Screen->w,Screen->h);
		fwrite(temp->pixels,1,3*(Screen->w)*(Screen->h),f);
		fclose(f);
	} else {
		cerr << "StelAppSdl::saveScreenShot: fopen(" 
			<< shotPath.string() 
			<< ") failed"
			<< endl;
	}
#else
	SDL_SaveBMP(temp, shotPath.string().c_str());
#endif

	SDL_FreeSurface(temp);
	cout << "Saved screenshot to file : " << shotPath.string() << endl;
}

/*************************************************************************
 Alternate fullscreen mode/windowed mode if possible
*************************************************************************/
void StelAppSdl::toggleFullScreen()
{
	SDL_WM_ToggleFullScreen(Screen);
}

bool StelAppSdl::getFullScreen() const
{
	cerr << "Warning StelAppSdl::getFullScreen() can't be implemented." << endl;
	return true;
}

#endif // USE_SDL
