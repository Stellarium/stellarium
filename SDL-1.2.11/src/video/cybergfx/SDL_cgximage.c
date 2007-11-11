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

#include "SDL_endian.h"
#include "SDL_cgximage_c.h"

#ifdef HAVE_KSTAT
#include <kstat.h>
#endif

#ifdef USE_CGX_WRITELUTPIXEL
#if defined(__SASC) || defined(__PPC__)
	#define WLUT WriteLUTPixelArray
#else
void WLUT(APTR a,UWORD b,UWORD c,UWORD d,struct RastPort *e,APTR f,UWORD g,UWORD h,UWORD i,UWORD l,UBYTE m)
{	WriteLUTPixelArray(a,b,c,d,e,f,g,h,i,l,m); }
#endif

#endif

/* Various screen update functions available */
static void CGX_NormalUpdate(_THIS, int numrects, SDL_Rect *rects);
static void CGX_FakeUpdate(_THIS, int numrects, SDL_Rect *rects);

BOOL SafeDisp=TRUE,SafeChange=TRUE;
struct MsgPort *safeport=NULL,*dispport=NULL;
ULONG safe_sigbit,disp_sigbit;
int use_picasso96=1;

int CGX_SetupImage(_THIS, SDL_Surface *screen)
{
	SDL_Ximage=NULL;

	if(screen->flags&SDL_HWSURFACE) {
		ULONG pitch;

		if(!screen->hwdata) {
			if(!(screen->hwdata=SDL_malloc(sizeof(struct private_hwdata))))
				return -1;

			D(bug("Creating system accel struct\n"));
		}
		screen->hwdata->lock=NULL;
		screen->hwdata->allocated=0;
		screen->hwdata->mask=NULL;
		screen->hwdata->bmap=SDL_RastPort->BitMap;
		screen->hwdata->videodata=this;

		if(!(screen->hwdata->lock=LockBitMapTags(screen->hwdata->bmap,
				LBMI_BASEADDRESS,(ULONG)&screen->pixels,
				LBMI_BYTESPERROW,(ULONG)&pitch,TAG_DONE))) {
			SDL_free(screen->hwdata);
			screen->hwdata=NULL;
			return -1;
		}
		else {
			UnLockBitMap(screen->hwdata->lock);
			screen->hwdata->lock=NULL;
		}

		screen->pitch=pitch;

		this->UpdateRects = CGX_FakeUpdate;

		D(bug("Accel video image configured (%lx, pitch %ld).\n",screen->pixels,screen->pitch));
		return 0;
	}

	screen->pixels = SDL_malloc(screen->h*screen->pitch);

	if ( screen->pixels == NULL ) {
		SDL_OutOfMemory();
		return(-1);
	}

	SDL_Ximage=screen->pixels;

	if ( SDL_Ximage == NULL ) {
		SDL_SetError("Couldn't create XImage");
		return(-1);
	}

	this->UpdateRects = CGX_NormalUpdate;

	return(0);
}

void CGX_DestroyImage(_THIS, SDL_Surface *screen)
{
	if ( SDL_Ximage ) {
		SDL_free(SDL_Ximage);
		SDL_Ximage = NULL;
	}
	if ( screen ) {
		screen->pixels = NULL;

		if(screen->hwdata) {
			SDL_free(screen->hwdata);
			screen->hwdata=NULL;
		}
	}
}

/* This is a hack to see whether this system has more than 1 CPU */
static int num_CPU(void)
{
	return 1;
}

int CGX_ResizeImage(_THIS, SDL_Surface *screen, Uint32 flags)
{
	int retval;

	D(bug("Calling ResizeImage()\n"));

	CGX_DestroyImage(this, screen);

	if ( flags & SDL_OPENGL ) {  /* No image when using GL */
        	retval = 0;
	} else {
		retval = CGX_SetupImage(this, screen);
		/* We support asynchronous blitting on the display */
		if ( flags & SDL_ASYNCBLIT ) {
			if ( num_CPU() > 1 ) {
				screen->flags |= SDL_ASYNCBLIT;
			}
		}
	}
	return(retval);
}

