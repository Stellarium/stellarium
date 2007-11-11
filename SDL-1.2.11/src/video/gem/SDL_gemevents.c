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

/*
 * GEM SDL video driver implementation
 * inspired from the Dummy SDL driver
 * 
 * Patrice Mandin
 * and work from
 * Olivier Landemarre, Johan Klockars, Xavier Joubert, Claude Attard
 */

#include <gem.h>

#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"
#include "SDL_gemvideo.h"
#include "SDL_gemevents_c.h"
#include "../ataricommon/SDL_atarikeys.h"	/* for keyboard scancodes */
#include "../ataricommon/SDL_atarievents_c.h"
#include "../ataricommon/SDL_xbiosevents_c.h"
#include "../ataricommon/SDL_ataridevmouse_c.h"

/* Defines */

#define ATARIBIOS_MAXKEYS 128

/* Variables */

static unsigned char gem_currentkeyboard[ATARIBIOS_MAXKEYS];
static unsigned char gem_previouskeyboard[ATARIBIOS_MAXKEYS];
static unsigned char gem_currentascii[ATARIBIOS_MAXKEYS];

/* The translation tables from a console scancode to a SDL keysym */
static SDLKey keymap[ATARIBIOS_MAXKEYS];

/* Functions prototypes */

static SDL_keysym *TranslateKey(int scancode, int asciicode, SDL_keysym *keysym,
	SDL_bool pressed);
static int do_messages(_THIS, short *message);
static void do_keyboard(short kc, short ks);
static void do_mouse(_THIS, short mx, short my, short mb, short ks);

/* Functions */

static SDL_keysym *TranslateKey(int scancode, int asciicode, SDL_keysym *keysym,
	SDL_bool pressed)
{
	/* Set the keysym information */
	keysym->scancode = scancode;

	if (asciicode)
		keysym->sym = asciicode;		
	else
		keysym->sym = keymap[scancode];

	keysym->mod = KMOD_NONE;
	keysym->unicode = 0;
	if (SDL_TranslateUNICODE && pressed) {
		keysym->unicode = SDL_AtariToUnicodeTable[asciicode];
	}

	return(keysym);
}

void GEM_InitOSKeymap(_THIS)
{
	int i;

	SDL_memset(gem_currentkeyboard, 0, sizeof(gem_currentkeyboard));
	SDL_memset(gem_previouskeyboard, 0, sizeof(gem_previouskeyboard));
	SDL_memset(gem_currentascii, 0, sizeof(gem_currentascii));

	/* Initialize keymap */
	for ( i=0; i<sizeof(keymap); i++ )
		keymap[i] = SDLK_UNKNOWN;

	/* Functions keys */
	for ( i = 0; i<10; i++ )
		keymap[SCANCODE_F1 + i] = SDLK_F1+i;

	/* Cursor keypad */
	keymap[SCANCODE_HELP] = SDLK_HELP;
	keymap[SCANCODE_UNDO] = SDLK_UNDO;
	keymap[SCANCODE_INSERT] = SDLK_INSERT;
	keymap[SCANCODE_CLRHOME] = SDLK_HOME;
	keymap[SCANCODE_UP] = SDLK_UP;
	keymap[SCANCODE_DOWN] = SDLK_DOWN;
	keymap[SCANCODE_RIGHT] = SDLK_RIGHT;
	keymap[SCANCODE_LEFT] = SDLK_LEFT;

	/* Special keys */
	keymap[SCANCODE_ESCAPE] = SDLK_ESCAPE;
	keymap[SCANCODE_BACKSPACE] = SDLK_BACKSPACE;
	keymap[SCANCODE_TAB] = SDLK_TAB;
	keymap[SCANCODE_ENTER] = SDLK_RETURN;
	keymap[SCANCODE_DELETE] = SDLK_DELETE;
	keymap[SCANCODE_LEFTCONTROL] = SDLK_LCTRL;
	keymap[SCANCODE_LEFTSHIFT] = SDLK_LSHIFT;
	keymap[SCANCODE_RIGHTSHIFT] = SDLK_RSHIFT;
	keymap[SCANCODE_LEFTALT] = SDLK_LALT;
	keymap[SCANCODE_CAPSLOCK] = SDLK_CAPSLOCK;

	/* Mouse init */
	GEM_mouse_relative = SDL_FALSE;
}

