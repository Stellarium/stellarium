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
 * CGX based SDL video driver implementation by Gabriele Greco
 * gabriele.greco@aruba.it
 */

#include "SDL_endian.h"
#include "SDL_timer.h"
#include "SDL_thread.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"
#include "SDL_cgxgl_c.h"
#include "SDL_cgxvideo.h"
#include "SDL_cgxwm_c.h"
#include "SDL_amigamouse_c.h"
#include "SDL_amigaevents_c.h"
#include "SDL_cgxmodes_c.h"
#include "SDL_cgximage_c.h"

/* Initialization/Query functions */
static int CGX_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Surface *CGX_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int CGX_ToggleFullScreen(_THIS, int on);
static void CGX_UpdateMouse(_THIS);
static int CGX_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void CGX_VideoQuit(_THIS);

/* CGX driver bootstrap functions */

struct Library *CyberGfxBase=NULL;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;

int CGX_SetGamma(_THIS, float red, float green, float blue)
{
    SDL_SetError("Gamma correction not supported");
    return -1;
}

int CGX_GetGamma(_THIS, float red, float green, float blue)
{
    SDL_SetError("Gamma correction not supported");
    return -1;
}

int CGX_SetGammaRamp(_THIS, Uint16 *ramp)
{
#if 0
	Int i, ncolors;
	XColor xcmap[256];

	/* See if actually setting the gamma is supported */
	if ( SDL_Visual->class != DirectColor ) {
	    SDL_SetError("Gamma correction not supported on this visual");
	    return(-1);
	}

	/* Calculate the appropriate palette for the given gamma ramp */
	ncolors = SDL_Visual->map_entries;
	for ( i=0; i<ncolors; ++i ) {
		Uint8 c = (256 * i / ncolors);
		xcmap[i].pixel = SDL_MapRGB(this->screen->format, c, c, c);
		xcmap[i].red   = ramp[0*256+c];
		xcmap[i].green = ramp[1*256+c];
		xcmap[i].blue  = ramp[2*256+c];
		xcmap[i].flags = (DoRed|DoGreen|DoBlue);
	}
	XStoreColors(GFX_Display, SDL_XColorMap, xcmap, ncolors);
	XSync(GFX_Display, False);

	return(0);

#else
    SDL_SetError("Gamma correction not supported on this visual");
    return(-1);

#endif
}

static void DestroyScreen(_THIS)
{
  	if(currently_fullscreen)
	{
		if(this->hidden->dbuffer)
		{
			extern struct MsgPort *safeport,*dispport;

			this->hidden->dbuffer=0;

			if(safeport)
			{
				while(GetMsg(safeport)!=NULL);
				DeleteMsgPort(safeport);
			}
			if(dispport)
			{
				while(GetMsg(dispport)!=NULL);
				DeleteMsgPort(dispport);
			}

			this->hidden->SB[0]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort=this->hidden->SB[0]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort=NULL;
			this->hidden->SB[1]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort=this->hidden->SB[1]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort=NULL;

			if(this->hidden->SB[1])
				FreeScreenBuffer(SDL_Display,this->hidden->SB[1]);
			if(this->hidden->SB[0])
				FreeScreenBuffer(SDL_Display,this->hidden->SB[0]);


			this->hidden->SB[0]=this->hidden->SB[1]=NULL;

			if(SDL_RastPort && SDL_RastPort != &SDL_Display->RastPort)
				SDL_free(SDL_RastPort);

			SDL_RastPort=NULL;
		}
		CloseScreen(GFX_Display);
		currently_fullscreen=0;
	}
	else if(GFX_Display)
		UnlockPubScreen(NULL,GFX_Display);

	GFX_Display = NULL;
}

static int CGX_Available(void)
{
	struct Library *l;

	l = OpenLibrary("cybergraphics.library",0L);

	if ( l != NULL ) {
		D(bug("CGX video device AVAILABLE\n"));
		CloseLibrary(l);
	}
	D(else bug("**CGX video device UNAVAILABLE\n"));

	return(l != NULL);
}

static void CGX_DeleteDevice(SDL_VideoDevice *device)
{
	if ( device ) {
		if ( device->hidden ) {
			SDL_free(device->hidden);
		}
		if ( device->gl_data ) {
			SDL_free(device->gl_data);
		}
		SDL_free(device);
	}
}

static SDL_VideoDevice *CGX_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *device->hidden));
		device->gl_data = (struct SDL_PrivateGLData *)
				SDL_malloc((sizeof *device->gl_data));
	}
	if ( (device == NULL) || (device->hidden == NULL) ||
	                         (device->gl_data == NULL) ) {
		D(bug("Unable to create video device!\n"));
		SDL_OutOfMemory();
		CGX_DeleteDevice(device);
		return(0);
	}
	SDL_memset(device->hidden, 0, sizeof(*device->hidden));
	SDL_memset(device->gl_data, 0, sizeof(*device->gl_data));

	/* Set the driver flags */
	device->handles_any_size = 1;

	/* Set the function pointers */
	device->VideoInit = CGX_VideoInit;
	device->ListModes = CGX_ListModes;
	device->SetVideoMode = CGX_SetVideoMode;
	device->ToggleFullScreen = CGX_ToggleFullScreen;
	device->UpdateMouse = CGX_UpdateMouse;
	device->SetColors = CGX_SetColors;
	device->UpdateRects = NULL;
	device->VideoQuit = CGX_VideoQuit;
	device->AllocHWSurface = CGX_AllocHWSurface;
	device->CheckHWBlit = CGX_CheckHWBlit;
	device->FillHWRect = CGX_FillHWRect;
	device->SetHWColorKey = CGX_SetHWColorKey;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = CGX_LockHWSurface;
	device->UnlockHWSurface = CGX_UnlockHWSurface;
	device->FlipHWSurface = CGX_FlipHWSurface;
	device->FreeHWSurface = CGX_FreeHWSurface;
	device->SetGamma = CGX_SetGamma;
	device->GetGamma = CGX_GetGamma;
	device->SetGammaRamp = CGX_SetGammaRamp;
	device->GetGammaRamp = NULL;
