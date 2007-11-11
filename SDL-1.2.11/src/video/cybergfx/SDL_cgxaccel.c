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
#include "SDL_video.h"
#include "../SDL_sysvideo.h"
#include "../SDL_blit.h"
#include "SDL_cgxvideo.h"

static int CGX_HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect,
					SDL_Surface *dst, SDL_Rect *dstrect);

// These are needed to avoid register troubles with gcc -O2!

#if defined(__SASC) || defined(__PPC__) || defined(MORPHOS)
#define BMKBRP(a,b,c,d,e,f,g,h,i,j) BltMaskBitMapRastPort(a,b,c,d,e,f,g,h,i,j)
#define	BBRP(a,b,c,d,e,f,g,h,i) BltBitMapRastPort(a,b,c,d,e,f,g,h,i)
#define BBB(a,b,c,d,e,f,g,h,i,j,k) BltBitMap(a,b,c,d,e,f,g,h,i,j,k)
#else
void BMKBRP(struct BitMap *a,WORD b, WORD c,struct RastPort *d,WORD e,WORD f,WORD g,WORD h,UBYTE i,APTR j)
{BltMaskBitMapRastPort(a,b,c,d,e,f,g,h,i,j);}

void BBRP(struct BitMap *a,WORD b, WORD c,struct RastPort *d,WORD e,WORD f,WORD g,WORD h,UBYTE i)
{BltBitMapRastPort(a,b,c,d,e,f,g,h,i);}

void BBB(struct BitMap *a,WORD b, WORD c,struct BitMap *d,WORD e,WORD f,WORD g,WORD h,UBYTE i,UBYTE j,UWORD *k)
{BltBitMap(a,b,c,d,e,f,g,h,i,j,k);}
#endif

int CGX_SetHWColorKey(_THIS,SDL_Surface *surface, Uint32 key)
{
	if(surface->hwdata)
	{
		if(surface->hwdata->mask)
			SDL_free(surface->hwdata->mask);

		if(surface->hwdata->mask=SDL_malloc(RASSIZE(surface->w,surface->h)))
		{
			Uint32 pitch,ok=0;
			APTR lock;

			SDL_memset(surface->hwdata->mask,255,RASSIZE(surface->w,surface->h));

			D(bug("Building colorkey mask: color: %ld, size: %ld x %ld, %ld bytes...Bpp:%ld\n",key,surface->w,surface->h,RASSIZE(surface->w,surface->h),surface->format->BytesPerPixel));

			if(lock=LockBitMapTags(surface->hwdata->bmap,LBMI_BASEADDRESS,(ULONG)&surface->pixels,
					LBMI_BYTESPERROW,(ULONG)&pitch,TAG_DONE))
			{
				switch(surface->format->BytesPerPixel)
				{
					case 1:
					{
						unsigned char k=key;
						register int i,j,t;
						register unsigned char *dest=surface->hwdata->mask,*map=surface->pixels;

						pitch-=surface->w;

						for(i=0;i<surface->h;i++)
						{
							for(t=128,j=0;j<surface->w;j++)
							{
								if(*map==k)
									*dest&=~t;	

								t>>=1;

								if(t==0)
								{
									dest++;
									t=128;
								}
								map++;
							}
							map+=pitch;
						}
					}
					break;
					case 2:
					{
						Uint16 k=key,*mapw;
						register int i,j,t;
						register unsigned char *dest=surface->hwdata->mask,*map=surface->pixels;

						for(i=surface->h;i;--i)
						{
							mapw=(Uint16 *)map;

							for(t=128,j=surface->w;j;--j)
							{
								if(*mapw==k)
									*dest&=~t;

								t>>=1;

								if(t==0)
								{
									dest++;
									t=128;
								}
								mapw++;
							}
							map+=pitch;
						}
					}
					break;
					case 4:
					{
						Uint32 *mapl;
						register int i,j,t;
						register unsigned char *dest=surface->hwdata->mask,*map=surface->pixels;

						for(i=surface->h;i;--i)
						{
							mapl=(Uint32 *)map;

							for(t=128,j=surface->w;j;--j)
							{
								if(*mapl==key)
									*dest&=~t;

								t>>=1;

								if(t==0)
								{
									dest++;
									t=128;
								}
								mapl++;
							}
							map+=pitch;
						}

					}
					break;
					default:
						D(bug("Pixel mode non supported for color key..."));
						SDL_free(surface->hwdata->mask);
						surface->hwdata->mask=NULL;
						ok=-1;
				}
				UnLockBitMap(lock);
				D(bug("...Colorkey built!\n"));
				return ok;
			}
		}
	}
	D(bug("HW colorkey not supported for this depth\n"));

	return -1;
}

