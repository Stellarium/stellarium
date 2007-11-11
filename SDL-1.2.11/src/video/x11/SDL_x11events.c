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

/* Handle the event stream, converting X11 events into SDL events */

#include <setjmp.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#ifdef __SVR4
#include <X11/Sunkeysym.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "SDL_timer.h"
#include "SDL_syswm.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"
#include "SDL_x11video.h"
#include "SDL_x11dga_c.h"
#include "SDL_x11modes_c.h"
#include "SDL_x11image_c.h"
#include "SDL_x11gamma_c.h"
#include "SDL_x11wm_c.h"
#include "SDL_x11mouse_c.h"
#include "SDL_x11events_c.h"


/* Define this if you want to debug X11 events */
/*#define DEBUG_XEVENTS*/

/* The translation tables from an X11 keysym to a SDL keysym */
static SDLKey ODD_keymap[256];
static SDLKey MISC_keymap[256];
SDLKey X11_TranslateKeycode(Display *display, KeyCode kc);


#ifdef X_HAVE_UTF8_STRING
Uint32 Utf8ToUcs4(const Uint8 *utf8)
{
	Uint32 c;
	int i = 1;
	int noOctets = 0;
	int firstOctetMask = 0;
	unsigned char firstOctet = utf8[0];
	if (firstOctet < 0x80) {
		/*
		  Characters in the range:
		    00000000 to 01111111 (ASCII Range)
		  are stored in one octet:
		    0xxxxxxx (The same as its ASCII representation)
		  The least 6 significant bits of the first octet is the most 6 significant nonzero bits
		  of the UCS4 representation.
		*/
		noOctets = 1;
		firstOctetMask = 0x7F;  /* 0(1111111) - The most significant bit is ignored */
	} else if ((firstOctet & 0xE0) /* get the most 3 significant bits by AND'ing with 11100000 */
	              == 0xC0 ) {  /* see if those 3 bits are 110. If so, the char is in this range */
		/*
		  Characters in the range:
		    00000000 10000000 to 00000111 11111111
		  are stored in two octets:
		    110xxxxx 10xxxxxx
		  The least 5 significant bits of the first octet is the most 5 significant nonzero bits
		  of the UCS4 representation.
		*/
		noOctets = 2;
		firstOctetMask = 0x1F;  /* 000(11111) - The most 3 significant bits are ignored */
	} else if ((firstOctet & 0xF0) /* get the most 4 significant bits by AND'ing with 11110000 */
	              == 0xE0) {  /* see if those 4 bits are 1110. If so, the char is in this range */
		/*
		  Characters in the range:
		    00001000 00000000 to 11111111 11111111
		  are stored in three octets:
		    1110xxxx 10xxxxxx 10xxxxxx
		  The least 4 significant bits of the first octet is the most 4 significant nonzero bits
		  of the UCS4 representation.
		*/
		noOctets = 3;
		firstOctetMask = 0x0F; /* 0000(1111) - The most 4 significant bits are ignored */
	} else if ((firstOctet & 0xF8) /* get the most 5 significant bits by AND'ing with 11111000 */
	              == 0xF0) {  /* see if those 5 bits are 11110. If so, the char is in this range */
		/*
		  Characters in the range:
		    00000001 00000000 00000000 to 00011111 11111111 11111111
		  are stored in four octets:
		    11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		  The least 3 significant bits of the first octet is the most 3 significant nonzero bits
		  of the UCS4 representation.
		*/
		noOctets = 4;
		firstOctetMask = 0x07; /* 11110(111) - The most 5 significant bits are ignored */
	} else if ((firstOctet & 0xFC) /* get the most 6 significant bits by AND'ing with 11111100 */
	              == 0xF8) { /* see if those 6 bits are 111110. If so, the char is in this range */
		/*
		  Characters in the range:
		    00000000 00100000 00000000 00000000 to
		    00000011 11111111 11111111 11111111
		  are stored in five octets:
		    111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		  The least 2 significant bits of the first octet is the most 2 significant nonzero bits
		  of the UCS4 representation.
		*/
		noOctets = 5;
		firstOctetMask = 0x03; /* 111110(11) - The most 6 significant bits are ignored */
	} else if ((firstOctet & 0xFE) /* get the most 7 significant bits by AND'ing with 11111110 */
	              == 0xFC) { /* see if those 7 bits are 1111110. If so, the char is in this range */
		/*
		  Characters in the range:
		    00000100 00000000 00000000 00000000 to
		    01111111 11111111 11111111 11111111
		  are stored in six octets:
		    1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		  The least significant bit of the first octet is the most significant nonzero bit
		  of the UCS4 representation.
		*/
		noOctets = 6;
		firstOctetMask = 0x01; /* 1111110(1) - The most 7 significant bits are ignored */
	} else
		return 0;  /* The given chunk is not a valid UTF-8 encoded Unicode character */
	
	/*
	  The least noOctets significant bits of the first octet is the most 2 significant nonzero bits
	  of the UCS4 representation.
	  The first 6 bits of the UCS4 representation is the least 8-noOctets-1 significant bits of
	  firstOctet if the character is not ASCII. If so, it's the least 7 significant bits of firstOctet.
	  This done by AND'ing firstOctet with its mask to trim the bits used for identifying the
	  number of continuing octets (if any) and leave only the free bits (the x's)
	  Sample:
	  1-octet:    0xxxxxxx  &  01111111 = 0xxxxxxx
	  2-octets:  110xxxxx  &  00011111 = 000xxxxx
	*/
	c = firstOctet & firstOctetMask;
	
	/* Now, start filling c.ucs4 with the bits from the continuing octets from utf8. */
	for (i = 1; i < noOctets; i++) {
		/* A valid continuing octet is of the form 10xxxxxx */
		if ((utf8[i] & 0xC0) /* get the most 2 significant bits by AND'ing with 11000000 */
		    != 0x80) /* see if those 2 bits are 10. If not, the is a malformed sequence. */
			/*The given chunk is a partial sequence at the end of a string that could
			   begin a valid character */
			return 0;
		
		/* Make room for the next 6-bits */
		c <<= 6;
		
		/*
		  Take only the least 6 significance bits of the current octet (utf8[i]) and fill the created room
		  of c.ucs4 with them.
		  This done by AND'ing utf8[i] with 00111111 and the OR'ing the result with c.ucs4.
		*/
		c |= utf8[i] & 0x3F;
	}
	return c;
}
#endif

