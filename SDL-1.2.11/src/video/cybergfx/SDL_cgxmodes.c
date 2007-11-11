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

/* Utilities for getting and setting the X display mode */

#include "SDL_timer.h"
#include "SDL_events.h"
#include "../../events/SDL_events_c.h"
#include "SDL_cgxvideo.h"
#include "SDL_cgxwm_c.h"
#include "SDL_cgxmodes_c.h"

#define CGX_DEBUG

static void set_best_resolution(_THIS, int width, int height)
{
	Uint32 idok;
	int depth=8;

	if(SDL_Display)
		depth=GetCyberMapAttr(SDL_Display->RastPort.BitMap,CYBRMATTR_DEPTH);

	idok=BestCModeIDTags(CYBRBIDTG_NominalWidth,width,
				CYBRBIDTG_NominalHeight,height,
				CYBRBIDTG_Depth,depth,
				TAG_DONE);

	if(idok!=INVALID_ID)
	{
		if(SDL_Display)
		{
			if(currently_fullscreen)
				CloseScreen(SDL_Display);
			else
				UnlockPubScreen(NULL,SDL_Display);
		}
		SDL_Display=GFX_Display=OpenScreenTags(NULL,SA_Width,width,SA_Height,height,
											SA_Depth,depth,SA_DisplayID,idok,
											SA_ShowTitle,FALSE,
											TAG_DONE);
	}
}

static void get_real_resolution(_THIS, int* w, int* h)
{
    *w = /*SDL_Display->Width*/ SDL_Window->Width-SDL_Window->BorderLeft-SDL_Window->BorderRight;
    *h = /*SDL_Display->Height*/ SDL_Window->Height-SDL_Window->BorderBottom-SDL_Window->BorderTop;
}

static void move_cursor_to(_THIS, int x, int y)
{
/*    XWarpPointer(SDL_Display, None, SDL_Root, 0, 0, 0, 0, x, y); */

/* DA FARE! */
}

static void add_visual(_THIS, int depth, int class)
{
	Uint32 tID;

	tID=BestCModeIDTags(CYBRBIDTG_Depth,depth,
						CYBRBIDTG_NominalWidth,640,
						CYBRBIDTG_NominalHeight,480,
						TAG_DONE);

	if(tID!=INVALID_ID)
	{
		int n = this->hidden->nvisuals;

		this->hidden->visuals[n].depth = depth;
		this->hidden->visuals[n].visual = tID;
		this->hidden->visuals[n].bpp = GetCyberIDAttr(CYBRIDATTR_BPPIX,tID);
		this->hidden->nvisuals++;
	}
}

#define TrueColor 1
#define PseudoColor 2

int CGX_GetVideoModes(_THIS)
{
    int i;
	ULONG nextid;
	int nmodes=0;

	SDL_modelist=NULL;

	nextid=NextDisplayInfo(INVALID_ID);

	while(nextid!=INVALID_ID)
	{
		if(IsCyberModeID(nextid))
		{
			DisplayInfoHandle h;

			if(h=FindDisplayInfo(nextid))
			{
				struct DimensionInfo info;

				if(GetDisplayInfoData(h,(char *)&info,sizeof(struct DimensionInfo),DTAG_DIMS,NULL))
				{
					int ok=0;

					for(i=0;i<nmodes;i++)
					{
						if(	SDL_modelist[i]->w == (info.Nominal.MaxX+1) &&
							SDL_modelist[i]->h == (info.Nominal.MaxY+1) )
							ok=1;
					}

					if(!ok)
					{
						nmodes++;

						SDL_modelist = (SDL_Rect **)SDL_realloc(SDL_modelist,(nmodes+1)*sizeof(SDL_Rect *));
						SDL_modelist[nmodes]=NULL;

						if ( SDL_modelist )
						{
							SDL_modelist[nmodes-1] = (SDL_Rect *)SDL_malloc(sizeof(SDL_Rect));

							if ( SDL_modelist[nmodes-1] == NULL )
								break;

							SDL_modelist[nmodes-1]->x = 0;
							SDL_modelist[nmodes-1]->y = 0;
							SDL_modelist[nmodes-1]->w = info.Nominal.MaxX+1;
							SDL_modelist[nmodes-1]->h = info.Nominal.MaxY+1;
						}
					}
				}
			}
		}
		nextid=NextDisplayInfo(nextid);
	}


	this->hidden->nvisuals = 0;
	/* Search for the visuals in deepest-first order, so that the first
	   will be the richest one */
	add_visual(this, 32, TrueColor);
	add_visual(this, 24, TrueColor);
	add_visual(this, 16, TrueColor);
	add_visual(this, 15, TrueColor);
	add_visual(this, 8, PseudoColor);

	if(this->hidden->nvisuals == 0) {
	    SDL_SetError("Found no sufficiently capable CGX visuals");
		    return -1;
	}

    if ( SDL_modelist == NULL ) {
        SDL_modelist = (SDL_Rect **)SDL_malloc((1+1)*sizeof(SDL_Rect *));
        i = 0;
        if ( SDL_modelist ) {
            SDL_modelist[i] = (SDL_Rect *)SDL_malloc(sizeof(SDL_Rect));
            if ( SDL_modelist[i] ) {
                SDL_modelist[i]->x = 0;
                SDL_modelist[i]->y = 0;
                SDL_modelist[i]->w = SDL_Display->Width;
                SDL_modelist[i]->h = SDL_Display->Height;
                ++i;
            }
            SDL_modelist[i] = NULL;
        }
    }

    D( if ( SDL_modelist ) {
        bug("CGX video mode list: (%ld)\n",nmodes);
        for ( i=0; SDL_modelist[i]; ++i ) {
            bug( "\t%ld x %ld\n",
                SDL_modelist[i]->w, SDL_modelist[i]->h);
        }
    	}
    );

    D(  { bug("CGX visuals list: (%ld)\n",this->hidden->nvisuals);

	for(i=0;i<this->hidden->nvisuals;i++)
		bug("\t%lx - depth: %ld bpp: %ld\n",this->hidden->visuals[i].visual,this->hidden->visuals[i].depth,this->hidden->visuals[i].bpp);
	}
    );
    return 0;
}