#if SDL_VIDEO_OPENGL
	device->GL_LoadLibrary = CGX_GL_LoadLibrary;
	device->GL_GetProcAddress = CGX_GL_GetProcAddress;
	device->GL_GetAttribute = CGX_GL_GetAttribute;
	device->GL_MakeCurrent = CGX_GL_MakeCurrent;
	device->GL_SwapBuffers = CGX_GL_SwapBuffers;
#endif
	device->SetIcon = CGX_SetIcon;
	device->SetCaption = CGX_SetCaption;
	device->IconifyWindow = NULL; /* CGX_IconifyWindow; */
	device->GrabInput = NULL /* CGX_GrabInput*/;
	device->GetWMInfo = CGX_GetWMInfo;
	device->FreeWMCursor = amiga_FreeWMCursor;
	device->CreateWMCursor = amiga_CreateWMCursor;
	device->ShowWMCursor = amiga_ShowWMCursor;
	device->WarpWMCursor = amiga_WarpWMCursor;
	device->CheckMouseMode = amiga_CheckMouseMode;
	device->InitOSKeymap = amiga_InitOSKeymap;
	device->PumpEvents = amiga_PumpEvents;

	device->free = CGX_DeleteDevice;

	return device;
}

VideoBootStrap CGX_bootstrap = {
	"CGX", "AmigaOS CyberGraphics", CGX_Available, CGX_CreateDevice
};

Uint32 MakeBitMask(_THIS,int type,int format,int *bpp)
{
	D(if(type==0)bug("REAL pixel format: "));

	if(this->hidden->depth==*bpp)
	{

	switch(format)
    	{
		case PIXFMT_LUT8:
			D(if(type==0)bug("LUT8\n"));
			return 0;
		case PIXFMT_BGR15:
		case PIXFMT_RGB15PC:
			switch(type)
			{
				case 0:
					D(bug("RGB15PC/BGR15\n"));
					return 31;
				case 1:
					return 992;
				case 2:
					return 31744;
			}
		case PIXFMT_RGB15:
		case PIXFMT_BGR15PC:
			switch(type)
			{
				case 0:
					D(bug("RGB15/BGR15PC\n"));
					return 31744;
				case 1:
					return 992;
				case 2:
					return 31;
			}
		case PIXFMT_BGR16PC:
		case PIXFMT_RGB16:
			switch(type)
			{
				case 0:
					D(bug("RGB16PC\n"));
					return 63488;
				case 1:
					return 2016;
				case 2:
					return 31;
			}
		case PIXFMT_BGR16:
		case PIXFMT_RGB16PC:
			switch(type)
			{
				case 0:
					D(bug("RGB16PC/BGR16\n"));
					return 31;
				case 1:
					return 2016;
				case 2:
					return 63488;
			}

		case PIXFMT_RGB24:
			switch(type)
			{
				case 0:
					D(bug("RGB24/BGR24\n"));
					return 0xff0000;
				case 1:
					return 0xff00;
				case 2:
					return 0xff;
			}
		case PIXFMT_BGR24:
			switch(type)
			{
				case 0:
					D(bug("BGR24\n"));
					return 0xff;
				case 1:
					return 0xff00;
				case 2:
					return 0xff0000;
			}
		case PIXFMT_ARGB32:
			switch(type)
			{
				case 0:
					D(bug("ARGB32\n"));
					return 0xff0000;
				case 1:
					return 0xff00;
				case 2:
					return 0xff;
			}
		case PIXFMT_BGRA32:
			switch(type)
			{
				case 0:
					D(bug("BGRA32\n"));
					return 0xff00;
				case 1:
					return 0xff0000;
				case 2:
					return 0xff000000;
			}
		case PIXFMT_RGBA32:
			switch(type)
			{
				case 0:
					D(bug("RGBA32\n"));
					return 0xff000000;
				case 1:
					return 0xff0000;
				case 2:
					return 0xff00;
			}
		default:
			D(bug("Unknown pixel format! Default to 24bit\n"));
			return (Uint32) (255<<(type*8));
	}
	}
	else
	{
		D(if(type==0)bug("DIFFERENT from screen.\nAllocated screen format: "));

		switch(*bpp)
		{
			case 32:
				D(if(type==0) bug("RGBA32\n"));
				switch(type)
				{
					case 0:
						return 0xff000000;
					case 1:
						return 0xff0000;
					case 2:
						return 0xff00;
				}
				break;
			case 24:
use_truecolor:
				switch(type)
				{
					case 0:
						D(bug("RGB24\n"));
						return 0xff0000;
					case 1:
						return 0xff00;
					case 2:
						return 0xff;
				}
			case 16:
			case 15:
				D(if(type==0) bug("Not supported, switching to 24bit!\n"));
				*bpp=24;
				goto use_truecolor;
				break;
			default:
				D(if(type==0)bug("This is a chunky display\n"));
// For chunky display mask is always 0;
				return 0;
		}
	}
	return 0;
}