/* Check to see if this is a repeated key.
   (idea shamelessly lifted from GII -- thanks guys! :)
 */
static int X11_KeyRepeat(Display *display, XEvent *event)
{
	XEvent peekevent;
	int repeated;

	repeated = 0;
	if ( XPending(display) ) {
		XPeekEvent(display, &peekevent);
		if ( (peekevent.type == KeyPress) &&
		     (peekevent.xkey.keycode == event->xkey.keycode) &&
		     ((peekevent.xkey.time-event->xkey.time) < 2) ) {
			repeated = 1;
			XNextEvent(display, &peekevent);
		}
	}
	return(repeated);
}

/* Note:  The X server buffers and accumulates mouse motion events, so
   the motion event generated by the warp may not appear exactly as we
   expect it to.  We work around this (and improve performance) by only
   warping the pointer when it reaches the edge, and then wait for it.
*/
#define MOUSE_FUDGE_FACTOR	8

static __inline__ int X11_WarpedMotion(_THIS, XEvent *xevent)
{
	int w, h, i;
	int deltax, deltay;
	int posted;

	w = SDL_VideoSurface->w;
	h = SDL_VideoSurface->h;
	deltax = xevent->xmotion.x - mouse_last.x;
	deltay = xevent->xmotion.y - mouse_last.y;
#ifdef DEBUG_MOTION
  printf("Warped mouse motion: %d,%d\n", deltax, deltay);
#endif
	mouse_last.x = xevent->xmotion.x;
	mouse_last.y = xevent->xmotion.y;
	posted = SDL_PrivateMouseMotion(0, 1, deltax, deltay);

	if ( (xevent->xmotion.x < MOUSE_FUDGE_FACTOR) ||
	     (xevent->xmotion.x > (w-MOUSE_FUDGE_FACTOR)) ||
	     (xevent->xmotion.y < MOUSE_FUDGE_FACTOR) ||
	     (xevent->xmotion.y > (h-MOUSE_FUDGE_FACTOR)) ) {
		/* Get the events that have accumulated */
		while ( XCheckTypedEvent(SDL_Display, MotionNotify, xevent) ) {
			deltax = xevent->xmotion.x - mouse_last.x;
			deltay = xevent->xmotion.y - mouse_last.y;
#ifdef DEBUG_MOTION
  printf("Extra mouse motion: %d,%d\n", deltax, deltay);
#endif
			mouse_last.x = xevent->xmotion.x;
			mouse_last.y = xevent->xmotion.y;
			posted += SDL_PrivateMouseMotion(0, 1, deltax, deltay);
		}
		mouse_last.x = w/2;
		mouse_last.y = h/2;
		XWarpPointer(SDL_Display, None, SDL_Window, 0, 0, 0, 0,
					mouse_last.x, mouse_last.y);
		for ( i=0; i<10; ++i ) {
        		XMaskEvent(SDL_Display, PointerMotionMask, xevent);
			if ( (xevent->xmotion.x >
			          (mouse_last.x-MOUSE_FUDGE_FACTOR)) &&
			     (xevent->xmotion.x <
			          (mouse_last.x+MOUSE_FUDGE_FACTOR)) &&
			     (xevent->xmotion.y >
			          (mouse_last.y-MOUSE_FUDGE_FACTOR)) &&
			     (xevent->xmotion.y <
			          (mouse_last.y+MOUSE_FUDGE_FACTOR)) ) {
				break;
			}
#ifdef DEBUG_XEVENTS
  printf("Lost mouse motion: %d,%d\n", xevent->xmotion.x, xevent->xmotion.y);
#endif
		}
#ifdef DEBUG_XEVENTS
		if ( i == 10 ) {
			printf("Warning: didn't detect mouse warp motion\n");
		}
#endif
	}
	return(posted);
}