int CGX_CheckHWBlit(_THIS,SDL_Surface *src,SDL_Surface *dst)
{
// Doesn't support yet alpha blitting

	if(src->hwdata&& !(src->flags & (SDL_SRCALPHA)))
	{
		D(bug("CheckHW blit... OK!\n"));

		if ( (src->flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY ) {
			if ( CGX_SetHWColorKey(this, src, src->format->colorkey) < 0 ) {
				src->flags &= ~SDL_HWACCEL;
				return -1;
			}
		}

		src->flags|=SDL_HWACCEL;
		src->map->hw_blit = CGX_HWAccelBlit;
		return 1;
	}
	else
		src->flags &= ~SDL_HWACCEL;

	D(bug("CheckHW blit... NO!\n"));

	return 0;
}

static int temprp_init=0;
static struct RastPort temprp;

static int CGX_HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect,
					SDL_Surface *dst, SDL_Rect *dstrect)
{
	struct SDL_VideoDevice *this=src->hwdata->videodata;

//	D(bug("Accel blit!\n"));

	if(src->flags&SDL_SRCCOLORKEY && src->hwdata->mask)
	{
		if(dst==SDL_VideoSurface)
		{
			BMKBRP(src->hwdata->bmap,srcrect->x,srcrect->y,
						SDL_RastPort,dstrect->x+SDL_Window->BorderLeft,dstrect->y+SDL_Window->BorderTop,
						srcrect->w,srcrect->h,0xc0,src->hwdata->mask);
		}
		else if(dst->hwdata)
		{
			if(!temprp_init)
			{
				InitRastPort(&temprp);
				temprp_init=1;
			}
			temprp.BitMap=(struct BitMap *)dst->hwdata->bmap;

			BMKBRP(src->hwdata->bmap,srcrect->x,srcrect->y,
						&temprp,dstrect->x,dstrect->y,
						srcrect->w,srcrect->h,0xc0,src->hwdata->mask);
			
		}
	}
	else if(dst==SDL_VideoSurface)
	{
		BBRP(src->hwdata->bmap,srcrect->x,srcrect->y,SDL_RastPort,dstrect->x+SDL_Window->BorderLeft,dstrect->y+SDL_Window->BorderTop,srcrect->w,srcrect->h,0xc0);
	}
	else if(dst->hwdata)
		BBB(src->hwdata->bmap,srcrect->x,srcrect->y,dst->hwdata->bmap,dstrect->x,dstrect->y,srcrect->w,srcrect->h,0xc0,0xff,NULL);

	return 0;
}

int CGX_FillHWRect(_THIS,SDL_Surface *dst,SDL_Rect *dstrect,Uint32 color)
{
	if(dst==SDL_VideoSurface)
	{
		FillPixelArray(SDL_RastPort,dstrect->x+SDL_Window->BorderLeft,dstrect->y+SDL_Window->BorderTop,dstrect->w,dstrect->h,color);
	}
	else if(dst->hwdata)
	{
		if(!temprp_init)
		{
			InitRastPort(&temprp);
			temprp_init=1;
		}

		temprp.BitMap=(struct BitMap *)dst->hwdata->bmap;

		FillPixelArray(&temprp,dstrect->x,dstrect->y,dstrect->w,dstrect->h,color);
	}
	return 0;
}
