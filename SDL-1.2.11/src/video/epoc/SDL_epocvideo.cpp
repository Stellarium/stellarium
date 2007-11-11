/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/*
    SDL_epocvideo.cpp
    Epoc based SDL video driver implementation

    Epoc version by Hannu Viitala (hannu.j.viitala@mbnet.fi)
*/

extern "C" {
#include "SDL_timer.h"
#include "SDL_video.h"
#undef NULL
#include "../SDL_pixels_c.h"
};

#include "SDL_epocvideo.h"
#include "SDL_epocevents_c.h"

#include <hal.h>
#include <coedef.h>

/* For debugging */

void RDebug_Print_b(char* error_str, void* param)
    {
    TBuf8<128> error8((TUint8*)error_str);
    TBuf<128> error;
    error.Copy(error8);
    if (param) //!! Do not work if the parameter is really 0!!
        RDebug::Print(error, param);
    else 
        RDebug::Print(error);
    }

extern "C" void RDebug_Print(char* error_str, void* param)
    {
    RDebug_Print_b(error_str, param);
    }


int Debug_AvailMem2()
    {
    //User::CompressAllHeaps();
    TMemoryInfoV1Buf membuf; 
    User::LeaveIfError(UserHal::MemoryInfo(membuf));
    TMemoryInfoV1 minfo = membuf();
	return(minfo.iFreeRamInBytes);
    }

extern "C" int Debug_AvailMem()
    {
    return(Debug_AvailMem2());
    }