static int X11_DispatchEvent(_THIS)
{
	int posted;
	XEvent xevent;

	SDL_memset(&xevent, '\0', sizeof (XEvent));  /* valgrind fix. --ryan. */
	XNextEvent(SDL_Display, &xevent);

	posted = 0;
	switch (xevent.type) {

	    /* Gaining mouse coverage? */
	    case EnterNotify: {
#ifdef DEBUG_XEVENTS
printf("EnterNotify! (%d,%d)\n", xevent.xcrossing.x, xevent.xcrossing.y);
if ( xevent.xcrossing.mode == NotifyGrab )
printf("Mode: NotifyGrab\n");
if ( xevent.xcrossing.mode == NotifyUngrab )
printf("Mode: NotifyUngrab\n");
#endif
		if ( (xevent.xcrossing.mode != NotifyGrab) &&
		     (xevent.xcrossing.mode != NotifyUngrab) ) {
			if ( this->input_grab == SDL_GRAB_OFF ) {
				posted = SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
			}
			posted = SDL_PrivateMouseMotion(0, 0,
					xevent.xcrossing.x,
					xevent.xcrossing.y);
		}
	    }
	    break;

	    /* Losing mouse coverage? */
	    case LeaveNotify: {
#ifdef DEBUG_XEVENTS
printf("LeaveNotify! (%d,%d)\n", xevent.xcrossing.x, xevent.xcrossing.y);
if ( xevent.xcrossing.mode == NotifyGrab )
printf("Mode: NotifyGrab\n");
if ( xevent.xcrossing.mode == NotifyUngrab )
printf("Mode: NotifyUngrab\n");
#endif
		if ( (xevent.xcrossing.mode != NotifyGrab) &&
		     (xevent.xcrossing.mode != NotifyUngrab) &&
		     (xevent.xcrossing.detail != NotifyInferior) ) {
			if ( this->input_grab == SDL_GRAB_OFF ) {
				posted = SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
			} else {
				posted = SDL_PrivateMouseMotion(0, 0,
						xevent.xcrossing.x,
						xevent.xcrossing.y);
			}
		}
	    }
	    break;

	    /* Gaining input focus? */
	    case FocusIn: {
#ifdef DEBUG_XEVENTS
printf("FocusIn!\n");
#endif
		posted = SDL_PrivateAppActive(1, SDL_APPINPUTFOCUS);

#ifdef X_HAVE_UTF8_STRING
		if ( SDL_IC != NULL ) {
			XSetICFocus(SDL_IC);
		}
#endif
		/* Queue entry into fullscreen mode */
		switch_waiting = 0x01 | SDL_FULLSCREEN;
		switch_time = SDL_GetTicks() + 1500;
	    }
	    break;

	    /* Losing input focus? */
	    case FocusOut: {
#ifdef DEBUG_XEVENTS
printf("FocusOut!\n");
#endif
		posted = SDL_PrivateAppActive(0, SDL_APPINPUTFOCUS);

#ifdef X_HAVE_UTF8_STRING
		if ( SDL_IC != NULL ) {
			XUnsetICFocus(SDL_IC);
		}
#endif
		/* Queue leaving fullscreen mode */
		switch_waiting = 0x01;
		switch_time = SDL_GetTicks() + 200;
	    }
	    break;

	    /* Generated upon EnterWindow and FocusIn */
	    case KeymapNotify: {
#ifdef DEBUG_XEVENTS
printf("KeymapNotify!\n");
#endif
		X11_SetKeyboardState(SDL_Display,  xevent.xkeymap.key_vector);
	    }
	    break;

	    /* Mouse motion? */
	    case MotionNotify: {
		if ( SDL_VideoSurface ) {
			if ( mouse_relative ) {
				if ( using_dga & DGA_MOUSE ) {
#ifdef DEBUG_MOTION
  printf("DGA motion: %d,%d\n", xevent.xmotion.x_root, xevent.xmotion.y_root);
#endif
					posted = SDL_PrivateMouseMotion(0, 1,
							xevent.xmotion.x_root,
							xevent.xmotion.y_root);
				} else {
					posted = X11_WarpedMotion(this,&xevent);
				}
			} else {
#ifdef DEBUG_MOTION
  printf("X11 motion: %d,%d\n", xevent.xmotion.x, xevent.xmotion.y);
#endif
				posted = SDL_PrivateMouseMotion(0, 0,
						xevent.xmotion.x,
						xevent.xmotion.y);
			}
		}
	    }
	    break;

	    /* Mouse button press? */
	    case ButtonPress: {
		posted = SDL_PrivateMouseButton(SDL_PRESSED, 
					xevent.xbutton.button, 0, 0);
	    }
	    break;

	    /* Mouse button release? */
	    case ButtonRelease: {
		posted = SDL_PrivateMouseButton(SDL_RELEASED, 
					xevent.xbutton.button, 0, 0);
	    }
	    break;

	    /* Key press? */
	    case KeyPress: {
		static SDL_keysym saved_keysym;
		SDL_keysym keysym;
		KeyCode keycode = xevent.xkey.keycode;

#ifdef DEBUG_XEVENTS
printf("KeyPress (X11 keycode = 0x%X)\n", xevent.xkey.keycode);
#endif
		/* Get the translated SDL virtual keysym */
		if ( keycode ) {
			keysym.scancode = keycode;
			keysym.sym = X11_TranslateKeycode(SDL_Display, keycode);
			keysym.mod = KMOD_NONE;
			keysym.unicode = 0;
		} else {
			keysym = saved_keysym;
		}

		/* If we're not doing translation, we're done! */
		if ( !SDL_TranslateUNICODE ) {
			posted = SDL_PrivateKeyboard(SDL_PRESSED, &keysym);
			break;
		}

		if ( XFilterEvent(&xevent, None) ) {
			if ( xevent.xkey.keycode ) {
				posted = SDL_PrivateKeyboard(SDL_PRESSED, &keysym);
			} else {
				/* Save event to be associated with IM text
				   In 1.3 we'll have a text event instead.. */
				saved_keysym = keysym;
			}
			break;
		}

		/* Look up the translated value for the key event */
#ifdef X_HAVE_UTF8_STRING
		if ( SDL_IC != NULL ) {
			static Status state;
			/* A UTF-8 character can be at most 6 bytes */
			char keybuf[6];
			if ( Xutf8LookupString(SDL_IC, &xevent.xkey,
			                        keybuf, sizeof(keybuf),
			                        NULL, &state) ) {
				keysym.unicode = Utf8ToUcs4((Uint8*)keybuf);
			}
		}
		else
#endif
		{
			static XComposeStatus state;
			char keybuf[32];

			if ( XLookupString(&xevent.xkey,
			                    keybuf, sizeof(keybuf),
			                    NULL, &state) ) {
				/*
				* FIXME: XLookupString() may yield more than one
				* character, so we need a mechanism to allow for
				* this (perhaps null keypress events with a
				* unicode value)
				*/
				keysym.unicode = (Uint8)keybuf[0];
			}
		}
		posted = SDL_PrivateKeyboard(SDL_PRESSED, &keysym);
	    }
	    break;

	    /* Key release? */
	    case KeyRelease: {
		SDL_keysym keysym;
		KeyCode keycode = xevent.xkey.keycode;

#ifdef DEBUG_XEVENTS
printf("KeyRelease (X11 keycode = 0x%X)\n", xevent.xkey.keycode);
#endif
		/* Check to see if this is a repeated key */
		if ( X11_KeyRepeat(SDL_Display, &xevent) ) {
			break;
		}

		/* Get the translated SDL virtual keysym */
		keysym.scancode = keycode;
		keysym.sym = X11_TranslateKeycode(SDL_Display, keycode);
		keysym.mod = KMOD_NONE;
		keysym.unicode = 0;

		posted = SDL_PrivateKeyboard(SDL_RELEASED, &keysym);
	    }
	    break;

	    /* Have we been iconified? */
	    case UnmapNotify: {
#ifdef DEBUG_XEVENTS
printf("UnmapNotify!\n");
#endif
		/* If we're active, make ourselves inactive */
		if ( SDL_GetAppState() & SDL_APPACTIVE ) {
			/* Swap out the gamma before we go inactive */
			X11_SwapVidModeGamma(this);

			/* Send an internal deactivate event */
			posted = SDL_PrivateAppActive(0,
					SDL_APPACTIVE|SDL_APPINPUTFOCUS);
		}
	    }
	    break;

	    /* Have we been restored? */
	    case MapNotify: {
#ifdef DEBUG_XEVENTS
printf("MapNotify!\n");
#endif
		/* If we're not active, make ourselves active */
		if ( !(SDL_GetAppState() & SDL_APPACTIVE) ) {
			/* Send an internal activate event */
			posted = SDL_PrivateAppActive(1, SDL_APPACTIVE);

			/* Now that we're active, swap the gamma back */
			X11_SwapVidModeGamma(this);
		}

		if ( SDL_VideoSurface &&
		     (SDL_VideoSurface->flags & SDL_FULLSCREEN) ) {
			X11_EnterFullScreen(this);
		} else {
			X11_GrabInputNoLock(this, this->input_grab);
		}
		X11_CheckMouseModeNoLock(this);

		if ( SDL_VideoSurface ) {
			X11_RefreshDisplay(this);
		}
	    }
	    break;

	    /* Have we been resized or moved? */
	    case ConfigureNotify: {
#ifdef DEBUG_XEVENTS
printf("ConfigureNotify! (resize: %dx%d)\n", xevent.xconfigure.width, xevent.xconfigure.height);
#endif
		if ( SDL_VideoSurface ) {
		    if ((xevent.xconfigure.width != SDL_VideoSurface->w) ||
		        (xevent.xconfigure.height != SDL_VideoSurface->h)) {
			/* FIXME: Find a better fix for the bug with KDE 1.2 */
			if ( ! ((xevent.xconfigure.width == 32) &&
			        (xevent.xconfigure.height == 32)) ) {
				SDL_PrivateResize(xevent.xconfigure.width,
				                  xevent.xconfigure.height);
			}
		    } else {
			/* OpenGL windows need to know about the change */
			if ( SDL_VideoSurface->flags & SDL_OPENGL ) {
				SDL_PrivateExpose();
			}
		    }
		}
	    }
	    break;

	    /* Have we been requested to quit (or another client message?) */
	    case ClientMessage: {
		if ( (xevent.xclient.format == 32) &&
		     (xevent.xclient.data.l[0] == WM_DELETE_WINDOW) )
		{
			posted = SDL_PrivateQuit();
		} else
		if ( SDL_ProcessEvents[SDL_SYSWMEVENT] == SDL_ENABLE ) {
			SDL_SysWMmsg wmmsg;

			SDL_VERSION(&wmmsg.version);
			wmmsg.subsystem = SDL_SYSWM_X11;
			wmmsg.event.xevent = xevent;
			posted = SDL_PrivateSysWMEvent(&wmmsg);
		}
	    }
	    break;

	    /* Do we need to refresh ourselves? */
	    case Expose: {
#ifdef DEBUG_XEVENTS
printf("Expose (count = %d)\n", xevent.xexpose.count);
#endif
		if ( SDL_VideoSurface && (xevent.xexpose.count == 0) ) {
			X11_RefreshDisplay(this);
		}
	    }
	    break;

	    default: {
#ifdef DEBUG_XEVENTS
printf("Unhandled event %d\n", xevent.type);
#endif
		/* Only post the event if we're watching for it */
		if ( SDL_ProcessEvents[SDL_SYSWMEVENT] == SDL_ENABLE ) {
			SDL_SysWMmsg wmmsg;

			SDL_VERSION(&wmmsg.version);
			wmmsg.subsystem = SDL_SYSWM_X11;
			wmmsg.event.xevent = xevent;
			posted = SDL_PrivateSysWMEvent(&wmmsg);
		}
	    }
	    break;
	}
	return(posted);
}