static int CGX_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	int i;
	struct Library *RTGBase;

	D(bug("VideoInit... Opening libraries\n"));

	if(!IntuitionBase) {
		if( !(IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39L))) {
			SDL_SetError("Couldn't open intuition V39+");
			return -1;
		}
	}

	if(!GfxBase) {
		if( !(GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39L))) {
			SDL_SetError("Couldn't open graphics V39+");
			return -1;
		}
	}

	if(!CyberGfxBase) {
		if( !(CyberGfxBase=OpenLibrary("cybergraphics.library",40L))) {
			SDL_SetError("Couldn't open cybergraphics.");
			return(-1);
		}
	}

	if(RTGBase=OpenLibrary("libs:picasso96/rtg.library",0L)) {
		extern int use_picasso96;

		CloseLibrary(RTGBase);
		use_picasso96=1;
	}

	D(bug("Library intialized, locking screen...\n"));

	SDL_Display = LockPubScreen(NULL);

	if ( SDL_Display == NULL ) {
		D(bug("Cannot lock display...\n"));
		SDL_SetError("Couldn't lock the display");
		return(-1);
	}
	this->info.current_w = SDL_Display->Width;
	this->info.current_h = SDL_Display->Height;

	D(bug("Checking if we are using a CGX native display...\n"));

	if(!IsCyberModeID(GetVPModeID(&SDL_Display->ViewPort)))
	{
		Uint32 okid=BestCModeIDTags(CYBRBIDTG_NominalWidth,SDL_Display->Width,
				CYBRBIDTG_NominalHeight,SDL_Display->Height,
				CYBRBIDTG_Depth,8,
				TAG_DONE);

		D(bug("Default visual is not CGX native!\n"));

		UnlockPubScreen(NULL,SDL_Display);

		GFX_Display=NULL;

		if(okid!=INVALID_ID)
		{
			GFX_Display=OpenScreenTags(NULL,
									SA_Width,SDL_Display->Width,
									SA_Height,SDL_Display->Height,
									SA_Depth,8,SA_Quiet,TRUE,
									SA_ShowTitle,FALSE,
									SA_DisplayID,okid,
									TAG_DONE);
		}

		if(!GFX_Display)
		{
			SDL_SetError("Unable to open a suited CGX display");
			return -1;
		}
		else SDL_Display=GFX_Display;

	}
	else GFX_Display = SDL_Display;


	/* See whether or not we need to swap pixels */

	swap_pixels = 0;

// Non e' detto che sia cosi' pero', alcune schede potrebbero gestire i modi in modo differente

	if ( SDL_BYTEORDER == SDL_LIL_ENDIAN ) {
		swap_pixels = 1;
	}

	D(bug("Before GetVideoModes....\n"));

	/* Get the available video modes */
	if(CGX_GetVideoModes(this) < 0)
	    return -1;

	/* Determine the default screen depth:
	   Use the default visual (or at least one with the same depth) */

	for(i = 0; i < this->hidden->nvisuals; i++)
	    if(this->hidden->visuals[i].depth == GetCyberMapAttr(SDL_Display->RastPort.BitMap,CYBRMATTR_DEPTH))
					break;
	if(i == this->hidden->nvisuals) {
	    /* default visual was useless, take the deepest one instead */
	    i = 0;
	}
	SDL_Visual = this->hidden->visuals[i].visual;

//	SDL_XColorMap = SDL_DisplayColormap;

	this->hidden->depth = this->hidden->visuals[i].depth;
	D(bug("Init: Setting screen depth to: %ld\n",this->hidden->depth));
	vformat->BitsPerPixel = this->hidden->visuals[i].depth; /* this->hidden->visuals[i].bpp; */

	{
		int form;
		APTR handle;
		struct DisplayInfo info;

		if(!(handle=FindDisplayInfo(this->hidden->visuals[i].visual)))
		{
			D(bug("Unable to get visual info...\n"));
			return -1;
		}

		if(!GetDisplayInfoData(handle,(char *)&info,sizeof(struct DisplayInfo),DTAG_DISP,NULL)) {
			D(bug("Unable to get visual info data...\n"));
			return -1;
		}

		form=GetCyberIDAttr(CYBRIDATTR_PIXFMT,SDL_Visual);

// In this case I use makebitmask in a way that I'm sure I'll get PIXFMT pixel mask

		if ( vformat->BitsPerPixel > 8 )
		{
			vformat->Rmask = MakeBitMask(this,0,form,&this->hidden->depth);
	  		vformat->Gmask = MakeBitMask(this,1,form,&this->hidden->depth);
		  	vformat->Bmask = MakeBitMask(this,2,form,&this->hidden->depth);
		}
	}

	/* See if we have been passed a window to use */
/*	SDL_windowid = SDL_getenv("SDL_WINDOWID"); */
	SDL_windowid=NULL;

	/* Create the blank cursor */
	SDL_BlankCursor = AllocMem(16,MEMF_CHIP|MEMF_CLEAR);

	/* Fill in some window manager capabilities */
	this->info.wm_available = 1;
	this->info.blit_hw = 1;
	this->info.blit_hw_CC = 1;
	this->info.blit_sw = 1;
	this->info.blit_fill = 1;
	this->info.video_mem=2000000; // Not always true but almost any Amiga card has this memory!

	this->hidden->same_format=0;
	SDL_RastPort=&SDL_Display->RastPort;
	/* We're done! */
	D(bug("End of CGX_VideoInit\n"));

	return(0);
}

