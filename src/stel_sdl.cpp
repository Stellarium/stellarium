/*
 * Copyright (C) 2003 Fabien Ch�eau
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

#ifdef HAVE_SDL_MIXER_H
#include "SDL_mixer.h"
#endif

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
  SDL_FreeCursor(Cursor);
}

void stel_sdl::init(void)
{
    Uint32	Vflags;		// Our Video Flags
    Screen = NULL;

#ifdef HAVE_SDL_MIXER_H

    // Init the SDL library, the VIDEO subsystem    
    if(SDL_Init(SDL_INIT_VIDEO |  SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE)<0)
	{
		// couldn't init audio, so try without
		printf("Unable to open SDL with audio: %s\n", SDL_GetError() );

		if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE)<0)
		{
			printf("Unable to open SDL: %s\n", SDL_GetError() );
			exit(-1);
		}
    } 
	else
	{
		/*
		// initialized with audio enabled
		// TODO: only initi audio if config option allows and script needs
		if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers))
		{
			printf("Unable to open audio!\n");
		}

		*/
	}
#else
	// SDL_mixer is not available - no audio
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE)<0)
	{
		printf("Unable to open SDL: %s\n", SDL_GetError() );
		exit(-1);
	}
#endif

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
		printf("sdl: Couldn't set %dx%d video mode (%s), retrying with stencil size 0\n",
 		core->get_screen_W(), core->get_screen_H(), SDL_GetError());

		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
		Screen = SDL_SetVideoMode(core->get_screen_W(), core->get_screen_H(), core->get_bppMode(), Vflags);

		if(!Screen)
		{
			printf("sdl: Couldn't set %dx%d video mode: %s!\n",
			core->get_screen_W(), core->get_screen_H(), SDL_GetError());
			exit(-1);
		}	
	}

	// Disable key repeat
    SDL_EnableKeyRepeat(0, 0);

	SDL_EnableUNICODE(1);


	// set mouse cursor 
/* XPM */
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
	SDL_Surface *icon = SDL_LoadBMP((core->get_DataDir() + "icon.bmp").c_str());
	SDL_WM_SetIcon(icon, NULL);
	SDL_FreeSurface(icon);
	
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);	
}


// from an sdl wiki
SDL_Cursor* stel_sdl::create_cursor(const char *image[])
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

    // Hold the value of SDL_GetTicks at start of main loop (set 0 time)
    LastCount = SDL_GetTicks();

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

						if (E.key.keysym.sym==SDLK_s && (SDL_GetModState() & KMOD_CTRL) &&
							(Screen->flags & SDL_OPENGL))
                        {
							string tempName;
							char c[3];
							FILE *fp;

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

							string shotdir;
#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__)
							if(getenv("USERPROFILE")!=NULL){
								//for Win XP etc.
								shotdir = string(getenv("USERPROFILE")) + "\\My Documents\\";
							}else{
								//for Win 98 etc.
								shotdir = "C:\\My Documents\\";
							}
#else
							shotdir = string(getenv("HOME")) + "/";
#endif
#ifdef MACOSX
							shotdir += "/Desktop/";
#endif
							for(int j=0; j<=100; ++j)
							{
								snprintf(c,3,"%d",j);

							tempName = shotdir + "stellarium" + c + ".bmp";
								fp = fopen(tempName.c_str(), "r");
								if(fp == NULL)
									break;
								else
									fclose(fp);
							}

							SDL_SaveBMP(temp, tempName.c_str());
							SDL_FreeSurface(temp);
							cout << "Saved screenshot to file : " << tempName << endl;
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
				// This is used to constraint the maximum FPS rate
				if (TickCount-LastCount < 1000.f/core->getMaxFPS())
				{
					SDL_Delay((unsigned int)(1000.f/core->getMaxFPS())-(TickCount-LastCount));
				}
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