/* Ack!  XPending() actually performs a blocking read if no events available */
int X11_Pending(Display *display)
{
	/* Flush the display connection and look to see if events are queued */
	XFlush(display);
	if ( XEventsQueued(display, QueuedAlready) ) {
		return(1);
	}

	/* More drastic measures are required -- see if X is ready to talk */
	{
		static struct timeval zero_time;	/* static == 0 */
		int x11_fd;
		fd_set fdset;

		x11_fd = ConnectionNumber(display);
		FD_ZERO(&fdset);
		FD_SET(x11_fd, &fdset);
		if ( select(x11_fd+1, &fdset, NULL, NULL, &zero_time) == 1 ) {
			return(XPending(display));
		}
	}

	/* Oh well, nothing is ready .. */
	return(0);
}

void X11_PumpEvents(_THIS)
{
	int pending;

	/* Keep processing pending events */
	pending = 0;
	while ( X11_Pending(SDL_Display) ) {
		X11_DispatchEvent(this);
		++pending;
	}
	if ( switch_waiting ) {
		Uint32 now;

		now  = SDL_GetTicks();
		if ( pending || !SDL_VideoSurface ) {
			/* Try again later... */
			if ( switch_waiting & SDL_FULLSCREEN ) {
				switch_time = now + 1500;
			} else {
				switch_time = now + 200;
			}
		} else if ( (int)(switch_time-now) <= 0 ) {
			Uint32 go_fullscreen;

			go_fullscreen = switch_waiting & SDL_FULLSCREEN;
			switch_waiting = 0;
			if ( SDL_VideoSurface->flags & SDL_FULLSCREEN ) {
				if ( go_fullscreen ) {
					X11_EnterFullScreen(this);
				} else {
					X11_LeaveFullScreen(this);
				}
			}
			/* Handle focus in/out when grabbed */
			if ( go_fullscreen ) {
				X11_GrabInputNoLock(this, this->input_grab);
			} else {
				X11_GrabInputNoLock(this, SDL_GRAB_OFF);
			}
			X11_CheckMouseModeNoLock(this);
		}
	}
}