void CGX_DestroyWindow(_THIS, SDL_Surface *screen)
{
	D(bug("Destroy Window...\n"));

	if ( ! SDL_windowid ) {
		/* Hide the managed window */
		int was_fullscreen=0;

		/* Clean up OpenGL */
		if ( screen ) {
		screen->flags &= ~(SDL_OPENGL|SDL_OPENGLBLIT);
		}

		if ( screen && (screen->flags & SDL_FULLSCREEN) ) {
			was_fullscreen=1;
			screen->flags &= ~SDL_FULLSCREEN;
//			CGX_LeaveFullScreen(this); tolto x crash
		}

		/* Destroy the output window */
		if ( SDL_Window ) {
			CloseWindow(SDL_Window);
			SDL_Window=NULL;
		}

		/* Free the colormap entries */
		if ( SDL_XPixels ) {
			int numcolors;
			unsigned long pixel;

			if(this->screen->format&&this->hidden->depth==8&&!was_fullscreen)
			{
				numcolors = 1<<this->screen->format->BitsPerPixel;

				if(numcolors>256)
					numcolors=256;

				if(!was_fullscreen&&this->hidden->depth==8)
				{
					for ( pixel=0; pixel<numcolors; pixel++ )
					{
						if(SDL_XPixels[pixel]>=0)
							ReleasePen(GFX_Display->ViewPort.ColorMap,SDL_XPixels[pixel]);
					}
				}
			}
			SDL_free(SDL_XPixels);
			SDL_XPixels = NULL;
		}
	}
}

static void CGX_SetSizeHints(_THIS, int w, int h, Uint32 flags)
{
	if ( flags & SDL_RESIZABLE ) {
		WindowLimits(SDL_Window, 32, 32,4096,4096);
	} else {
		WindowLimits(SDL_Window, w,h,w,h);
	}
	if ( flags & SDL_FULLSCREEN ) {
		flags&=~SDL_RESIZABLE;
	} else if ( SDL_getenv("SDL_VIDEO_CENTERED") ) {
		int display_w, display_h;

		display_w = SDL_Display->Width;
		display_h = SDL_Display->Height;
		ChangeWindowBox(SDL_Window,(display_w - w - SDL_Window->BorderLeft-SDL_Window->BorderRight)/2,
					(display_h - h - SDL_Window->BorderTop-SDL_Window->BorderBottom)/2,
					w+SDL_Window->BorderLeft+SDL_Window->BorderRight,
					h+SDL_Window->BorderTop+SDL_Window->BorderBottom);
	}
}