int CGX_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	D(bug("Alloc HW surface...%ld x %ld x %ld!\n",surface->w,surface->h,this->hidden->depth));

	if(surface==SDL_VideoSurface)
	{
		D(bug("Allocation skipped, it's system one!\n"));
		return 0;
	}

	if(!surface->hwdata)
	{
		if(!(surface->hwdata=SDL_malloc(sizeof(struct private_hwdata))))
			return -1;
	}

	surface->hwdata->mask=NULL;
	surface->hwdata->lock=NULL;
	surface->hwdata->videodata=this;
	surface->hwdata->allocated=0;

	if(surface->hwdata->bmap=AllocBitMap(surface->w,surface->h,this->hidden->depth,BMF_MINPLANES,SDL_Display->RastPort.BitMap))
	{
		surface->hwdata->allocated=1;
		surface->flags|=SDL_HWSURFACE;
		D(bug("...OK\n"));
		return 0;
	}
	else
	{
		SDL_free(surface->hwdata);
		surface->hwdata=NULL;
	}

	return(-1);
}
void CGX_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	if(surface && surface!=SDL_VideoSurface && surface->hwdata)
	{
		D(bug("Free hw surface.\n"));

		if(surface->hwdata->mask)
			SDL_free(surface->hwdata->mask);

		if(surface->hwdata->bmap&&surface->hwdata->allocated)
			FreeBitMap(surface->hwdata->bmap);

		SDL_free(surface->hwdata);
		surface->hwdata=NULL;
		surface->pixels=NULL;
		D(bug("end of free hw surface\n"));
	}
	return;
}

int CGX_LockHWSurface(_THIS, SDL_Surface *surface)
{
	if (surface->hwdata)
	{
//		D(bug("Locking a bitmap...\n"));
		if(!surface->hwdata->lock)
		{	
			Uint32 pitch;

			if(!(surface->hwdata->lock=LockBitMapTags(surface->hwdata->bmap,
					LBMI_BASEADDRESS,(ULONG)&surface->pixels,
					LBMI_BYTESPERROW,(ULONG)&pitch,TAG_DONE)))
				return -1;

// surface->pitch e' a 16bit!

			surface->pitch=pitch;

			if(!currently_fullscreen&&surface==SDL_VideoSurface)
				surface->pixels=((char *)surface->pixels)+(surface->pitch*(SDL_Window->BorderTop+SDL_Window->TopEdge)+
					surface->format->BytesPerPixel*(SDL_Window->BorderLeft+SDL_Window->LeftEdge));
		}
		D(else bug("Already locked!!!\n"));
	}
	return(0);
}

void CGX_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	if(surface->hwdata && surface->hwdata->lock)
	{
		UnLockBitMap(surface->hwdata->lock);
		surface->hwdata->lock=NULL;
//		surface->pixels=NULL;
	}
}

int CGX_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	static int current=0;

	if(this->hidden->dbuffer)
	{
		if(!SafeChange)
		{
			Wait(disp_sigbit);
// Non faccio nulla, vuoto solo la porta
			while(GetMsg(dispport)!=NULL) 
				;
			SafeChange=TRUE;
		}

		if(ChangeScreenBuffer(SDL_Display,this->hidden->SB[current^1]))
		{
			surface->hwdata->bmap=SDL_RastPort->BitMap=this->hidden->SB[current]->sb_BitMap;
			SafeChange=FALSE;
			SafeDisp=FALSE;
			current^=1;
		}

		if(!SafeDisp)
		{
			Wait(safe_sigbit);
			while(GetMsg(safeport)!=NULL) 
				;
			SafeDisp=TRUE;
		}

	}
	return(0);
}