void X11_InitKeymap(void)
{
	int i;

	/* Odd keys used in international keyboards */
	for ( i=0; i<SDL_arraysize(ODD_keymap); ++i )
		ODD_keymap[i] = SDLK_UNKNOWN;

 	/* Some of these might be mappable to an existing SDLK_ code */
 	ODD_keymap[XK_dead_grave&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_acute&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_tilde&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_macron&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_breve&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_abovedot&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_diaeresis&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_abovering&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_doubleacute&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_caron&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_cedilla&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_ogonek&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_iota&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_voiced_sound&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_semivoiced_sound&0xFF] = SDLK_COMPOSE;
 	ODD_keymap[XK_dead_belowdot&0xFF] = SDLK_COMPOSE;
#ifdef XK_dead_hook
 	ODD_keymap[XK_dead_hook&0xFF] = SDLK_COMPOSE;
#endif
#ifdef XK_dead_horn
 	ODD_keymap[XK_dead_horn&0xFF] = SDLK_COMPOSE;
#endif

#ifdef XK_dead_circumflex
	/* These X keysyms have 0xFE as the high byte */
	ODD_keymap[XK_dead_circumflex&0xFF] = SDLK_CARET;
#endif
#ifdef XK_ISO_Level3_Shift
	ODD_keymap[XK_ISO_Level3_Shift&0xFF] = SDLK_MODE; /* "Alt Gr" key */
#endif

	/* Map the miscellaneous keys */
	for ( i=0; i<SDL_arraysize(MISC_keymap); ++i )
		MISC_keymap[i] = SDLK_UNKNOWN;

	/* These X keysyms have 0xFF as the high byte */
	MISC_keymap[XK_BackSpace&0xFF] = SDLK_BACKSPACE;
	MISC_keymap[XK_Tab&0xFF] = SDLK_TAB;
	MISC_keymap[XK_Clear&0xFF] = SDLK_CLEAR;
	MISC_keymap[XK_Return&0xFF] = SDLK_RETURN;
	MISC_keymap[XK_Pause&0xFF] = SDLK_PAUSE;
	MISC_keymap[XK_Escape&0xFF] = SDLK_ESCAPE;
	MISC_keymap[XK_Delete&0xFF] = SDLK_DELETE;

	MISC_keymap[XK_KP_0&0xFF] = SDLK_KP0;		/* Keypad 0-9 */
	MISC_keymap[XK_KP_1&0xFF] = SDLK_KP1;
	MISC_keymap[XK_KP_2&0xFF] = SDLK_KP2;
	MISC_keymap[XK_KP_3&0xFF] = SDLK_KP3;
	MISC_keymap[XK_KP_4&0xFF] = SDLK_KP4;
	MISC_keymap[XK_KP_5&0xFF] = SDLK_KP5;
	MISC_keymap[XK_KP_6&0xFF] = SDLK_KP6;
	MISC_keymap[XK_KP_7&0xFF] = SDLK_KP7;
	MISC_keymap[XK_KP_8&0xFF] = SDLK_KP8;
	MISC_keymap[XK_KP_9&0xFF] = SDLK_KP9;
	MISC_keymap[XK_KP_Insert&0xFF] = SDLK_KP0;
	MISC_keymap[XK_KP_End&0xFF] = SDLK_KP1;	
	MISC_keymap[XK_KP_Down&0xFF] = SDLK_KP2;
	MISC_keymap[XK_KP_Page_Down&0xFF] = SDLK_KP3;
	MISC_keymap[XK_KP_Left&0xFF] = SDLK_KP4;
	MISC_keymap[XK_KP_Begin&0xFF] = SDLK_KP5;
	MISC_keymap[XK_KP_Right&0xFF] = SDLK_KP6;
	MISC_keymap[XK_KP_Home&0xFF] = SDLK_KP7;
	MISC_keymap[XK_KP_Up&0xFF] = SDLK_KP8;
	MISC_keymap[XK_KP_Page_Up&0xFF] = SDLK_KP9;
	MISC_keymap[XK_KP_Delete&0xFF] = SDLK_KP_PERIOD;
	MISC_keymap[XK_KP_Decimal&0xFF] = SDLK_KP_PERIOD;
	MISC_keymap[XK_KP_Divide&0xFF] = SDLK_KP_DIVIDE;
	MISC_keymap[XK_KP_Multiply&0xFF] = SDLK_KP_MULTIPLY;
	MISC_keymap[XK_KP_Subtract&0xFF] = SDLK_KP_MINUS;
	MISC_keymap[XK_KP_Add&0xFF] = SDLK_KP_PLUS;
	MISC_keymap[XK_KP_Enter&0xFF] = SDLK_KP_ENTER;
	MISC_keymap[XK_KP_Equal&0xFF] = SDLK_KP_EQUALS;

	MISC_keymap[XK_Up&0xFF] = SDLK_UP;
	MISC_keymap[XK_Down&0xFF] = SDLK_DOWN;
	MISC_keymap[XK_Right&0xFF] = SDLK_RIGHT;
	MISC_keymap[XK_Left&0xFF] = SDLK_LEFT;
	MISC_keymap[XK_Insert&0xFF] = SDLK_INSERT;
	MISC_keymap[XK_Home&0xFF] = SDLK_HOME;
	MISC_keymap[XK_End&0xFF] = SDLK_END;
	MISC_keymap[XK_Page_Up&0xFF] = SDLK_PAGEUP;
	MISC_keymap[XK_Page_Down&0xFF] = SDLK_PAGEDOWN;

	MISC_keymap[XK_F1&0xFF] = SDLK_F1;
	MISC_keymap[XK_F2&0xFF] = SDLK_F2;
	MISC_keymap[XK_F3&0xFF] = SDLK_F3;
	MISC_keymap[XK_F4&0xFF] = SDLK_F4;
	MISC_keymap[XK_F5&0xFF] = SDLK_F5;
	MISC_keymap[XK_F6&0xFF] = SDLK_F6;
	MISC_keymap[XK_F7&0xFF] = SDLK_F7;
	MISC_keymap[XK_F8&0xFF] = SDLK_F8;
	MISC_keymap[XK_F9&0xFF] = SDLK_F9;
	MISC_keymap[XK_F10&0xFF] = SDLK_F10;
	MISC_keymap[XK_F11&0xFF] = SDLK_F11;
	MISC_keymap[XK_F12&0xFF] = SDLK_F12;
	MISC_keymap[XK_F13&0xFF] = SDLK_F13;
	MISC_keymap[XK_F14&0xFF] = SDLK_F14;
	MISC_keymap[XK_F15&0xFF] = SDLK_F15;

	MISC_keymap[XK_Num_Lock&0xFF] = SDLK_NUMLOCK;
	MISC_keymap[XK_Caps_Lock&0xFF] = SDLK_CAPSLOCK;
	MISC_keymap[XK_Scroll_Lock&0xFF] = SDLK_SCROLLOCK;
	MISC_keymap[XK_Shift_R&0xFF] = SDLK_RSHIFT;
	MISC_keymap[XK_Shift_L&0xFF] = SDLK_LSHIFT;
	MISC_keymap[XK_Control_R&0xFF] = SDLK_RCTRL;
	MISC_keymap[XK_Control_L&0xFF] = SDLK_LCTRL;
	MISC_keymap[XK_Alt_R&0xFF] = SDLK_RALT;
	MISC_keymap[XK_Alt_L&0xFF] = SDLK_LALT;
	MISC_keymap[XK_Meta_R&0xFF] = SDLK_RMETA;
	MISC_keymap[XK_Meta_L&0xFF] = SDLK_LMETA;
	MISC_keymap[XK_Super_L&0xFF] = SDLK_LSUPER; /* Left "Windows" */
	MISC_keymap[XK_Super_R&0xFF] = SDLK_RSUPER; /* Right "Windows */
	MISC_keymap[XK_Mode_switch&0xFF] = SDLK_MODE; /* "Alt Gr" key */
	MISC_keymap[XK_Multi_key&0xFF] = SDLK_COMPOSE; /* Multi-key compose */

	MISC_keymap[XK_Help&0xFF] = SDLK_HELP;
	MISC_keymap[XK_Print&0xFF] = SDLK_PRINT;
	MISC_keymap[XK_Sys_Req&0xFF] = SDLK_SYSREQ;
	MISC_keymap[XK_Break&0xFF] = SDLK_BREAK;
	MISC_keymap[XK_Menu&0xFF] = SDLK_MENU;
	MISC_keymap[XK_Hyper_R&0xFF] = SDLK_MENU;   /* Windows "Menu" key */
}