int CGX_CreateWindow(_THIS, SDL_Surface *screen,
			    int w, int h, int bpp, Uint32 flags)
{
#if 0
	int i, depth;
	Uint32 vis;
#endif
	D(bug("CGX_CreateWindow\n"));

	/* If a window is already present, destroy it and start fresh */
	if ( SDL_Window ) {
		CGX_DestroyWindow(this, screen);
	}

	/* See if we have been given a window id */
	if ( SDL_windowid ) {
		SDL_Window = (struct Window *)atol(SDL_windowid);
	} else {
		SDL_Window = 0;
	}

	/* find out which visual we are going to use */
#if 0
/* questo l'ho spostato nell'apertura dello schermo, in quanto su Amiga le finestre
   hanno il pixel mode degli schermi.
 */
	/*if ( flags & SDL_OPENGL ) {
		SDL_SetError("OpenGL not supported by the Amiga SDL!");
		return -1;
	}
	else {*/
		for ( i = 0; i < this->hidden->nvisuals; i++ ) {
			if ( this->hidden->visuals[i].depth == bpp ) /* era .depth */
				break;
		}
		if ( i == this->hidden->nvisuals ) {
			SDL_SetError("No matching visual for requested depth");
			return -1;	/* should never happen */
		}
		vis = this->hidden->visuals[i].visual;
		depth = this->hidden->visuals[i].depth;
//	}
	SDL_Visual = vis;
	this->hidden->depth = depth;
	D(bug("Setting screen depth to: %ld\n",this->hidden->depth));
#endif

	/* Allocate the new pixel format for this video mode */
	{
		Uint32 form;
		APTR handle;
		struct DisplayInfo info;

		if(!(handle=FindDisplayInfo(SDL_Visual)))
			return -1;

		if(!GetDisplayInfoData(handle,(char *)&info,sizeof(struct DisplayInfo),DTAG_DISP,NULL))
			return -1;

		form=GetCyberIDAttr(CYBRIDATTR_PIXFMT,SDL_Visual);

		if(flags&SDL_HWSURFACE)
		{
			if(bpp!=this->hidden->depth)
			{
				bpp=this->hidden->depth;
				D(bug("Accel forces bpp to be equal (%ld)\n",bpp));
			}
		}

		D(bug("BEFORE screen allocation: bpp:%ld (real:%ld)\n",bpp,this->hidden->depth));

/* With this call if needed I'll revert the wanted bpp to a bpp best suited for the display, actually occurs
   only with requested format 15/16bit and display format != 15/16bit
 */

		if ( ! SDL_ReallocFormat(screen, bpp,
				MakeBitMask(this,0,form,&bpp), MakeBitMask(this,1,form,&bpp), MakeBitMask(this,2,form,&bpp), 0) )
			return -1;

		D(bug("AFTER screen allocation: bpp:%ld (real:%ld)\n",bpp,this->hidden->depth));

	}

	/* Create the appropriate colormap */
/*
	if ( SDL_XColorMap != SDL_DisplayColormap ) {
		XFreeColormap(SDL_Display, SDL_XColorMap);
	}
*/
	if ( GetCyberMapAttr(SDL_Display->RastPort.BitMap,CYBRMATTR_PIXFMT)==PIXFMT_LUT8 || bpp==8 ) {
	    int ncolors,i;
	    D(bug("XPixels palette allocation...\n"));

	    /* Allocate the pixel flags */

	    if(bpp==8)
		ncolors=256;
	    else
		ncolors = 1 << screen->format->BitsPerPixel;

	    SDL_XPixels = (Sint32 *)SDL_malloc(ncolors * sizeof(Sint32));

	    if(SDL_XPixels == NULL) {
		SDL_OutOfMemory();
		return -1;
	    }


	    for(i=0;i<ncolors;i++)
		    SDL_XPixels[i]=-1;

	    /* always allocate a private colormap on non-default visuals */
	    if(bpp==8)
		flags |= SDL_HWPALETTE;

	    if ( flags & SDL_HWPALETTE )
			screen->flags |= SDL_HWPALETTE;
	} 

	/* resize the (possibly new) window manager window */

	/* Create (or use) the X11 display window */

	if ( !SDL_windowid ) {
			if( flags & SDL_FULLSCREEN )
			{
				SDL_Window = OpenWindowTags(NULL,WA_Width,w,WA_Height,h,
											WA_Flags,WFLG_ACTIVATE|WFLG_RMBTRAP|WFLG_BORDERLESS|WFLG_BACKDROP|WFLG_REPORTMOUSE,
											WA_IDCMP,IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE,
											WA_CustomScreen,(ULONG)SDL_Display,
											TAG_DONE);

				D(bug("Opening backdrop window %ldx%ld on display %lx!\n",w,h,SDL_Display));
			}
			else
			{
				/* Create GimmeZeroZero window when OpenGL is used */
				unsigned long gzz = FALSE;
				if( flags & SDL_OPENGL ) {
					gzz = TRUE;
				}

				SDL_Window = OpenWindowTags(NULL,WA_InnerWidth,w,WA_InnerHeight,h,
											WA_Flags,WFLG_REPORTMOUSE|WFLG_ACTIVATE|WFLG_RMBTRAP | ((flags&SDL_NOFRAME) ? 0 : (WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_DRAGBAR | ((flags&SDL_RESIZABLE) ? WFLG_SIZEGADGET|WFLG_SIZEBBOTTOM : 0))),
											WA_IDCMP,IDCMP_RAWKEY|IDCMP_CLOSEWINDOW|IDCMP_MOUSEBUTTONS|IDCMP_NEWSIZE|IDCMP_MOUSEMOVE,
											WA_PubScreen,(ULONG)SDL_Display,
											WA_GimmeZeroZero, gzz,
														TAG_DONE);
				D(bug("Opening WB window of size: %ldx%ld!\n",w,h));
			}

		if(!SDL_Window)
			return -1;
	}

	this->hidden->BytesPerPixel=GetCyberMapAttr(SDL_Window->RPort->BitMap,CYBRMATTR_BPPIX);

	if(screen->flags & SDL_DOUBLEBUF)
	{
		if(SDL_RastPort=SDL_malloc(sizeof(struct RastPort)))
		{
			InitRastPort(SDL_RastPort);
			SDL_RastPort->BitMap=this->hidden->SB[1]->sb_BitMap;
		}
		else
			return -1;
	}
	else SDL_RastPort=SDL_Window->RPort;

	if(flags&SDL_HWSURFACE)
		screen->flags|=SDL_HWSURFACE;

	if( !SDL_windowid ) {
	    CGX_SetSizeHints(this, w, h, flags);
	}

	/* Set our colormaps when not setting a GL mode */
/*
	if ( ! (flags & SDL_OPENGL) ) {
		XSetWindowColormap(SDL_Display, SDL_Window, SDL_XColorMap);
	}
*/

	/* Map them both and go fullscreen, if requested */
	if ( ! SDL_windowid ) {
		if ( flags & SDL_FULLSCREEN ) {
			screen->flags |= SDL_FULLSCREEN;
			currently_fullscreen=1;
//			CGX_EnterFullScreen(this); Ci siamo gia'!
		} else {
			screen->flags &= ~SDL_FULLSCREEN;
		}
	}
	screen->w = w;
	screen->h = h;
	screen->pitch = SDL_CalculatePitch(screen);
	CGX_ResizeImage(this, screen, flags);

	/* Make OpenGL Context if needed*/
	if(flags & SDL_OPENGL) {
		if(this->gl_data->gl_active == 0) {
			if(CGX_GL_Init(this) < 0)
				return -1;
			else
				screen->flags |= SDL_OPENGL;
		}
		else {
			if(CGX_GL_Update(this) < 0)
				return -1;
			else
				screen->flags |= SDL_OPENGL;
		}
	}
}