/* Byte-swap the pixels in the display image */
static void CGX_SwapAllPixels(SDL_Surface *screen)
{
	int x, y;

	switch (screen->format->BytesPerPixel) {
	    case 2: {
		Uint16 *spot;
		for ( y=0; y<screen->h; ++y ) {
			spot = (Uint16 *) ((Uint8 *)screen->pixels +
						y * screen->pitch);
			for ( x=0; x<screen->w; ++x, ++spot ) {
				*spot = SDL_Swap16(*spot);
			}
		}
	    }
	    break;

	    case 4: {
		Uint32 *spot;
		for ( y=0; y<screen->h; ++y ) {
			spot = (Uint32 *) ((Uint8 *)screen->pixels +
						y * screen->pitch);
			for ( x=0; x<screen->w; ++x, ++spot ) {
				*spot = SDL_Swap32(*spot);
			}
		}
	    }
	    break;

	    default:
		/* should never get here */
		break;
	}
}
static void CGX_SwapPixels(SDL_Surface *screen, int numrects, SDL_Rect *rects)
{
	int i;
	int x, minx, maxx;
	int y, miny, maxy;

	switch (screen->format->BytesPerPixel) {
	    case 2: {
		Uint16 *spot;
		for ( i=0; i<numrects; ++i ) {
			minx = rects[i].x;
			maxx = rects[i].x+rects[i].w;
			miny = rects[i].y;
			maxy = rects[i].y+rects[i].h;
			for ( y=miny; y<maxy; ++y ) {
				spot = (Uint16 *) ((Uint8 *)screen->pixels +
						y * screen->pitch + minx * 2);
				for ( x=minx; x<maxx; ++x, ++spot ) {
					*spot = SDL_Swap16(*spot);
				}
			}
		}
	    }
	    break;

	    case 4: {
		Uint32 *spot;
		for ( i=0; i<numrects; ++i ) {
			minx = rects[i].x;
			maxx = rects[i].x+rects[i].w;
			miny = rects[i].y;
			maxy = rects[i].y+rects[i].h;
			for ( y=miny; y<maxy; ++y ) {
				spot = (Uint32 *) ((Uint8 *)screen->pixels +
						y * screen->pitch + minx * 4);
				for ( x=minx; x<maxx; ++x, ++spot ) {
					*spot = SDL_Swap32(*spot);
				}
			}
		}
	    }
	    break;

	    default:
		/* should never get here */
		break;
	}
}

#ifdef __SASC

#define USE_WPA WritePixelArray
#else

void USE_WPA(char *a,int b,int c,int d, struct RastPort *e,int f,int g, int h, int i, Uint32 l)
{
		WritePixelArray(a,b,c,d,e,f,g,h,i,l);
}

#endif

static void CGX_FakeUpdate(_THIS, int numrects, SDL_Rect *rects)
{
}

static void CGX_NormalUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	int i,format,customroutine=0;
#ifndef USE_CGX_WRITELUTPIXEL
	int bpp;
