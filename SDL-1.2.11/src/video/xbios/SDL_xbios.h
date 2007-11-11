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

#ifndef _SDL_xbios_h
#define _SDL_xbios_h

#include "SDL_stdinc.h"
#include "../SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

/* TT video modes:	2
   Falcon RVB:		16 (could be *2 by adding PAL/NTSC modes)
   Falcon VGA:		6
   ST low:		1
*/
#define SDL_NUMMODES 16

typedef struct
{
	Uint16 number;		/* Video mode number */
	Uint16 width;		/* Size */	
	Uint16 height;
	Uint16 depth;		/* bits per plane */
	SDL_bool doubleline;	/* Double the lines ? */
} xbiosmode_t;

/* Private display data */
#define NUM_MODELISTS	2		/* 8 and 16 bits-per-pixel */

struct SDL_PrivateVideoData {
	long cookie_vdo;
	int old_video_mode;				/* Old video mode before entering SDL */
	void *old_video_base;			/* Old pointer to screen buffer */
	void *old_palette;				/* Old palette */
	Uint32 old_num_colors;			/* Nb of colors in saved palette */
	int num_modes;					/* Number of xbios video modes */
	xbiosmode_t	*mode_list;			/* List of xbios video modes */

	void *screens[2];		/* Pointers to aligned screen buffer */
	void *screensmem[2];	/* Pointers to screen buffer */
	void *shadowscreen;		/* Shadow screen for c2p conversion */
	int doubleline;			/* Double line mode ? */
	int frame_number;		/* Number of frame for double buffer */
	int pitch;				/* Destination line width for C2P */
	int width, height;		/* Screen size for centered C2P */

	SDL_bool centscreen;	/* Centscreen extension present ? */

	SDL_Rect *SDL_modelist[NUM_MODELISTS][SDL_NUMMODES+1];
	xbiosmode_t *videomodes[NUM_MODELISTS][SDL_NUMMODES+1];
};

/* _VDO cookie values */
enum {
	VDO_ST=0,
	VDO_STE,
	VDO_TT,
	VDO_F30
};

/* Monitor types */
enum {
	MONITOR_MONO=0,
	MONITOR_TV,
	MONITOR_VGA,
	MONITOR_RGB
};

/* EgetShift masks */
#define ES_BANK		0x000f
#define ES_MODE		0x0700
#define ES_GRAY		0x1000
#define ES_SMEAR	0x8000

/* TT shifter modes */
#define ST_LOW	0x0000
#define ST_MED	0x0100
#define ST_HIGH	0x0200
#define TT_LOW	0x0700
#define TT_MED	0x0300
#define TT_HIGH	0x0600

/* Hidden structure -> variables names */
#define SDL_modelist		(this->hidden->SDL_modelist)
#define XBIOS_mutex		    (this->hidden->mutex)
#define XBIOS_cvdo		    (this->hidden->cookie_vdo)
#define XBIOS_oldpalette	(this->hidden->old_palette)
#define XBIOS_oldnumcol		(this->hidden->old_num_colors)
#define XBIOS_oldvbase		(this->hidden->old_video_base)
#define XBIOS_oldvmode		(this->hidden->old_video_mode)
#define XBIOS_nummodes		(this->hidden->num_modes)
#define XBIOS_modelist		(this->hidden->mode_list)
#define XBIOS_screens		(this->hidden->screens)
#define XBIOS_screensmem	(this->hidden->screensmem)
#define XBIOS_shadowscreen	(this->hidden->shadowscreen)
#define XBIOS_videomodes	(this->hidden->videomodes)
#define XBIOS_doubleline	(this->hidden->doubleline)
#define XBIOS_fbnum			(this->hidden->frame_number)
#define XBIOS_pitch			(this->hidden->pitch)
#define XBIOS_width			(this->hidden->width)
#define XBIOS_height		(this->hidden->height)
#define XBIOS_centscreen	(this->hidden->centscreen)

/*--- Functions prototypes ---*/

void SDL_XBIOS_AddMode(_THIS, Uint16 modecode, Uint16 width, Uint16 height,
	Uint16 depth, SDL_bool flags);

#endif /* _SDL_xbios_h */
