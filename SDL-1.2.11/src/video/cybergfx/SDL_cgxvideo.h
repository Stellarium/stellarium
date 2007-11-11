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

#ifndef _SDL_cgxvideo_h
#define _SDL_cgxvideo_h


#include <exec/exec.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/scale.h>
#include <graphics/gfx.h>
#include <intuition/intuition.h>
#if defined(__SASC) || defined(STORMC4_WOS)
#include <proto/exec.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/console.h>
#else
#include <inline/exec.h>
#include <inline/cybergraphics.h>
#include <inline/graphics.h>
#include <inline/intuition.h>
#include <inline/console.h>
#endif

#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "mydebug.h"

#define USE_CGX_WRITELUTPIXEL

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

/* Private display data */
struct SDL_PrivateVideoData {
    struct Screen *Public_Display; /* Used for events and window management */
    struct Screen *GFX_Display;	/* Used for graphics and colormap stuff */
    Uint32 SDL_VisualUnused;		/* The visual used by our window */
    struct Window *SDL_Window;	/* Shared by both displays (no X security?) */
    unsigned char *BlankCursor;	/* The invisible cursor */

    char *SDL_windowid;		/* Flag: true if we have been passed a window */

    /* The variables used for displaying graphics */
    Uint8 *Ximage;		/* The X image for our window */
    int swap_pixels;		/* Flag: true if display is swapped endian */

    /* Support for internal mouse warping */
    struct {
        int x;
        int y;
    } mouse_last;
    struct {
        int numerator;
        int denominator;
        int threshold;
    } mouse_accel;
    int mouse_relative;

    /* The current list of available video modes */
    SDL_Rect **modelist;

    /* available visuals of interest to us, sorted deepest first */
    struct {
		Uint32 visual;
		int depth;		/* number of significant bits/pixel */
		int bpp;		/* pixel quantum in bits */
    } visuals[5];		/* at most entries for 8, 15, 16, 24 */
    int nvisuals;

    Uint32 vis;		/* current visual in use */
    int depth;			/* current visual depth (not bpp) */
    int BytesPerPixel;
    int currently_fullscreen,same_format,dbuffer;

    /* Automatic mode switching support (entering/leaving fullscreen) */
    Uint32 switch_waiting;
    Uint32 switch_time;

    /* Prevent too many XSync() calls */
    int blit_queued;

    /* Colormap handling */
    LONG Pens;
    Sint32 *XPixels;		/* A list of pixels that have been allocated, the size depends on the screen format */
	struct ScreenBuffer *SB[2];
	struct RastPort *RP;
    short *iconcolors;		/* List of colors used by the icon */
};

/* Old variable names */
#define local_X11		(this->hidden->local_X11)
#define SDL_Display		(this->hidden->Public_Display)
#define GFX_Display		(this->hidden->GFX_Display)
#define SDL_Screen		DefaultScreen(this->hidden->Public_Display)

#define SDL_Visual		(this->hidden->vis)

#define SDL_Root		RootWindow(SDL_Display, SDL_Screen)
#define WMwindow		(this->hidden->WMwindow)
#define FSwindow		(this->hidden->FSwindow)
#define SDL_Window		(this->hidden->SDL_Window)
#define WM_DELETE_WINDOW	(this->hidden->WM_DELETE_WINDOW)
#define SDL_BlankCursor		(this->hidden->BlankCursor)
#define SDL_windowid		(this->hidden->SDL_windowid)
#define SDL_Ximage		(this->hidden->Ximage)
#define SDL_GC			(this->hidden->gc)
#define swap_pixels		(this->hidden->swap_pixels)
#define mouse_last		(this->hidden->mouse_last)
#define mouse_accel		(this->hidden->mouse_accel)
#define mouse_relative		(this->hidden->mouse_relative)
#define SDL_modelist		(this->hidden->modelist)
#define SDL_RastPort		(this->hidden->RP)
#define saved_mode		(this->hidden->saved_mode)
#define saved_view		(this->hidden->saved_view)
#define currently_fullscreen	(this->hidden->currently_fullscreen)
#define blit_queued		(this->hidden->blit_queued)
#define SDL_DisplayColormap	(this->hidden->GFX_Display->ViewPort.ColorMap)
#define SDL_XPixels		(this->hidden->XPixels)
#define SDL_iconcolors		(this->hidden->iconcolors)

/* Used to get the X cursor from a window-manager specific cursor */
// extern Cursor SDL_GetWMXCursor(WMcursor *cursor);

extern int CGX_CreateWindow(_THIS, SDL_Surface *screen,
			    int w, int h, int bpp, Uint32 flags);
extern int CGX_ResizeWindow(_THIS,
			SDL_Surface *screen, int w, int h, Uint32 flags);

extern void CGX_DestroyWindow(_THIS, SDL_Surface *screen);

extern struct Library *CyberGfxBase;
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;

struct private_hwdata
{
	struct BitMap *bmap;
	APTR lock;
	struct SDL_VideoDevice *videodata;
	APTR mask;
	int allocated;
};

int CGX_CheckHWBlit(_THIS,SDL_Surface *src,SDL_Surface *dst);
int CGX_FillHWRect(_THIS,SDL_Surface *dst,SDL_Rect *dstrect,Uint32 color);
int CGX_SetHWColorKey(_THIS,SDL_Surface *surface, Uint32 key);
#endif /* _SDL_x11video_h */