extern "C" {

/* Initialization/Query functions */

static int EPOC_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **EPOC_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *EPOC_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int EPOC_SetColors(_THIS, int firstcolor, int ncolors,
			  SDL_Color *colors);
static void EPOC_VideoQuit(_THIS);

/* Hardware surface functions */

static int EPOC_AllocHWSurface(_THIS, SDL_Surface *surface);
static int EPOC_LockHWSurface(_THIS, SDL_Surface *surface);
static int EPOC_FlipHWSurface(_THIS, SDL_Surface *surface);
static void EPOC_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void EPOC_FreeHWSurface(_THIS, SDL_Surface *surface);
static void EPOC_DirectUpdate(_THIS, int numrects, SDL_Rect *rects);

static int EPOC_Available(void);
static SDL_VideoDevice *EPOC_CreateDevice(int devindex);

/* Mouse functions */

static WMcursor *EPOC_CreateWMCursor(_THIS, Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);
static void EPOC_FreeWMCursor(_THIS, WMcursor *cursor);
static int EPOC_ShowWMCursor(_THIS, WMcursor *cursor);



/* !! Table for fast conversion from 8 bit to 12 bit */
static TUint16 EPOC_HWPalette_256_to_4k[256];

VideoBootStrap EPOC_bootstrap = {
	"epoc", "EPOC system",
    EPOC_Available, EPOC_CreateDevice
};

const TUint32 WindowClientHandle = 9210; //!!

/* Epoc video driver bootstrap functions */

static int EPOC_Available(void)
{
    return 1; /* Always available */
}

static void EPOC_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *EPOC_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Allocate all variables that we free on delete */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return(0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = EPOC_VideoInit;
	device->ListModes = EPOC_ListModes;
	device->SetVideoMode = EPOC_SetVideoMode;
	device->SetColors = EPOC_SetColors;
	device->UpdateRects = NULL;
	device->VideoQuit = EPOC_VideoQuit;
	device->AllocHWSurface = EPOC_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = EPOC_LockHWSurface;
	device->UnlockHWSurface = EPOC_UnlockHWSurface;
	device->FlipHWSurface = EPOC_FlipHWSurface;
	device->FreeHWSurface = EPOC_FreeHWSurface;
	device->SetIcon = NULL;
	device->SetCaption = NULL;
	device->GetWMInfo = NULL;
	device->FreeWMCursor = EPOC_FreeWMCursor;
	device->CreateWMCursor = EPOC_CreateWMCursor;
	device->ShowWMCursor = EPOC_ShowWMCursor;
	device->WarpWMCursor = NULL;
	device->InitOSKeymap = EPOC_InitOSKeymap;
	device->PumpEvents = EPOC_PumpEvents;
	device->free = EPOC_DeleteDevice;

	return device;
}


int GetBpp(TDisplayMode displaymode)
{
    TInt numColors = TDisplayModeUtils::NumDisplayModeColors(displaymode);
    TInt bitsPerPixel = 1;
    for (TInt32 i = 2; i < numColors; i <<= 1, bitsPerPixel++);
    return bitsPerPixel;    
}

void ConstructWindowL(_THIS)
{
	TInt	error;

	error = Private->EPOC_WsSession.Connect();
	User::LeaveIfError(error);
	Private->EPOC_WsScreen=new(ELeave) CWsScreenDevice(Private->EPOC_WsSession);
	User::LeaveIfError(Private->EPOC_WsScreen->Construct());
	User::LeaveIfError(Private->EPOC_WsScreen->CreateContext(Private->EPOC_WindowGc));

	Private->EPOC_WsWindowGroup=RWindowGroup(Private->EPOC_WsSession);
	User::LeaveIfError(Private->EPOC_WsWindowGroup.Construct(WindowClientHandle));
	Private->EPOC_WsWindowGroup.SetOrdinalPosition(0);

    //!!
    TBuf<32> winGroupName;
    winGroupName.Append(0);
    winGroupName.Append(0);
    winGroupName.Append(0);// uid
    winGroupName.Append(0);
    winGroupName.Append(_L("SDL")); // caption
    winGroupName.Append(0);
    winGroupName.Append(0); //doc name
	Private->EPOC_WsWindowGroup.SetName(winGroupName); //!!

	Private->EPOC_WsWindow=RWindow(Private->EPOC_WsSession);
	User::LeaveIfError(Private->EPOC_WsWindow.Construct(Private->EPOC_WsWindowGroup,WindowClientHandle));
	Private->EPOC_WsWindow.SetBackgroundColor(KRgbWhite);
    Private->EPOC_WsWindow.Activate();
	Private->EPOC_WsWindow.SetSize(Private->EPOC_WsScreen->SizeInPixels()); 
	Private->EPOC_WsWindow.SetVisible(ETrue);

    Private->EPOC_WsWindowGroupID = Private->EPOC_WsWindowGroup.Identifier();
    Private->EPOC_IsWindowFocused = EFalse;
}


int EPOC_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
    // !!TODO:handle leave functions!

    int i;

	/* Initialize all variables that we clean on shutdown */   

	for ( i=0; i<SDL_NUMMODES; ++i ) {
		Private->SDL_modelist[i] = (SDL_Rect *)SDL_malloc(sizeof(SDL_Rect));
		Private->SDL_modelist[i]->x = Private->SDL_modelist[i]->y = 0;
	}
	/* Modes sorted largest to smallest !!TODO:sorting order??*/
	Private->SDL_modelist[0]->w = 640; Private->SDL_modelist[0]->h = 200; 
	Private->SDL_modelist[1]->w = 320; Private->SDL_modelist[1]->h = 200;
	Private->SDL_modelist[2]->w = 640; Private->SDL_modelist[2]->h = 400; 
	Private->SDL_modelist[3]->w = 640; Private->SDL_modelist[3]->h = 480;
	Private->SDL_modelist[4] = NULL;

    /* Construct Epoc window */

    ConstructWindowL(_this);

    /* Initialise Epoc frame buffer */

    TDisplayMode displayMode = Private->EPOC_WsScreen->DisplayMode();

    #ifndef __WINS__

    TScreenInfoV01 screenInfo;
	TPckg<TScreenInfoV01> sInfo(screenInfo);
	UserSvr::ScreenInfo(sInfo);

	Private->EPOC_ScreenSize		= screenInfo.iScreenSize; 
	Private->EPOC_DisplayMode		= displayMode;
    Private->EPOC_HasFrameBuffer	= screenInfo.iScreenAddressValid;
	Private->EPOC_FrameBuffer		= Private->EPOC_HasFrameBuffer ? (TUint8*) screenInfo.iScreenAddress : NULL;
	Private->EPOC_BytesPerPixel	    = ((GetBpp(displayMode)-1) / 8) + 1;
	Private->EPOC_BytesPerScanLine	= screenInfo.iScreenSize.iWidth * Private->EPOC_BytesPerPixel;

    /* It seems that in SA1100 machines for 8bpp displays there is a 512 palette table at the 
     * beginning of the frame buffer. E.g. Series 7 and Netbook.
     * In 12 bpp machines the table has 16 entries.
	 */
	if (Private->EPOC_HasFrameBuffer && GetBpp(displayMode) == 8)
		Private->EPOC_FrameBuffer += 512;
	if (Private->EPOC_HasFrameBuffer && GetBpp(displayMode) == 12)
		Private->EPOC_FrameBuffer += 16 * 2;

    #else /* defined __WINS__ */
    
    /* Create bitmap, device and context for screen drawing */
    Private->EPOC_ScreenSize        = Private->EPOC_WsScreen->SizeInPixels();

	Private->EPOC_Bitmap = new (ELeave) CWsBitmap(Private->EPOC_WsSession);
	Private->EPOC_Bitmap->Create(Private->EPOC_ScreenSize, displayMode);

	Private->EPOC_DisplayMode	    = displayMode;
    Private->EPOC_HasFrameBuffer    = ETrue;
    Private->EPOC_FrameBuffer       = NULL; /* Private->EPOC_Bitmap->DataAddress() can change any time */
	Private->EPOC_BytesPerPixel	    = ((GetBpp(displayMode)-1) / 8) + 1;
	Private->EPOC_BytesPerScanLine  = Private->EPOC_WsScreen->SizeInPixels().iWidth * Private->EPOC_BytesPerPixel;

    #endif /* __WINS__ */

    _this->info.current_w = Private->EPOC_ScreenSize.iWidth;
    _this->info.current_h = Private->EPOC_ScreenSize.iHeight;

    /* The "best" video format should be returned to caller. */

    vformat->BitsPerPixel       = /*!!GetBpp(displayMode) */ 8;
    vformat->BytesPerPixel      = /*!!Private->EPOC_BytesPerPixel*/ 1;

    /* Activate events for me */

	Private->EPOC_WsEventStatus = KRequestPending;
	Private->EPOC_WsSession.EventReady(&Private->EPOC_WsEventStatus);
	Private->EPOC_RedrawEventStatus = KRequestPending;
	Private->EPOC_WsSession.RedrawReady(&Private->EPOC_RedrawEventStatus);
    Private->EPOC_WsWindow.PointerFilter(EPointerFilterDrag, 0); 

    Private->EPOC_ScreenOffset = 0;

    //!! TODO: error handling
    //if (ret != KErrNone)
    //    return(-1);
    //else
	    return(0);
}