int CGX_ResizeWindow(_THIS,
			SDL_Surface *screen, int w, int h, Uint32 flags)
{
	D(bug("CGX_ResizeWindow\n"));

	if ( ! SDL_windowid ) {
		/* Resize the window manager window */
		CGX_SetSizeHints(this, w, h, flags);

		ChangeWindowBox(SDL_Window,SDL_Window->LeftEdge,SDL_Window->TopEdge, w+SDL_Window->BorderLeft+SDL_Window->BorderRight,
					h+SDL_Window->BorderTop+SDL_Window->BorderBottom);

		screen->w = w;
		screen->h = h;
		screen->pitch = SDL_CalculatePitch(screen);
		CGX_ResizeImage(this, screen, flags);
	}
	return(0);
}

static SDL_Surface *CGX_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	Uint32 saved_flags;
	int needcreate=0;

	D(bug("CGX_SetVideoMode current:%lx\n",current));

	/* Lock the event thread, in multi-threading environments */
	SDL_Lock_EventThread();

// Check if the window needs to be closed or can be resized

	if( (flags&SDL_FULLSCREEN) || (current && current->flags&SDL_FULLSCREEN && !(flags&SDL_FULLSCREEN)))
		needcreate=1;

// Check if we need to close an already existing videomode...

	if(current && current->flags&SDL_FULLSCREEN && !(flags&SDL_FULLSCREEN)) {
		unsigned long i;
		D(bug("Destroying image, window & screen!\n"));

		CGX_DestroyImage(this,current);
		CGX_DestroyWindow(this,current);
		DestroyScreen(this);
		GFX_Display=SDL_Display=LockPubScreen(NULL);

		bpp=this->hidden->depth=GetCyberMapAttr(SDL_Display->RastPort.BitMap,CYBRMATTR_DEPTH);

		for ( i = 0; i < this->hidden->nvisuals; i++ ) {
			if ( this->hidden->visuals[i].depth == bpp ) /* era .depth */
				break;
		}
		if ( i == this->hidden->nvisuals ) {
			SDL_SetError("No matching visual for requested depth");
			return NULL;	/* should never happen */
		}
		SDL_Visual = this->hidden->visuals[i].visual;

		D(bug("Setting screen depth to: %ld\n",this->hidden->depth));

	}
	/* Check the combination of flags we were passed */
	if ( flags & SDL_FULLSCREEN ) {
		int i;

		/* Clear fullscreen flag if not supported */
		if ( SDL_windowid ) {
			flags &= ~SDL_FULLSCREEN;
		}
		else if(current && current->flags&SDL_FULLSCREEN ) {
			if(current->w!=width ||
				current->h!=height ||
				(this->hidden && this->hidden->depth!=bpp))
			{
				D(bug("Deleting previous window...\n"));
				CGX_DestroyImage(this,current);
				CGX_DestroyWindow(this,current);
				DestroyScreen(this);
				goto buildnewscreen;
			}
		}
		else
buildnewscreen:
		{
			Uint32 okid=BestCModeIDTags(CYBRBIDTG_NominalWidth,width,
				CYBRBIDTG_NominalHeight,height,
				CYBRBIDTG_Depth,bpp,
				TAG_DONE);

			GFX_Display=NULL;

			D(bug("Opening screen...\n"));

			if(okid!=INVALID_ID)
				GFX_Display=OpenScreenTags(NULL,
								SA_Width,width,
								SA_Height,height,
								SA_Quiet,TRUE,SA_ShowTitle,FALSE,
								SA_Depth,bpp,
								SA_DisplayID,okid,
								TAG_DONE);

			if(!GFX_Display) {
				GFX_Display=SDL_Display;
				flags &= ~SDL_FULLSCREEN;
				flags &= ~SDL_DOUBLEBUF;
			}
			else {
				UnlockPubScreen(NULL,SDL_Display);
				SDL_Display=GFX_Display;
	
				D(bug("Screen opened.\n"));

				if(flags&SDL_DOUBLEBUF) {
					int ok=0;
					D(bug("Start of DBuffering allocations...\n"));

					if(this->hidden->SB[0]=AllocScreenBuffer(SDL_Display,NULL,SB_SCREEN_BITMAP)) {

						if(this->hidden->SB[1]=AllocScreenBuffer(SDL_Display,NULL,0L)) {
							extern struct MsgPort *safeport,*dispport;

							safeport=CreateMsgPort();
							dispport=CreateMsgPort();

							if(!safeport || !dispport) {
								if(safeport) {
									DeleteMsgPort(safeport);
									safeport=NULL;
								}
								if(dispport) {
									DeleteMsgPort(dispport);
									dispport=NULL;
								}
								FreeScreenBuffer(SDL_Display,this->hidden->SB[0]);
								FreeScreenBuffer(SDL_Display,this->hidden->SB[1]);
							}
							else {
								extern ULONG safe_sigbit,disp_sigbit;
								int i;

								safe_sigbit=1L<< safeport->mp_SigBit;
								disp_sigbit=1L<< dispport->mp_SigBit;

								for(i=0;i<2;i++) {
									this->hidden->SB[i]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort=safeport;
									this->hidden->SB[i]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort=dispport;
								}

								ok=1;
								D(bug("Dbuffering enabled!\n"));
								this->hidden->dbuffer=1;
								current->flags|=SDL_DOUBLEBUF;
							}
						}
						else {
							FreeScreenBuffer(SDL_Display,this->hidden->SB[1]);
							this->hidden->SB[0]=NULL;
						}
					}

					if(!ok)
						flags&=~SDL_DOUBLEBUF;
				}
			}

			if(GetCyberMapAttr(SDL_Display->RastPort.BitMap,CYBRMATTR_DEPTH)==bpp)
				this->hidden->same_format=1;
		}

		bpp=this->hidden->depth=GetCyberMapAttr(SDL_Display->RastPort.BitMap,CYBRMATTR_DEPTH);
		D(bug("Setting screen depth to: %ld\n",this->hidden->depth));

		for ( i = 0; i < this->hidden->nvisuals; i++ )
			if ( this->hidden->visuals[i].depth == bpp ) /* era .depth */
				break;

		if ( i == this->hidden->nvisuals ) {
			SDL_SetError("No matching visual for requested depth");
			return NULL;	/* should never happen */
		}
		SDL_Visual = this->hidden->visuals[i].visual;

	}

	/* Set up the X11 window */
	saved_flags = current->flags;

	if (SDL_Window && (saved_flags&SDL_OPENGL) == (flags&SDL_OPENGL)
	    && bpp == current->format->BitsPerPixel && !needcreate) {
		if (CGX_ResizeWindow(this, current, width, height, flags) < 0) {
			current = NULL;
			goto done;
		}
	} else {
		if (CGX_CreateWindow(this,current,width,height,bpp,flags) < 0) {
			current = NULL;
			goto done;
		}
	}