int CGX_SupportedVisual(_THIS, SDL_PixelFormat *format)
{
    int i;
    for(i = 0; i < this->hidden->nvisuals; i++)
	{
		if(this->hidden->visuals[i].depth == format->BitsPerPixel) // Era bpp
		    return 1;
	}
    return 0;
}

SDL_Rect **CGX_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
    if ( CGX_SupportedVisual(this, format) ) {
        if ( flags & SDL_FULLSCREEN ) {
            return(SDL_modelist);
        } else {
            return((SDL_Rect **)-1);
        }
    } else {
        return((SDL_Rect **)0);
    }
}

void CGX_FreeVideoModes(_THIS)
{
    int i;

    if ( SDL_modelist ) {
        for ( i=0; SDL_modelist[i]; ++i ) {
            SDL_free(SDL_modelist[i]);
        }
        SDL_free(SDL_modelist);
        SDL_modelist = NULL;
    }
}

int CGX_ResizeFullScreen(_THIS)
{
    int x, y;
    int real_w, real_h;

    if ( currently_fullscreen ) {
/* Per ora non faccio nulla qui */
    }
    return(1);
}

void _QueueEnterFullScreen(_THIS)
{
}

int CGX_EnterFullScreen(_THIS)
{
    int okay;
	 Uint32 saved_flags;

    okay = 1;
    saved_flags = this->screen->flags;

    if ( ! currently_fullscreen )
	{
        int real_w, real_h;

        /* Map the fullscreen window to blank the screen */
        get_real_resolution(this, &real_w, &real_h);

		CGX_DestroyWindow(this,this->screen);
		set_best_resolution(this, real_w,real_h);

        currently_fullscreen = 1;
		  this->screen->flags = saved_flags;

		  CGX_CreateWindow(this,this->screen,real_w,real_h,GetCyberMapAttr(SDL_Display->RastPort.BitMap,CYBRMATTR_DEPTH),this->screen->flags);

        /* Set the new resolution */
        okay = CGX_ResizeFullScreen(this);
        if ( ! okay ) {
            CGX_LeaveFullScreen(this);
        }
	/* Set the colormap */
/*
		if ( SDL_XColorMap ) {
			XInstallColormap(SDL_Display, SDL_XColorMap);
		}
*/
    }
//    CGX_GrabInputNoLock(this, this->input_grab | SDL_GRAB_FULLSCREEN);
    return(okay);
}

int CGX_LeaveFullScreen(_THIS)
{
    if ( currently_fullscreen ) {
		int width,height;
		if ( SDL_Window ) {
			CloseWindow(SDL_Window);
			SDL_Window=NULL;
		}
		CloseScreen(SDL_Display);

		GFX_Display=SDL_Display=LockPubScreen(NULL);

	        currently_fullscreen = 0;

		CGX_CreateWindow(this,this->screen,this->screen->w,this->screen->h,GetCyberMapAttr(SDL_Display->RastPort.BitMap,CYBRMATTR_DEPTH),this->screen->flags);
		CGX_ResizeImage(this,this->screen,0L);
    }

    return(0);
}