void GEM_PumpEvents(_THIS)
{
	short mousex, mousey, mouseb, dummy;
	short kstate, prevkc, prevks;
	int i;
	SDL_keysym	keysym;

	SDL_memset(gem_currentkeyboard,0,sizeof(gem_currentkeyboard));
	prevkc = prevks = 0;
	
	for (;;)
	{
		int quit, resultat, event_mask, mouse_event;
		short buffer[8], kc;
		short x2,y2,w2,h2;

		quit =
			mouse_event =
			x2=y2=w2=h2 = 0;

		event_mask = MU_MESAG|MU_TIMER|MU_KEYBD;
		if (!GEM_fullscreen && (GEM_handle>=0)) {
			wind_get (GEM_handle, WF_WORKXYWH, &x2, &y2, &w2, &h2);
			event_mask |= MU_M1;
			if ( (SDL_GetAppState() & SDL_APPMOUSEFOCUS) ) {
				mouse_event = MO_LEAVE;				
			} else {
				mouse_event = MO_ENTER;				
			}
		}

		resultat = evnt_multi(
			event_mask,
			0,0,0,
			mouse_event,x2,y2,w2,h2,
			0,0,0,0,0,
			buffer,
			10,
			&dummy,&dummy,&dummy,&kstate,&kc,&dummy
		);

		/* Message event ? */
		if (resultat & MU_MESAG)
			quit = do_messages(this, buffer);

		/* Keyboard event ? */
		if (resultat & MU_KEYBD) {
			if ((prevkc != kc) || (prevks != kstate)) {
				do_keyboard(kc,kstate);
			} else {
				/* Avoid looping, if repeating same key */
				break;
			}
		}

		/* Mouse entering/leaving window */
		if (resultat & MU_M1) {
			if (this->input_grab == SDL_GRAB_OFF) {
				if (SDL_GetAppState() & SDL_APPMOUSEFOCUS) {
					SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
					if (SDL_GetAppState() & SDL_APPINPUTFOCUS) {
						graf_mouse(ARROW, NULL);
					}
				} else {
					SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
					if (SDL_GetAppState() & SDL_APPINPUTFOCUS) {
						if (GEM_cursor == (void *) -1) {
							graf_mouse(M_OFF, NULL);
						} else if (GEM_cursor) {
							graf_mouse(USER_DEF, GEM_cursor->mform_p);
						}
					}
				}
			}
		}

		/* Timer event ? */
		if ((resultat & MU_TIMER) || quit)
			break;
	}

	/* Update mouse */
	graf_mkstate(&mousex, &mousey, &mouseb, &kstate);
	do_mouse(this, mousex, mousey, mouseb, kstate);

	/* Now generate keyboard events */
	for (i=0; i<ATARIBIOS_MAXKEYS; i++) {
		/* Key pressed ? */
		if (gem_currentkeyboard[i] && !gem_previouskeyboard[i])
			SDL_PrivateKeyboard(SDL_PRESSED,
				TranslateKey(i, gem_currentascii[i], &keysym, SDL_TRUE));
			
		/* Key unpressed ? */
		if (gem_previouskeyboard[i] && !gem_currentkeyboard[i])
			SDL_PrivateKeyboard(SDL_RELEASED,
				TranslateKey(i, gem_currentascii[i], &keysym, SDL_FALSE));
	}

	SDL_memcpy(gem_previouskeyboard,gem_currentkeyboard,sizeof(gem_previouskeyboard));

	/* Refresh window name ? */
	if (GEM_refresh_name) {
		if ( SDL_GetAppState() & SDL_APPACTIVE ) {
			/* Fullscreen/windowed */
			if (GEM_title_name) {
				wind_set(GEM_handle,WF_NAME,(short)(((unsigned long)GEM_title_name)>>16),(short)(((unsigned long)GEM_title_name) & 0xffff),0,0);
			}
		} else {
			/* Iconified */
			if (GEM_icon_name) {
				wind_set(GEM_handle,WF_NAME,(short)(((unsigned long)GEM_icon_name)>>16),(short)(((unsigned long)GEM_icon_name) & 0xffff),0,0);
			}
		}
		GEM_refresh_name = SDL_FALSE;
	}
}