#if 0
	/* Set up the new mode framebuffer */
	if ( ((current->w != width) || (current->h != height)) ||
             ((saved_flags&SDL_OPENGL) != (flags&SDL_OPENGL)) ) {
		current->w = width;
		current->h = height;
		current->pitch = SDL_CalculatePitch(current);
		CGX_ResizeImage(this, current, flags);
	}
#endif

	current->flags |= (flags&SDL_RESIZABLE); // Resizable only if the user asked it

  done:
	/* Release the event thread */
	SDL_Unlock_EventThread();

	/* We're done! */
	return(current);
}

static int CGX_ToggleFullScreen(_THIS, int on)
{
	Uint32 event_thread;

	/* Don't switch if we don't own the window */
	if ( SDL_windowid ) {
		return(0);
	}

	/* Don't lock if we are the event thread */
	event_thread = SDL_EventThreadID();
	if ( event_thread && (SDL_ThreadID() == event_thread) ) {
		event_thread = 0;
	}
	if ( event_thread ) {
		SDL_Lock_EventThread();
	}
	if ( on ) {
		this->screen->flags |= SDL_FULLSCREEN;
		CGX_EnterFullScreen(this);
	} else {
		this->screen->flags &= ~SDL_FULLSCREEN;
		CGX_LeaveFullScreen(this);
	}

	CGX_RefreshDisplay(this);
	if ( event_thread ) {
		SDL_Unlock_EventThread();
	}

	SDL_ResetKeyboard();

	return(1);
}

static void SetSingleColor(Uint32 fmt, unsigned char r, unsigned char g, unsigned char b, unsigned char *c)
{
	switch(fmt)
	{
		case PIXFMT_BGR15:
		case PIXFMT_RGB15PC:
			{
				Uint16 *t=(Uint16 *)c;
				*t=(r>>3) | ((g>>3)<<5) | ((b>>3)<<10) ;
			}
			break;
		case PIXFMT_RGB15:
		case PIXFMT_BGR15PC:
			{
				Uint16 *t=(Uint16 *)c;
				*t=(b>>3) | ((g>>3)<<5) | ((r>>3)<<10) ;
			}
			break;
		case PIXFMT_BGR16PC:
		case PIXFMT_RGB16:
			{
				Uint16 *t=(Uint16 *)c;
				*t=(b>>3) | ((g>>2)<<5) | ((r>>3)<<11) ;
			}
			break;
		case PIXFMT_BGR16:
		case PIXFMT_RGB16PC:
			{
				Uint16 *t=(Uint16 *)c;
				*t=(r>>3) | ((g>>2)<<5) | ((b>>3)<<11) ;
			}
			break;
		case PIXFMT_RGB24:
			c[0]=r;
			c[1]=g;
			c[2]=b;
			c[3]=0;
			break;
		case PIXFMT_BGR24:
			c[0]=b;
			c[1]=g;
			c[2]=r;
			c[3]=0;
			break;
		case PIXFMT_ARGB32:
			c[0]=0;
			c[1]=r;
			c[2]=g;
			c[3]=b;
			break;
		case PIXFMT_BGRA32:
			c[0]=b;
			c[1]=g;
			c[2]=r;
			c[3]=0;
			break;
		case PIXFMT_RGBA32:
			c[0]=r;
			c[1]=g;
			c[2]=b;
			c[3]=0;
			break;

		default:
			D(bug("Error, SetSingleColor with PIXFMT %ld!\n",fmt));
	}
}

/* Update the current mouse state and position */
static void CGX_UpdateMouse(_THIS)
{
	/* Lock the event thread, in multi-threading environments */
	SDL_Lock_EventThread();

	if(currently_fullscreen)
	{
			SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
			SDL_PrivateMouseMotion(0, 0, SDL_Display->MouseX, SDL_Display->MouseY);
	}
	else
	{
		if(	SDL_Display->MouseX>=(SDL_Window->LeftEdge+SDL_Window->BorderLeft) && SDL_Display->MouseX<(SDL_Window->LeftEdge+SDL_Window->Width-SDL_Window->BorderRight) &&
			SDL_Display->MouseY>=(SDL_Window->TopEdge+SDL_Window->BorderLeft) && SDL_Display->MouseY<(SDL_Window->TopEdge+SDL_Window->Height-SDL_Window->BorderBottom)
			)
		{
			SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
			SDL_PrivateMouseMotion(0, 0, SDL_Display->MouseX-SDL_Window->LeftEdge-SDL_Window->BorderLeft,
										SDL_Display->MouseY-SDL_Window->TopEdge-SDL_Window->BorderTop);
		}
		else
		{
			SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
		}
	}
	SDL_Unlock_EventThread();
}