SDL_Rect **EPOC_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
    if (format->BitsPerPixel == 12 || format->BitsPerPixel == 8)
        return Private->SDL_modelist;
    return NULL;
}

int EPOC_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
    for(int i = firstcolor; i < ncolors; i++) {
	    // 4k value: 000rgb
	    TUint16 color4K = 0;
        color4K |= (colors[i].r & 0x0000f0) << 4; 
	    color4K |= (colors[i].g & 0x0000f0);      
	    color4K |= (colors[i].b & 0x0000f0) >> 4;
        EPOC_HWPalette_256_to_4k[i] = color4K;
    }
	return(0);
}


SDL_Surface *EPOC_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
    /* Check parameters */
    if (!((width == 640 && height == 200 && bpp == 12) ||
          (width == 640 && height == 400 && bpp == 12) || 
          (width == 640 && height == 480 && bpp == 12) || 
          (width == 320 && height == 200 && bpp == 12) || 
          (width == 640 && height == 200 && bpp == 8) ||
          (width == 640 && height == 400 && bpp == 8) || 
          (width == 640 && height == 480 && bpp == 8) || 
          (width == 320 && height == 200 && bpp == 8))) {
		SDL_SetError("Requested video mode is not supported");
        return NULL;
    }

    if (current && current->pixels) {
        SDL_free(current->pixels);
        current->pixels = NULL;
    }
	if ( ! SDL_ReallocFormat(current, bpp, 0, 0, 0, 0) ) {
		return(NULL);
	}

    /* Set up the new mode framebuffer */
    if (bpp == 8) 
	    current->flags = (SDL_FULLSCREEN|SDL_SWSURFACE|SDL_PREALLOC|SDL_HWPALETTE); 
    else // 12 bpp
	    current->flags = (SDL_FULLSCREEN|SDL_SWSURFACE|SDL_PREALLOC); 
	current->w = width;
	current->h = height;
    int numBytesPerPixel = ((bpp-1)>>3) + 1;   
	current->pitch = numBytesPerPixel * width; // Number of bytes in scanline 
	current->pixels = SDL_malloc(width * height * numBytesPerPixel);
	SDL_memset(current->pixels, 0, width * height * numBytesPerPixel);

	/* Set the blit function */
	_this->UpdateRects = EPOC_DirectUpdate;

    /* Must buffer height be shrinked to screen by 2 ? */
    if (current->h >= 400)
        Private->EPOC_ShrinkedHeight = ETrue;

    /* Centralize game window on device screen  */
    Private->EPOC_ScreenOffset = (Private->EPOC_ScreenSize.iWidth - current->w) / 2;

	/* We're done */
	return(current);
}