static int do_messages(_THIS, short *message)
{
	int quit, posted;
	short x2,y2,w2,h2;

	quit=0;
	switch (message[0]) {
		case WM_CLOSED:
		case AP_TERM:    
			posted = SDL_PrivateQuit();
			quit=1;
			break;
		case WM_MOVED:
			wind_set(message[3],WF_CURRXYWH,message[4],message[5],message[6],message[7]);
			break;
		case WM_TOPPED:
			wind_set(message[3],WF_TOP,message[4],0,0,0);
			/* Continue with TOP event processing */
		case WM_ONTOP:
			SDL_PrivateAppActive(1, SDL_APPINPUTFOCUS);
			if (VDI_setpalette) {
				VDI_setpalette(this, VDI_curpalette);
			}
			break;
		case WM_REDRAW:
			if (!GEM_lock_redraw) {
				GEM_wind_redraw(this, message[3],&message[4]);
			}
			break;
		case WM_ICONIFY:
		case WM_ALLICONIFY:
			wind_set(message[3],WF_ICONIFY,message[4],message[5],message[6],message[7]);
			/* If we're active, make ourselves inactive */
			if ( SDL_GetAppState() & SDL_APPACTIVE ) {
				/* Send an internal deactivate event */
				SDL_PrivateAppActive(0, SDL_APPACTIVE);
			}
			/* Update window title */
			if (GEM_refresh_name && GEM_icon_name) {
				wind_set(GEM_handle,WF_NAME,(short)(((unsigned long)GEM_icon_name)>>16),(short)(((unsigned long)GEM_icon_name) & 0xffff),0,0);
				GEM_refresh_name = SDL_FALSE;
			}
			break;
		case WM_UNICONIFY:
			wind_set(message[3],WF_UNICONIFY,message[4],message[5],message[6],message[7]);
			/* If we're not active, make ourselves active */
			if ( !(SDL_GetAppState() & SDL_APPACTIVE) ) {
				/* Send an internal activate event */
				SDL_PrivateAppActive(1, SDL_APPACTIVE);
			}
			if (GEM_refresh_name && GEM_title_name) {
				wind_set(GEM_handle,WF_NAME,(short)(((unsigned long)GEM_title_name)>>16),(short)(((unsigned long)GEM_title_name) & 0xffff),0,0);
				GEM_refresh_name = SDL_FALSE;
			}
			break;
		case WM_SIZED:
			wind_set (message[3], WF_CURRXYWH, message[4], message[5], message[6], message[7]);
			wind_get (message[3], WF_WORKXYWH, &x2, &y2, &w2, &h2);
			GEM_win_fulled = SDL_FALSE;		/* Cancel maximized flag */
			GEM_lock_redraw = SDL_TRUE;		/* Prevent redraw till buffers resized */
			SDL_PrivateResize(w2, h2);
			break;
		case WM_FULLED:
			{
				short x,y,w,h;

				if (GEM_win_fulled) {
					wind_get (message[3], WF_PREVXYWH, &x, &y, &w, &h);
					GEM_win_fulled = SDL_FALSE;
				} else {
					x = GEM_desk_x;
					y = GEM_desk_y;
					w = GEM_desk_w;
					h = GEM_desk_h;
					GEM_win_fulled = SDL_TRUE;
				}
				wind_set (message[3], WF_CURRXYWH, x, y, w, h);
				wind_get (message[3], WF_WORKXYWH, &x2, &y2, &w2, &h2);
				GEM_lock_redraw = SDL_TRUE;		/* Prevent redraw till buffers resized */
				SDL_PrivateResize(w2, h2);
			}
			break;
		case WM_BOTTOMED:
			wind_set(message[3],WF_BOTTOM,0,0,0,0);
			/* Continue with BOTTOM event processing */
		case WM_UNTOPPED:
			SDL_PrivateAppActive(0, SDL_APPINPUTFOCUS);
			if (VDI_setpalette) {
				VDI_setpalette(this, VDI_oldpalette);
			}
			break;
	}
	
	return quit;
}

