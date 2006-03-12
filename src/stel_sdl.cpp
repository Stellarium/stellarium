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

#include "stelapp.h"
#include "s_gui.h"

#ifdef HAVE_SDL_MIXER_H
#include "SDL_mixer.h"
#endif

using namespace s_gui;


void StelApp::initSDL(int w, int h, int bbpMode, bool fullScreen, string iconFile)
{
    Uint32	Vflags;		// Our Video Flags
    Screen = NULL;

#ifdef HAVE_SDL_MIXER_H

    // Init the SDL library, the VIDEO subsystem    
    // Tony - added timer
    if(SDL_Init(SDL_INIT_VIDEO |  SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER)<0)
	{
		// couldn't init audio, so try without
		fprintf(stderr, "Error: unable to open SDL with audio: %s\n", SDL_GetError() );

		if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE)<0)
		{
			fprintf(stderr, "Error: unable to open SDL: %s\n", SDL_GetError() );
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
		fprintf(stderr, "Unable to open SDL: %s\n", SDL_GetError() );
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


// from an sdl wiki
SDL_Cursor* StelApp::create_cursor(const char *image[])
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
void StelApp::terminateApplication(void)
{
	static SDL_Event Q;						// Send a SDL_QUIT event
	Q.type = SDL_QUIT;						// To the SDL event queue
	if(SDL_PushEvent(&Q) == -1)				// Try to send the event
	{
		printf("SDL_QUIT event can't be pushed: %s\n", SDL_GetError() );
		exit(-1);
	}
}

void StelApp::start_main_loop()
{
    bool AppVisible = true;			// At The Beginning, Our App Is Visible
    enum S_GUI_VALUE bt;

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
					if (E.resize.h && E.resize.w) core->setViewportSize(E.resize.w, E.resize.h);
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
				  	handleMove(E.motion.x,E.motion.y);
					break;
				
				case SDL_MOUSEBUTTONDOWN:
					// Convert the name from GLU to my GUI
					switch (E.button.button)
					{
						case SDL_BUTTON_RIGHT : bt=S_GUI_MOUSE_RIGHT; break;
						case SDL_BUTTON_LEFT :  bt=S_GUI_MOUSE_LEFT; break;
						case SDL_BUTTON_MIDDLE : bt=S_GUI_MOUSE_MIDDLE; break;
						case SDL_BUTTON_WHEELUP : bt=S_GUI_MOUSE_WHEELUP; break;
						case SDL_BUTTON_WHEELDOWN : bt=S_GUI_MOUSE_WHEELDOWN; break;
						default : bt=S_GUI_MOUSE_LEFT;
					}
					handleClick(E.button.x,E.button.y,bt,S_GUI_PRESSED);
					break;

				case SDL_MOUSEBUTTONUP:
					// Convert the name from GLU to my GUI
					switch (E.button.button)
					{
						case SDL_BUTTON_RIGHT : bt=S_GUI_MOUSE_RIGHT; break;
						case SDL_BUTTON_LEFT :  bt=S_GUI_MOUSE_LEFT; break;
						case SDL_BUTTON_MIDDLE : bt=S_GUI_MOUSE_MIDDLE; break;
						case SDL_BUTTON_WHEELUP : bt=S_GUI_MOUSE_WHEELUP; break;
						case SDL_BUTTON_WHEELDOWN : bt=S_GUI_MOUSE_WHEELDOWN; break;
						default : bt=S_GUI_MOUSE_LEFT;
					}
					handleClick(E.button.x,E.button.y,bt,S_GUI_RELEASED);
					break;

				case SDL_KEYDOWN:
					// Send the event to the gui and stop if it has been intercepted

					/*
					printf("key pressed: ");
					if( E.key.keysym.unicode < 0x80 && E.key.keysym.unicode > 0 ){
						printf( "%c (0x%04X)\n", (char)E.key.keysym.unicode,
								E.key.keysym.unicode );
					}
					else{
						printf( "? (0x%04X)\n", E.key.keysym.unicode );
					}
					*/

					// use unicode translation, since not keyboard dependent
					// however, for non-printing keys must revert to just keysym... !
					if ((E.key.keysym.unicode && !handleKeys(E.key.keysym.unicode,S_GUI_PRESSED)) ||
						(!E.key.keysym.unicode && !handleKeys(E.key.keysym.sym,S_GUI_PRESSED)))
					{

						/* Fumio patch... can't use because ignores unicode values and hence is US keyboard specific.
						   - what was the reasoning?
					if ((E.key.keysym.sym >= SDLK_LAST && !core->handle_keys(E.key.keysym.unicode,S_GUI_PRESSED)) ||
						(E.key.keysym.sym < SDLK_LAST && !core->handle_keys(E.key.keysym.sym,S_GUI_PRESSED)))
						*/

						if (E.key.keysym.sym==SDLK_F1) SDL_WM_ToggleFullScreen(Screen); // Try fullscreen

						// ctrl-s saves screenshot
						if (E.key.keysym.unicode==0x0013 &&  (Screen->flags & SDL_OPENGL))
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
					if (E.key.keysym.sym==SDLK_ESCAPE && (SDL_GetModState() & KMOD_CTRL)) terminateApplication();
					break;

				case SDL_KEYUP:
					handleKeys(E.key.keysym.sym,S_GUI_RELEASED);
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
				if (TickCount-LastCount < 1000.f/getMaxFPS())
				{
					SDL_Delay((unsigned int)(1000.f/getMaxFPS())-(TickCount-LastCount));
				}
				TickCount = SDL_GetTicks();			// Get present ticks
				this->update(TickCount-LastCount);	// And update the motions and data
				this->draw(TickCount-LastCount);	// Do the drawings!
				LastCount = TickCount;				// Save the present tick probing
				SDL_GL_SwapBuffers();				// And swap the buffers
			}
		}
	}
}