void RedrawWindowL(_THIS)
{
    SDL_Rect fullScreen;
    fullScreen.x = 0;
    fullScreen.y = 0;
    fullScreen.w = _this->screen->w;
    fullScreen.h = _this->screen->h;

#ifdef __WINS__
	    TBitmapUtil lock(Private->EPOC_Bitmap);	
        lock.Begin(TPoint(0,0)); // Lock bitmap heap
	    Private->EPOC_WindowGc->Activate(Private->EPOC_WsWindow);
#endif

    if (fullScreen.w < Private->EPOC_ScreenSize.iWidth
        && fullScreen.w < Private->EPOC_ScreenSize.iWidth) {
        /* Draw blue stripes background */
#ifdef __WINS__
        TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
#else
        TUint16* screenBuffer = (TUint16*)Private->EPOC_FrameBuffer;
#endif
        for (int y=0; y < Private->EPOC_ScreenSize.iHeight; y++) {
            for (int x=0; x < Private->EPOC_ScreenSize.iWidth; x++) {
                TUint16 color = ((x+y)>>1) & 0xf; /* Draw pattern */
                *screenBuffer++ = color;
            }
        }
    }


    /* Tell the system that something has been drawn */
	TRect  rect = TRect(Private->EPOC_WsWindow.Size());
  	Private->EPOC_WsWindow.Invalidate(rect);

#ifdef __WINS__
	Private->EPOC_WsWindow.BeginRedraw(rect);
	Private->EPOC_WindowGc->BitBlt(TPoint(), Private->EPOC_Bitmap);
	Private->EPOC_WsWindow.EndRedraw();
	Private->EPOC_WindowGc->Deactivate();
    lock.End(); // Unlock bitmap heap
	Private->EPOC_WsSession.Flush();
#endif

    /* Draw current buffer */
    EPOC_DirectUpdate(_this, 1, &fullScreen);
}