static int CGX_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	int      i;

	/* Check to make sure we have a colormap allocated */

	/* It's easy if we have a hidden colormap */
	if ( (this->screen->flags & SDL_HWPALETTE) && currently_fullscreen )
	{
		ULONG  xcmap[256*3+2];

		xcmap[0]=(ncolors<<16);
		xcmap[0]+=firstcolor;

//		D(bug("Setting %ld colors on an HWPALETTE screen\n",ncolors));

		for ( i=0; i<ncolors; i++ ) {
			xcmap[i*3+1] = colors[i+firstcolor].r<<24;
			xcmap[i*3+2] = colors[i+firstcolor].g<<24;
			xcmap[i*3+3] = colors[i+firstcolor].b<<24;
		}
		xcmap[ncolors*3+1]=0;
		LoadRGB32(&GFX_Display->ViewPort,xcmap);
	} else {
// XPixels are not needed on 8bit screen with hwpalette
		unsigned long pixel;

		if ( SDL_XPixels == NULL ) {
			D(bug("SetColors without colormap!"));
			return(0);
		}

		if(this->hidden->depth==8)
		{
// In this case I have to unalloc and realloc the full palette
			D(bug("Obtaining %ld colors on the screen\n",ncolors));

		/* Free existing allocated colors */
			for ( pixel=0; pixel<this->screen->format->palette->ncolors; ++pixel ) {
				if(SDL_XPixels[pixel]>=0)
					ReleasePen(GFX_Display->ViewPort.ColorMap,SDL_XPixels[pixel]);
			}

		/* Try to allocate all the colors */
			for ( i=0; i<this->screen->format->palette->ncolors; ++i ) {
				SDL_XPixels[i]=ObtainBestPenA(GFX_Display->ViewPort.ColorMap,colors[i].r<<24,colors[i].g<<24,colors[i].b<<24,NULL);
			}
		}
		else
		{
#ifndef USE_CGX_WRITELUTPIXEL
			Uint32 fmt;
			D(bug("Preparing a conversion pixel table...\n"));

			fmt=GetCyberMapAttr(SDL_Display->RastPort.BitMap,CYBRMATTR_PIXFMT);

			for(i=0;i<ncolors;i++)
			{
				SetSingleColor(fmt,colors[firstcolor+i].r,colors[firstcolor+i].g,colors[firstcolor+i].b,(unsigned char *)&SDL_XPixels[firstcolor+i]);
			}
#else
//			D(bug("Executing XPixel(%lx) remapping: (from %ld, %ld colors) first: r%ld g%ld b%ld\n",SDL_XPixels,firstcolor,ncolors,colors[firstcolor].r,colors[firstcolor].g,colors[firstcolor].b));
			for(i=0;i<ncolors;i++)
				SDL_XPixels[i+firstcolor]=(colors[firstcolor+i].r<<16)+(colors[firstcolor+i].g<<8)+colors[firstcolor+i].b;
#endif
		}
	}

// Actually it cannot fail!

	return 1;
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
static void CGX_VideoQuit(_THIS)
{
	/* Shutdown everything that's still up */
	/* The event thread should be done, so we can touch SDL_Display */
	D(bug("CGX_VideoQuit\n"));

	if ( SDL_Display != NULL ) {
		/* Clean up OpenGL */
		if(this->gl_data->gl_active == 1) {
			CGX_GL_Quit(this);
		}
		/* Start shutting down the windows */
		D(bug("Destroying image...\n"));
		CGX_DestroyImage(this, this->screen);
		D(bug("Destroying window...\n"));
		CGX_DestroyWindow(this, this->screen);
// Otherwise SDL_VideoQuit will try to free it!
		SDL_VideoSurface=NULL;

		CGX_FreeVideoModes(this);

		/* Free that blank cursor */
		if ( SDL_BlankCursor != NULL ) {
			FreeMem(SDL_BlankCursor,16);
			SDL_BlankCursor = NULL;
		}

		/* Close the X11 graphics connection */
		this->hidden->same_format=0;

		D(bug("Destroying screen...\n"));

		if ( GFX_Display != NULL )
			DestroyScreen(this);

		/* Close the X11 display connection */
		SDL_Display = NULL;

		/* Unload GL library after X11 shuts down */
	}

	D(bug("Closing libraries...\n"));

	if( CyberGfxBase) {
		CloseLibrary(CyberGfxBase);
		CyberGfxBase=NULL;
	}

	if (IntuitionBase) {
		CloseLibrary((struct Library *)IntuitionBase);
		IntuitionBase=NULL;
	}
	if (GfxBase) {
		CloseLibrary((struct Library *)GfxBase);
		GfxBase=NULL;
	}

	if ( this->screen && (this->screen->flags & SDL_HWSURFACE) ) {
		/* Direct screen access, no memory buffer */
		this->screen->pixels = NULL;
	}
	D(bug("End of CGX_VideoQuit.\n"));

}