#endif
	if(this->hidden->same_format && !use_picasso96)
	{
		format=RECTFMT_RAW;
	}
	else switch(this->screen->format->BytesPerPixel)
	{
		case 4:
			format=RECTFMT_RGBA;
			break;
		case 3:
			format=RECTFMT_RGB;
			break;
		case 2:
			customroutine=1;
			break;
		case 1:
//			D(bug("soft depth: 8 hardbpp: %ld\n",this->hidden->depth));
			if(this->hidden->depth>8)
			{
#ifndef USE_CGX_WRITELUTPIXEL
				if(this->hidden->depth>32)
					customroutine=4;
				else if(this->hidden->depth>16)
				{
					bpp=this->hidden->BytesPerPixel; // That one is the only one that needs bpp
					customroutine=2; // The slow one!
				}
				else
					customroutine=3;
#else

				customroutine=2;
#endif
				
//				format=RECTFMT_LUT8;   Vecchia funzione x usare la WritePixelArray.
			}
			else
				customroutine=1;
			break;
		default:
			D(bug("Unable to blit this surface!\n"));	
			return;
	}

	/* Check for endian-swapped X server, swap if necessary (VERY slow!) */
	if ( swap_pixels &&
	     ((this->screen->format->BytesPerPixel%2) == 0) ) {
		D(bug("Software Swapping! SLOOOW!\n"));
		CGX_SwapPixels(this->screen, numrects, rects);
		for ( i=0; i<numrects; ++i ) {
			if ( ! rects[i].w ) { /* Clipped? */
				continue;
			}
			USE_WPA(this->screen->pixels,rects[i].x, rects[i].y,this->screen->pitch,
					SDL_RastPort,SDL_Window->BorderLeft+rects[i].x,SDL_Window->BorderTop+rects[i].y,
					rects[i].w,rects[i].h,format);
		}
		CGX_SwapPixels(this->screen, numrects, rects);
	}
	else if (customroutine==2)
	{
#ifdef USE_CGX_WRITELUTPIXEL
		for ( i=0; i<numrects; ++i ) {
			if ( ! rects[i].w ) { /* Clipped? */
				continue;
			}

			WLUT(this->screen->pixels,rects[i].x, rects[i].y,this->screen->pitch,
					SDL_RastPort,SDL_XPixels,SDL_Window->BorderLeft+rects[i].x,SDL_Window->BorderTop+rects[i].y,
					rects[i].w,rects[i].h,CTABFMT_XRGB8);
		}
#else
		unsigned char *bm_address;
		Uint32	destpitch;
		APTR handle;

		if(handle=LockBitMapTags(SDL_RastPort->BitMap,LBMI_BASEADDRESS,&bm_address,
								LBMI_BYTESPERROW,&destpitch,TAG_DONE))
		{
			int srcwidth;
			unsigned char *destbase;
			register int j,k,t;
			register unsigned char *mask,*dst;
			register unsigned char *src,*dest;

// Aggiungo il bordo della finestra se sono fullscreen.
			if(currently_fullscreen)
				destbase=bm_address;
			else
				destbase=bm_address+(SDL_Window->TopEdge+SDL_Window->BorderTop)*destpitch+(SDL_Window->BorderLeft+SDL_Window->LeftEdge)*this->hidden->BytesPerPixel;

			for ( i=0; i<numrects; ++i ) 
			{
				srcwidth=rects[i].w;

				if ( !srcwidth ) { /* Clipped? */
					continue;
				}

				dest=destbase+rects[i].x*this->hidden->BytesPerPixel;
				dest+=(rects[i].y*destpitch);
				src=((char *)(this->screen->pixels))+rects[i].x;
				src+=(rects[i].y*this->screen->pitch);
				
				for(j=rects[i].h;j;--j)
				{
					dst=dest;
// SLOW routine, used for 8->24 bit mapping
					for(k=0;k<srcwidth;k++)
					{
						mask=(unsigned char *)(&SDL_XPixels[src[k]]);
						for(t=0;t<bpp;t++)
						{
							dst[t]=mask[t];
						}
						dst+=bpp;
					}
					src+=this->screen->pitch;
					dest+=destpitch;
				}
			}
			UnLockBitMap(handle);
		}
	}
	else if (customroutine==3)
	{
		unsigned char *bm_address;
		Uint32	destpitch;
		APTR handle;

		if(handle=LockBitMapTags(SDL_RastPort->BitMap,LBMI_BASEADDRESS,&bm_address,
								LBMI_BYTESPERROW,&destpitch,TAG_DONE))
		{
			int srcwidth;
			unsigned char *destbase;
			register int j,k;
			register unsigned char *src,*dest;
			register Uint16 *destl,*srcl;

			if(currently_fullscreen)
				destbase=bm_address;
			else
				destbase=bm_address+(SDL_Window->TopEdge+SDL_Window->BorderTop)*destpitch+(SDL_Window->BorderLeft+SDL_Window->LeftEdge)*this->hidden->BytesPerPixel;

			for ( i=0; i<numrects; ++i ) 
			{
				srcwidth=rects[i].w;

				if ( !srcwidth ) { /* Clipped? */
					continue;
				}

				dest=destbase+rects[i].x*this->hidden->BytesPerPixel;
				dest+=(rects[i].y*destpitch);
				src=((char *)(this->screen->pixels))+rects[i].x;
				src+=(rects[i].y*this->screen->pitch);
				
// This is the fast, well not too slow, remapping code for 16bit displays

				for(j=rects[i].h;j;--j)
				{
					destl=(Uint16 *)dest;

					for(k=0;k<srcwidth;k++)
					{
						srcl=(Uint16 *)&SDL_XPixels[src[k]];
						*destl=*srcl;
						destl++;
					}
					src+=this->screen->pitch;
					dest+=destpitch;
				}
			}
			UnLockBitMap(handle);
		}
	}
	else if (customroutine==4)
	{
		unsigned char *bm_address;
		Uint32	destpitch;
		APTR handle;

		if(handle=LockBitMapTags(SDL_RastPort->BitMap,LBMI_BASEADDRESS,&bm_address,
								LBMI_BYTESPERROW,&destpitch,TAG_DONE))
		{
			int srcwidth;
			unsigned char *destbase;
			register int j,k;
			register unsigned char *src,*dest;
			register Uint32 *destl,*srcl;

			if(currently_fullscreen)
				destbase=bm_address;
			else
				destbase=bm_address+(SDL_Window->TopEdge+SDL_Window->BorderTop)*destpitch+(SDL_Window->BorderLeft+SDL_Window->LeftEdge)*this->hidden->BytesPerPixel;

			for ( i=0; i<numrects; ++i ) 
			{
				srcwidth=rects[i].w;

				if ( !srcwidth ) { /* Clipped? */
					continue;
				}

				dest=destbase+rects[i].x*this->hidden->BytesPerPixel;
				dest+=(rects[i].y*destpitch);
				src=((char *)(this->screen->pixels))+rects[i].x;
				src+=(rects[i].y*this->screen->pitch);
				
// This is the fast, well not too slow, remapping code for 32bit displays

				for(j=rects[i].h;j;--j)
				{
					destl=(Uint32 *)dest;

					for(k=0;k<srcwidth;k++)
					{
						srcl=(Uint32 *)&SDL_XPixels[src[k]];
						*destl=*srcl;
						destl++;
					}
					src+=this->screen->pitch;
					dest+=destpitch;
				}
			}
			UnLockBitMap(handle);
		}
#endif
	}
	else if(customroutine)
	{
		unsigned char *bm_address;
		Uint32	destpitch;
		APTR handle;

//		D(bug("Using customroutine!\n"));

		if(handle=LockBitMapTags(SDL_RastPort->BitMap,LBMI_BASEADDRESS,(ULONG)&bm_address,
								LBMI_BYTESPERROW,(ULONG)&destpitch,TAG_DONE))
		{
			unsigned char *destbase;
			register int j,srcwidth;
			register unsigned char *src,*dest;

// Aggiungo il bordo della finestra se sono fullscreen.
			if(currently_fullscreen)
				destbase=bm_address;
			else
				destbase=bm_address+(SDL_Window->TopEdge+SDL_Window->BorderTop)*destpitch+(SDL_Window->BorderLeft+SDL_Window->LeftEdge)*this->screen->format->BytesPerPixel;

			for ( i=0; i<numrects; ++i ) 
			{
				srcwidth=rects[i].w;

				if ( !srcwidth ) { /* Clipped? */
					continue;
				}

				dest=destbase+rects[i].x*this->screen->format->BytesPerPixel;
				dest+=(rects[i].y*destpitch);
				src=((char *)(this->screen->pixels))+rects[i].x*this->screen->format->BytesPerPixel;
				src+=(rects[i].y*this->screen->pitch);
				
				srcwidth*=this->screen->format->BytesPerPixel;

//				D(bug("Rects: %ld,%ld %ld,%ld Src:%lx Dest:%lx\n",rects[i].x,rects[i].y,rects[i].w,rects[i].h,src,dest));

				for(j=rects[i].h;j;--j)
				{
					SDL_memcpy(dest,src,srcwidth);
					src+=this->screen->pitch;
					dest+=destpitch;
				}
			}
			UnLockBitMap(handle);
//			D(bug("Rectblit addr: %lx pitch: %ld rects:%ld srcptr: %lx srcpitch: %ld\n",bm_address,destpitch,numrects,this->screen->pixels,this->screen->pitch));
		}
	}
	else
	{
		for ( i=0; i<numrects; ++i ) {
			if ( ! rects[i].w ) { /* Clipped? */
				continue;
			}
			USE_WPA(this->screen->pixels,rects[i].x, rects[i].y,this->screen->pitch,
					SDL_RastPort,SDL_Window->BorderLeft+rects[i].x,SDL_Window->BorderTop+rects[i].y,
					rects[i].w,rects[i].h,format);
		}
	}
}