/* We don't actually allow hardware surfaces other than the main one */
static int EPOC_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}
static void EPOC_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static int EPOC_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}
static void EPOC_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static int EPOC_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void EPOC_DirectUpdate(_THIS, int numrects, SDL_Rect *rects)
{
    TInt focusWindowGroupId = Private->EPOC_WsSession.GetFocusWindowGroup();
    if (focusWindowGroupId != Private->EPOC_WsWindowGroupID) {

        /* Force focus window to redraw again for cleaning away SDL screen graphics */

  
        TInt pos = Private->EPOC_WsWindowGroup.OrdinalPosition();
        Private->EPOC_WsWindowGroup.SetOrdinalPosition(0, KMaxTInt);
       	TRect  rect = TRect(Private->EPOC_WsWindow.Size());
        Private->EPOC_WsWindow.Invalidate(rect);
        Private->EPOC_WsWindowGroup.SetOrdinalPosition(pos, ECoeWinPriorityNormal);
        
        /* If this is not the topmost window, wait here! Sleep for 1 second to give cpu to 
           multitasking and poll for being the topmost window.
        */
        while (Private->EPOC_WsSession.GetFocusWindowGroup() != Private->EPOC_WsWindowGroupID)
            SDL_Delay(1000);

        RedrawWindowL(_this);  
    }

	TInt i;
    TInt sourceNumBytesPerPixel = ((_this->screen->format->BitsPerPixel-1)>>3) + 1;   
    TInt targetNumBytesPerPixel = Private->EPOC_BytesPerPixel;   
    TInt fixedOffset = Private->EPOC_ScreenOffset;   
    TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
    TInt sourceScanlineLength = screenW;
    if (Private->EPOC_ShrinkedHeight) {  /* simulate 400 pixel height in 200 pixel screen */  
        sourceScanlineLength <<= 1; 
        screenH >>= 1;
    }
    TInt targetScanlineLength = Private->EPOC_ScreenSize.iWidth;
#ifdef __WINS__
	TBitmapUtil lock(Private->EPOC_Bitmap);	
    lock.Begin(TPoint(0,0)); // Lock bitmap heap
	Private->EPOC_WindowGc->Activate(Private->EPOC_WsWindow);
    TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
#else
    TUint16* screenBuffer = (TUint16*)Private->EPOC_FrameBuffer;
#endif


	/* Render the rectangles in the list */

	for ( i=0; i < numrects; ++i ) {
        SDL_Rect rect2;
        const SDL_Rect& currentRect = rects[i];
        rect2.x = currentRect.x;
        rect2.y = currentRect.y;
        rect2.w = currentRect.w;
        rect2.h = currentRect.h;

        if (rect2.w <= 0 || rect2.h <= 0) /* sanity check */
            continue;

        if (Private->EPOC_ShrinkedHeight) {  /* simulate 400 pixel height in 200 pixel screen */        
            rect2.y >>= 1;
            if (!(rect2.h >>= 1)) 
                rect2.h = 1; // always at least 1 pixel height!
        }

        /* All variables are measured in pixels */

        /* Check rects validity, i.e. upper and lower bounds */
        TInt maxX = Min(screenW - 1, rect2.x + rect2.w - 1);
        TInt maxY = Min(screenH - 1, rect2.y + rect2.h - 1);
        if (maxX < 0 || maxY < 0) /* sanity check */
            continue;
        maxY = Min(maxY, 199); 

        TInt sourceRectWidth = maxX - rect2.x + 1;
        TInt sourceRectWidthInBytes = sourceRectWidth * sourceNumBytesPerPixel;
        TInt sourceRectHeight = maxY - rect2.y + 1;
        TInt sourceStartOffset = rect2.x + rect2.y * sourceScanlineLength;
        TInt targetStartOffset = fixedOffset + rect2.x + rect2.y * targetScanlineLength;   
        
        // !! Nokia9210 native mode: 12 bpp --> 12 bpp
        if (_this->screen->format->BitsPerPixel == 12) { 
	        TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
            for(TInt y = 0 ; y < sourceRectHeight ; y++) {
                __ASSERT_DEBUG(screenMemory < (screenBuffer 
                    + Private->EPOC_ScreenSize.iWidth * Private->EPOC_ScreenSize.iHeight), 
                    User::Panic(_L("SDL"), KErrCorrupt));
                __ASSERT_DEBUG(screenMemory >= screenBuffer, 
                    User::Panic(_L("SDL"), KErrCorrupt));
                __ASSERT_DEBUG(bitmapLine < ((TUint16*)_this->screen->pixels + 
                    + (_this->screen->w * _this->screen->h)), 
                    User::Panic(_L("SDL"), KErrCorrupt));
                __ASSERT_DEBUG(bitmapLine >=  (TUint16*)_this->screen->pixels, 
                    User::Panic(_L("SDL"), KErrCorrupt));
		        Mem::Copy(screenMemory, bitmapLine, sourceRectWidthInBytes);
		        bitmapLine += sourceScanlineLength;
		        screenMemory += targetScanlineLength;
            }
        }
        // !! 256 color paletted mode: 8 bpp  --> 12 bpp
        else { 
	        TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
            for(TInt y = 0 ; y < sourceRectHeight ; y++) {
                TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
                TUint16* screenMemoryLinePos = screenMemory; /* 2 bytes per pixel */
                /* Convert each pixel from 256 palette to 4k color values */
                for(TInt x = 0 ; x < sourceRectWidth ; x++) {
                    __ASSERT_DEBUG(screenMemoryLinePos < (screenBuffer 
                        + (Private->EPOC_ScreenSize.iWidth * Private->EPOC_ScreenSize.iHeight)), 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    __ASSERT_DEBUG(screenMemoryLinePos >= screenBuffer, 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    __ASSERT_DEBUG(bitmapPos < ((TUint8*)_this->screen->pixels + 
                        + (_this->screen->w * _this->screen->h)), 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    __ASSERT_DEBUG(bitmapPos >= (TUint8*)_this->screen->pixels, 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    *screenMemoryLinePos = EPOC_HWPalette_256_to_4k[*bitmapPos];
                    bitmapPos++;
                    screenMemoryLinePos++;
                }
		        bitmapLine += sourceScanlineLength;
		        screenMemory += targetScanlineLength;
            }
	    }

    }    
    
#ifdef __WINS__

	TRect  rect = TRect(Private->EPOC_WsWindow.Size());
	Private->EPOC_WsWindow.Invalidate(rect);
	Private->EPOC_WsWindow.BeginRedraw(rect);
	Private->EPOC_WindowGc->BitBlt(TPoint(), Private->EPOC_Bitmap);
	Private->EPOC_WsWindow.EndRedraw();
	Private->EPOC_WindowGc->Deactivate();
    lock.End(); // Unlock bitmap heap
	Private->EPOC_WsSession.Flush();

#endif

    /* Update virtual cursor */
    //!!Private->EPOC_WsSession.SetPointerCursorPosition(Private->EPOC_WsSession.PointerCursorPosition());

    return;
}


/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void EPOC_VideoQuit(_THIS)
{
	int i;

	/* Free video mode lists */
	for ( i=0; i<SDL_NUMMODES; ++i ) {
		if ( Private->SDL_modelist[i] != NULL ) {
			SDL_free(Private->SDL_modelist[i]);
			Private->SDL_modelist[i] = NULL;
		}
	}
	
    if ( _this->screen && (_this->screen->flags & SDL_HWSURFACE) ) {
		/* Direct screen access, no memory buffer */
		_this->screen->pixels = NULL;
	}

    if (_this->screen && _this->screen->pixels) {
        SDL_free(_this->screen->pixels);
        _this->screen->pixels = NULL;
    }

    /* Free Epoc resources */

    /* Disable events for me */
	if (Private->EPOC_WsEventStatus != KRequestPending)
		Private->EPOC_WsSession.EventReadyCancel();
	if (Private->EPOC_RedrawEventStatus != KRequestPending)
		Private->EPOC_WsSession.RedrawReadyCancel();

    #ifdef __WINS__
	delete Private->EPOC_Bitmap;
	Private->EPOC_Bitmap = NULL;
    #endif

	if (Private->EPOC_WsWindow.WsHandle())
		Private->EPOC_WsWindow.Close();

	if (Private->EPOC_WsWindowGroup.WsHandle())
		Private->EPOC_WsWindowGroup.Close();

	delete Private->EPOC_WindowGc;
	Private->EPOC_WindowGc = NULL;

	delete Private->EPOC_WsScreen;
	Private->EPOC_WsScreen = NULL;

	if (Private->EPOC_WsSession.WsHandle())
		Private->EPOC_WsSession.Close();
}


WMcursor *EPOC_CreateWMCursor(_THIS, Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y)
{
	return (WMcursor *) 9210; // it's ok to return something unuseful but true
}

void EPOC_FreeWMCursor(_THIS, WMcursor *cursor)
{
    /* Disable virtual cursor */
    HAL::Set(HAL::EMouseState, HAL::EMouseState_Invisible);
    Private->EPOC_WsSession.SetPointerCursorMode(EPointerCursorNone);
}

int EPOC_ShowWMCursor(_THIS, WMcursor *cursor)
{

    if (cursor ==  (WMcursor *)9210) {
        /* Enable virtual cursor */
	    HAL::Set(HAL::EMouseState, HAL::EMouseState_Visible);
	    Private->EPOC_WsSession.SetPointerCursorMode(EPointerCursorNormal);
    }
    else {
        /* Disable virtual cursor */
        HAL::Set(HAL::EMouseState, HAL::EMouseState_Invisible);
        Private->EPOC_WsSession.SetPointerCursorMode(EPointerCursorNone);
    }

	return(1);
}

}; // extern "C"