/* Get the translated SDL virtual keysym */
SDLKey X11_TranslateKeycode(Display *display, KeyCode kc)
{
	KeySym xsym;
	SDLKey key;

	xsym = XKeycodeToKeysym(display, kc, 0);
#ifdef DEBUG_KEYS
	fprintf(stderr, "Translating key code %d -> 0x%.4x\n", kc, xsym);
#endif
	key = SDLK_UNKNOWN;
	if ( xsym ) {
		switch (xsym>>8) {
		    case 0x1005FF:
#ifdef SunXK_F36
			if ( xsym == SunXK_F36 )
				key = SDLK_F11;
#endif
#ifdef SunXK_F37
			if ( xsym == SunXK_F37 )
				key = SDLK_F12;
#endif
			break;
		    case 0x00:	/* Latin 1 */
			key = (SDLKey)(xsym & 0xFF);
			break;
		    case 0x01:	/* Latin 2 */
		    case 0x02:	/* Latin 3 */
		    case 0x03:	/* Latin 4 */
		    case 0x04:	/* Katakana */
		    case 0x05:	/* Arabic */
		    case 0x06:	/* Cyrillic */
		    case 0x07:	/* Greek */
		    case 0x08:	/* Technical */
		    case 0x0A:	/* Publishing */
		    case 0x0C:	/* Hebrew */
		    case 0x0D:	/* Thai */
			/* These are wrong, but it's better than nothing */
			key = (SDLKey)(xsym & 0xFF);
			break;
		    case 0xFE:
			key = ODD_keymap[xsym&0xFF];
			break;
		    case 0xFF:
			key = MISC_keymap[xsym&0xFF];
			break;
		    default:
			/*
			fprintf(stderr, "X11: Unhandled xsym, sym = 0x%04x\n",
					(unsigned int)xsym);
			*/
			break;
		}
	} else {
		/* X11 doesn't know how to translate the key! */
		switch (kc) {
		    /* Caution:
		       These keycodes are from the Microsoft Keyboard
		     */
		    case 115:
			key = SDLK_LSUPER;
			break;
		    case 116:
			key = SDLK_RSUPER;
			break;
		    case 117:
			key = SDLK_MENU;
			break;
		    default:
			/*
			 * no point in an error message; happens for
			 * several keys when we get a keymap notify
			 */
			break;
		}
	}
	return key;
}