void CGX_RefreshDisplay(_THIS)
{
	int format,customroutine=0;
#ifndef USE_CGX_WRITELUTPIXEL
	int bpp;
#endif
	/* Don't refresh a display that doesn't have an image (like GL) */
	if ( ! SDL_Ximage ) {
		return;
	}

	if(this->hidden->same_format && !use_picasso96)
	{
		format=RECTFMT_RAW;
	}
	else switch(this->screen->format->BytesPerPixel)
	{
		case 4:
			format=RECTFMT_RGBA;
			break;
		case 3:
			format=RECTFMT_RGB;
			break;
		case 2:
			customroutine=1;
			break;
		case 1:
//			D(bug("soft depth: 8 hardbpp: %ld\n",this->hidden->depth));
			if(this->hidden->depth>8)
			{
#ifndef USE_CGX_WRITELUTPIXEL
				if(this->hidden->depth>32)
					customroutine=4;
				else if(this->hidden->depth>16)
				{
					bpp=this->hidden->BytesPerPixel; // That one is the only one that needs bpp
					customroutine=2; // The slow one!
				}
				else
					customroutine=3;
#else

				customroutine=2;
#endif
//				format=RECTFMT_LUT8;
			}
			else
				customroutine=1;
			break;

	}

		/* Check for endian-swapped X server, swap if necessary */
	if ( swap_pixels &&
	     ((this->screen->format->BytesPerPixel%2) == 0) ) {
		CGX_SwapAllPixels(this->screen);
		USE_WPA(this->screen->pixels,0,0,this->screen->pitch,
				SDL_RastPort,SDL_Window->BorderLeft,SDL_Window->BorderTop,
				this->screen->w,this->screen->h,format);
		CGX_SwapAllPixels(this->screen);
	}
	else if (customroutine==2)
	{
#ifdef USE_CGX_WRITELUTPIXEL
		WLUT(this->screen->pixels,0,0,this->screen->pitch,
					SDL_RastPort,SDL_XPixels,SDL_Window->BorderLeft,SDL_Window->BorderTop,
					this->screen->w,this->screen->h,CTABFMT_XRGB8);
#else
		unsigned char *bm_address;
		Uint32	destpitch;
		APTR handle;

		if(handle=LockBitMapTags(SDL_RastPort->BitMap,LBMI_BASEADDRESS,(ULONG)&bm_address,
								LBMI_BYTESPERROW,(ULONG)&destpitch,TAG_DONE))
		{
			register int j,k,t;
			register unsigned char *mask,*dst;
			register unsigned char *src,*dest;

// Aggiungo il bordo della finestra se sono fullscreen.
			if(!currently_fullscreen)
				dest=bm_address+(SDL_Window->TopEdge+SDL_Window->BorderTop)*destpitch+(SDL_Window->BorderLeft+SDL_Window->LeftEdge)*this->hidden->BytesPerPixel;
			else
				dest=bm_address;

			src=this->screen->pixels;
				
			for(j=this->screen->h;j;--j)
			{
				dst=dest;
// SLOW routine, used for 8->24 bit mapping
				for(k=0;k<this->screen->w;k++)
				{
					mask=(unsigned char *)(&SDL_XPixels[src[k]]);
					for(t=0;t<bpp;t++)
					{
						dst[t]=mask[t];
					}
					dst+=bpp;
				}
				src+=this->screen->pitch;
				dest+=destpitch;
			}
			UnLockBitMap(handle);
		}
	}
	else if (customroutine==3)
	{
		unsigned char *bm_address;
		Uint32	destpitch;
		APTR handle;

		if(handle=LockBitMapTags(SDL_RastPort->BitMap,LBMI_BASEADDRESS,(ULONG)&bm_address,
								LBMI_BYTESPERROW,(ULONG)&destpitch,TAG_DONE))
		{
			register int j,k;
			register unsigned char *src,*dest;
			register Uint16 *destl,*srcl;

			if(!currently_fullscreen)
				dest=bm_address+(SDL_Window->TopEdge+SDL_Window->BorderTop)*destpitch+(SDL_Window->BorderLeft+SDL_Window->LeftEdge)*this->hidden->BytesPerPixel;
			else
				dest=bm_address;

			src=this->screen->pixels;
				
// This is the fast, well not too slow, remapping code for 16bit displays

			for(j=this->screen->h;j;--j)
			{
				destl=(Uint16 *)dest;

				for(k=0;k<this->screen->w;k++)
				{
					srcl=(Uint16 *)&SDL_XPixels[src[k]];
					*destl=*srcl;
					destl++;
				}
				src+=this->screen->pitch;
				dest+=destpitch;
			}
			UnLockBitMap(handle);
		}
	}
	else if (customroutine==4)
	{
		unsigned char *bm_address;
		Uint32	destpitch;
		APTR handle;

		if(handle=LockBitMapTags(SDL_RastPort->BitMap,LBMI_BASEADDRESS,(ULONG)&bm_address,
								LBMI_BYTESPERROW,(ULONG)&destpitch,TAG_DONE))
		{
			register int j,k;
			register unsigned char *src,*dest;
			register Uint32 *destl,*srcl;

			if(!currently_fullscreen)
				dest=bm_address+(SDL_Window->TopEdge+SDL_Window->BorderTop)*destpitch+(SDL_Window->BorderLeft+SDL_Window->LeftEdge)*this->hidden->BytesPerPixel;
			else
				dest=bm_address;

			src=this->screen->pixels;
				
// This is the fast, well not too slow, remapping code for 32bit displays

			for(j=this->screen->h;j;--j)
			{
				destl=(Uint32 *)dest;

				for(k=0;k<this->screen->w;k++)
				{
					srcl=(Uint32 *)&SDL_XPixels[src[k]];
					*destl=*srcl;
					destl++;
				}
				src+=this->screen->pitch;
				dest+=destpitch;
			}
			UnLockBitMap(handle);
		}
#endif
	}
	else if(customroutine)
	{
		unsigned char *bm_address;
		Uint32	destpitch;
		APTR handle;

		if(handle=LockBitMapTags(SDL_RastPort->BitMap,
					LBMI_BASEADDRESS,(ULONG)&bm_address,
					LBMI_BYTESPERROW,(ULONG)&destpitch,TAG_DONE))
		{
			register int j;
			register unsigned char *src,*dest;

			if(!currently_fullscreen)
				dest=bm_address+(SDL_Window->TopEdge+SDL_Window->BorderTop)*destpitch+(SDL_Window->BorderLeft+SDL_Window->LeftEdge)*this->screen->format->BytesPerPixel;
			else
				dest=bm_address;

			src=this->screen->pixels;
				
//			D(bug("addr: %lx pitch: %ld src:%lx srcpitch: %ld\n",dest,destpitch,this->screen->pixels,this->screen->pitch));

			if(this->screen->pitch==destpitch)
			{
				SDL_memcpy(dest,src,this->screen->pitch*this->screen->h);
			}
			else
			{
				for(j=this->screen->h;j;--j)
				{
					SDL_memcpy(dest,src,this->screen->pitch);
					src+=this->screen->pitch;
					dest+=destpitch;
				}
			}

			UnLockBitMap(handle);
		}
	}
	else
	{
		USE_WPA(this->screen->pixels,0,0,this->screen->pitch,
				SDL_RastPort,SDL_Window->BorderLeft,SDL_Window->BorderTop,
				this->screen->w,this->screen->h,format);
	}

}