static void do_keyboard(short kc, short ks)
{
	int			scancode, asciicode;

	if (kc) {
		scancode=(kc>>8) & 127;
		asciicode=kc & 255;

		gem_currentkeyboard[scancode]=0xFF;
		gem_currentascii[scancode]=asciicode;
	}

	/* Read special keys */
	if (ks & K_RSHIFT)
		gem_currentkeyboard[SCANCODE_RIGHTSHIFT]=0xFF;
	if (ks & K_LSHIFT)
		gem_currentkeyboard[SCANCODE_LEFTSHIFT]=0xFF;
	if (ks & K_CTRL)
		gem_currentkeyboard[SCANCODE_LEFTCONTROL]=0xFF;
	if (ks & K_ALT)
		gem_currentkeyboard[SCANCODE_LEFTALT]=0xFF;
}

static void do_mouse(_THIS, short mx, short my, short mb, short ks)
{
	static short prevmousex=0, prevmousey=0, prevmouseb=0;
	short x2, y2, w2, h2;

	/* Don't return mouse events if out of window */
	if ((SDL_GetAppState() & SDL_APPMOUSEFOCUS)==0) {
		return;
	}

	/* Retrieve window coords, and generate mouse events accordingly */
	x2 = y2 = 0;
	w2 = VDI_w;
	h2 = VDI_h;
	if ((!GEM_fullscreen) && (GEM_handle>=0)) {
		wind_get (GEM_handle, WF_WORKXYWH, &x2, &y2, &w2, &h2);

		/* Do not generate mouse button event if out of window working area */
		if ((mx<x2) || (mx>=x2+w2) || (my<y2) || (my>=y2+h2)) {
			mb=prevmouseb;
		}
	}

	/* Mouse motion ? */
	if (GEM_mouse_relative) {
		if (GEM_usedevmouse) {
			SDL_AtariDevMouse_PostMouseEvents(this, SDL_FALSE);
		} else {
			SDL_AtariXbios_PostMouseEvents(this, SDL_FALSE);
		}
	} else {
		if ((prevmousex!=mx) || (prevmousey!=my)) {
			int posx, posy;

			/* Give mouse position relative to window position */
			posx = mx - x2;
			if (posx<0) posx = 0;
			if (posx>w2) posx = w2-1;
			posy = my - y2;
			if (posy<0) posy = 0;
			if (posy>h2) posy = h2-1;

			SDL_PrivateMouseMotion(0, 0, posx, posy);
		}
		prevmousex = mx;
		prevmousey = my;
	}

	/* Mouse button ? */
	if (prevmouseb!=mb) {
		int i;

		for (i=0;i<2;i++) {
			int curbutton, prevbutton;
		
			curbutton = mb & (1<<i);
			prevbutton = prevmouseb & (1<<i);
		
			if (curbutton && !prevbutton) {
				SDL_PrivateMouseButton(SDL_PRESSED, i+1, 0, 0);
			}
			if (!curbutton && prevbutton) {
				SDL_PrivateMouseButton(SDL_RELEASED, i+1, 0, 0);
			}
		}
		prevmouseb = mb;
	}

	/* Read special keys */
	if (ks & K_RSHIFT)
		gem_currentkeyboard[SCANCODE_RIGHTSHIFT]=0xFF;
	if (ks & K_LSHIFT)
		gem_currentkeyboard[SCANCODE_LEFTSHIFT]=0xFF;
	if (ks & K_CTRL)
		gem_currentkeyboard[SCANCODE_LEFTCONTROL]=0xFF;
	if (ks & K_ALT)
		gem_currentkeyboard[SCANCODE_LEFTALT]=0xFF;
}