/* X11 modifier masks for various keys */
static unsigned meta_l_mask, meta_r_mask, alt_l_mask, alt_r_mask;
static unsigned num_mask, mode_switch_mask;

static void get_modifier_masks(Display *display)
{
	static unsigned got_masks;
	int i, j;
	XModifierKeymap *xmods;
	unsigned n;

	if(got_masks)
		return;

	xmods = XGetModifierMapping(display);
	n = xmods->max_keypermod;
	for(i = 3; i < 8; i++) {
		for(j = 0; j < n; j++) {
			KeyCode kc = xmods->modifiermap[i * n + j];
			KeySym ks = XKeycodeToKeysym(display, kc, 0);
			unsigned mask = 1 << i;
			switch(ks) {
			case XK_Num_Lock:
				num_mask = mask; break;
			case XK_Alt_L:
				alt_l_mask = mask; break;
			case XK_Alt_R:
				alt_r_mask = mask; break;
			case XK_Meta_L:
				meta_l_mask = mask; break;
			case XK_Meta_R:
				meta_r_mask = mask; break;
			case XK_Mode_switch:
				mode_switch_mask = mask; break;
			}
		}
	}
	XFreeModifiermap(xmods);
	got_masks = 1;
}


/*
 * This function is semi-official; it is not officially exported and should
 * not be considered part of the SDL API, but may be used by client code
 * that *really* needs it (including legacy code).
 * It is slow, though, and should be avoided if possible.
 *
 * Note that it isn't completely accurate either; in particular, multi-key
 * sequences (dead accents, compose key sequences) will not work since the
 * state has been irrevocably lost.
 */
Uint16 X11_KeyToUnicode(SDLKey keysym, SDLMod modifiers)
{
	struct SDL_VideoDevice *this = current_video;
	char keybuf[32];
	int i;
	KeySym xsym = 0;
	XKeyEvent xkey;
	Uint16 unicode;

	if ( !this || !SDL_Display ) {
		return 0;
	}

	SDL_memset(&xkey, 0, sizeof(xkey));
	xkey.display = SDL_Display;

	xsym = keysym;		/* last resort if not found */
	for (i = 0; i < 256; ++i) {
		if ( MISC_keymap[i] == keysym ) {
			xsym = 0xFF00 | i;
			break;
		} else if ( ODD_keymap[i] == keysym ) {
			xsym = 0xFE00 | i;
			break;
		}
	}

	xkey.keycode = XKeysymToKeycode(xkey.display, xsym);

	get_modifier_masks(SDL_Display);
	if(modifiers & KMOD_SHIFT)
		xkey.state |= ShiftMask;
	if(modifiers & KMOD_CAPS)
		xkey.state |= LockMask;
	if(modifiers & KMOD_CTRL)
		xkey.state |= ControlMask;
	if(modifiers & KMOD_MODE)
		xkey.state |= mode_switch_mask;
	if(modifiers & KMOD_LALT)
		xkey.state |= alt_l_mask;
	if(modifiers & KMOD_RALT)
		xkey.state |= alt_r_mask;
	if(modifiers & KMOD_LMETA)
		xkey.state |= meta_l_mask;
	if(modifiers & KMOD_RMETA)
		xkey.state |= meta_r_mask;
	if(modifiers & KMOD_NUM)
		xkey.state |= num_mask;

	unicode = 0;
	if ( XLookupString(&xkey, keybuf, sizeof(keybuf), NULL, NULL) )
		unicode = (unsigned char)keybuf[0];
	return(unicode);
}


/*
 * Called when focus is regained, to read the keyboard state and generate
 * synthetic keypress/release events.
 * key_vec is a bit vector of keycodes (256 bits)
 */
void X11_SetKeyboardState(Display *display, const char *key_vec)
{
	char keys_return[32];
	int i;
	Uint8 *kstate = SDL_GetKeyState(NULL);
	SDLMod modstate;
	Window junk_window;
	int x, y;
	unsigned int mask;

	/* The first time the window is mapped, we initialize key state */
	if ( ! key_vec ) {
		XQueryKeymap(display, keys_return);
		key_vec = keys_return;
	}

	/* Get the keyboard modifier state */
	modstate = 0;
	get_modifier_masks(display);
	if ( XQueryPointer(display, DefaultRootWindow(display),
		&junk_window, &junk_window, &x, &y, &x, &y, &mask) ) {
		if ( mask & LockMask ) {
			modstate |= KMOD_CAPS;
		}
		if ( mask & mode_switch_mask ) {
			modstate |= KMOD_MODE;
		}
		if ( mask & num_mask ) {
			modstate |= KMOD_NUM;
		}
	}

	/* Zero the new keyboard state and generate it */
	SDL_memset(kstate, 0, SDLK_LAST);
	/*
	 * An obvious optimisation is to check entire longwords at a time in
	 * both loops, but we can't be sure the arrays are aligned so it's not
	 * worth the extra complexity
	 */
	for ( i = 0; i < 32; i++ ) {
		int j;
		if ( !key_vec[i] )
			continue;
		for ( j = 0; j < 8; j++ ) {
			if ( key_vec[i] & (1 << j) ) {
				SDLKey key;
				KeyCode kc = (i << 3 | j);
				key = X11_TranslateKeycode(display, kc);
				if ( key == SDLK_UNKNOWN ) {
					continue;
				}
				kstate[key] = SDL_PRESSED;
				switch (key) {
				    case SDLK_LSHIFT:
					modstate |= KMOD_LSHIFT;
					break;
				    case SDLK_RSHIFT:
					modstate |= KMOD_RSHIFT;
					break;
				    case SDLK_LCTRL:
					modstate |= KMOD_LCTRL;
					break;
				    case SDLK_RCTRL:
					modstate |= KMOD_RCTRL;
					break;
				    case SDLK_LALT:
					modstate |= KMOD_LALT;
					break;
				    case SDLK_RALT:
					modstate |= KMOD_RALT;
					break;
				    case SDLK_LMETA:
					modstate |= KMOD_LMETA;
					break;
				    case SDLK_RMETA:
					modstate |= KMOD_RMETA;
					break;
				    default:
					break;
				}
			}
		}
	}

	/* Hack - set toggle key state */
	if ( modstate & KMOD_CAPS ) {
		kstate[SDLK_CAPSLOCK] = SDL_PRESSED;
	} else {
		kstate[SDLK_CAPSLOCK] = SDL_RELEASED;
	}
	if ( modstate & KMOD_NUM ) {
		kstate[SDLK_NUMLOCK] = SDL_PRESSED;
	} else {
		kstate[SDLK_NUMLOCK] = SDL_RELEASED;
	}

	/* Set the final modifier state */
	SDL_SetModState(modstate);
}

void X11_InitOSKeymap(_THIS)
{
	X11_InitKeymap();
}

void X11_SaveScreenSaver(Display *display, int *saved_timeout, BOOL *dpms)
{
	int timeout, interval, prefer_blank, allow_exp;
	XGetScreenSaver(display, &timeout, &interval, &prefer_blank, &allow_exp);
	*saved_timeout = timeout;

#if SDL_VIDEO_DRIVER_X11_DPMS
	if ( SDL_X11_HAVE_DPMS ) {
		int dummy;
	  	if ( DPMSQueryExtension(display, &dummy, &dummy) ) {
			CARD16 state;
			DPMSInfo(display, &state, dpms);
		}
	}
#else
	*dpms = 0;
#endif /* SDL_VIDEO_DRIVER_X11_DPMS */
}

void X11_DisableScreenSaver(Display *display)
{
	int timeout, interval, prefer_blank, allow_exp;
	XGetScreenSaver(display, &timeout, &interval, &prefer_blank, &allow_exp);
	timeout = 0;
	XSetScreenSaver(display, timeout, interval, prefer_blank, allow_exp);

#if SDL_VIDEO_DRIVER_X11_DPMS
	if ( SDL_X11_HAVE_DPMS ) {
		int dummy;
	  	if ( DPMSQueryExtension(display, &dummy, &dummy) ) {
			DPMSDisable(display);
		}
	}
#endif /* SDL_VIDEO_DRIVER_X11_DPMS */
}

void X11_RestoreScreenSaver(Display *display, int saved_timeout, BOOL dpms)
{
	int timeout, interval, prefer_blank, allow_exp;
	XGetScreenSaver(display, &timeout, &interval, &prefer_blank, &allow_exp);
	timeout = saved_timeout;
	XSetScreenSaver(display, timeout, interval, prefer_blank, allow_exp);

#if SDL_VIDEO_DRIVER_X11_DPMS
	if ( SDL_X11_HAVE_DPMS ) {
		int dummy;
	  	if ( DPMSQueryExtension(display, &dummy, &dummy) ) {
			if ( dpms ) {
				DPMSEnable(display);
			}
		}
	}
#endif /* SDL_VIDEO_DRIVER_X11_DPMS */
}
