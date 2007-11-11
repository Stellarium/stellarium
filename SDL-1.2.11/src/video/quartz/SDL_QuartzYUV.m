/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2003  Sam Lantinga

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

#include "SDL_QuartzVideo.h"
#include "SDL_QuartzWindow.h"
#include "../SDL_yuvfuncs.h"


#define yuv_idh (this->hidden->yuv_idh)
#define yuv_matrix (this->hidden->yuv_matrix)
#define yuv_codec (this->hidden->yuv_codec)
#define yuv_seq (this->hidden->yuv_seq)
#define yuv_pixmap (this->hidden->yuv_pixmap)
#define yuv_data (this->hidden->yuv_data)
#define yuv_width (this->hidden->yuv_width)
#define yuv_height (this->hidden->yuv_height)
#define yuv_port (this->hidden->yuv_port)


static int QZ_LockYUV (_THIS, SDL_Overlay *overlay) {

    return 0;
}

static void QZ_UnlockYUV (_THIS, SDL_Overlay *overlay) {

    ;
}

static int QZ_DisplayYUV (_THIS, SDL_Overlay *overlay, SDL_Rect *src, SDL_Rect *dst) {

    OSErr err;
    CodecFlags flags;

    if (dst->x != 0 || dst->y != 0) {

        SDL_SetError ("Need a dst at (0,0)");
        return -1;
    }

    if (dst->w != yuv_width || dst->h != yuv_height) {

        Fixed scale_x, scale_y;

        scale_x = FixDiv ( Long2Fix (dst->w), Long2Fix (overlay->w) );
        scale_y = FixDiv ( Long2Fix (dst->h), Long2Fix (overlay->h) );

        SetIdentityMatrix (yuv_matrix);
        ScaleMatrix (yuv_matrix, scale_x, scale_y, Long2Fix (0), Long2Fix (0));

        SetDSequenceMatrix (yuv_seq, yuv_matrix);

        yuv_width = dst->w;
        yuv_height = dst->h;
    }

    if( ( err = DecompressSequenceFrameS(
                                         yuv_seq,
                                         (void*)yuv_pixmap,
                                         sizeof (PlanarPixmapInfoYUV420),
                                         codecFlagUseImageBuffer, &flags, nil ) != noErr ) )
    {
        SDL_SetError ("DecompressSequenceFrameS failed");
    }

    return err != noErr;
}

static void QZ_FreeHWYUV (_THIS, SDL_Overlay *overlay) {

    CDSequenceEnd (yuv_seq);
    ExitMovies();

    SDL_free (overlay->hwfuncs);
    SDL_free (overlay->pitches);
    SDL_free (overlay->pixels);

    if (SDL_VideoSurface->flags & SDL_FULLSCREEN) {
        [ qz_window close ];
        qz_window = nil;
    }

    SDL_free (yuv_matrix);
    DisposeHandle ((Handle)yuv_idh);
}

/* check for 16 byte alignment, bail otherwise */
#define CHECK_ALIGN(x) do { if ((Uint32)x & 15) { SDL_SetError("Alignment error"); return NULL; } } while(0)

/* align a byte offset, return how much to add to make it a multiple of 16 */
#define ALIGN(x) ((16 - (x & 15)) & 15)

SDL_Overlay* QZ_CreateYUVOverlay (_THIS, int width, int height,
                                         Uint32 format, SDL_Surface *display) {

    Uint32 codec;
    OSStatus err;
    CGrafPtr port;
    SDL_Overlay *overlay;

    if (format == SDL_YV12_OVERLAY ||
        format == SDL_IYUV_OVERLAY) {

        codec = kYUV420CodecType;
    }
    else {
        SDL_SetError ("Hardware: unsupported video format");
        return NULL;
    }

    yuv_idh = (ImageDescriptionHandle) NewHandleClear (sizeof(ImageDescription));
    if (yuv_idh == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    yuv_matrix = (MatrixRecordPtr) SDL_malloc (sizeof(MatrixRecord));
    if (yuv_matrix == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    if ( EnterMovies() != noErr ) {
        SDL_SetError ("Could not init QuickTime for YUV playback");
        return NULL;
    }

    err = FindCodec (codec, bestSpeedCodec, nil, &yuv_codec);
    if (err != noErr) {
        SDL_SetError ("Could not find QuickTime codec for format");
        return NULL;
    }

    if (SDL_VideoSurface->flags & SDL_FULLSCREEN) {

        /*
          Acceleration requires a window to be present.
          A CGrafPtr that points to the screen isn't good enough
        */
        NSRect content = NSMakeRect (0, 0, SDL_VideoSurface->w, SDL_VideoSurface->h);

        qz_window = [ [ SDL_QuartzWindow alloc ]
                            initWithContentRect:content
                            styleMask:NSBorderlessWindowMask
                            backing:NSBackingStoreBuffered defer:NO ];

        if (qz_window == nil) {
            SDL_SetError ("Could not create the Cocoa window");
            return NULL;
        }

        [ qz_window setContentView:[ [ NSQuickDrawView alloc ] init ] ];
        [ qz_window setReleasedWhenClosed:YES ];
        [ qz_window center ];
        [ qz_window setAcceptsMouseMovedEvents:YES ];
        [ qz_window setLevel:CGShieldingWindowLevel() ];
        [ qz_window makeKeyAndOrderFront:nil ];

        port = [ [ qz_window contentView ] qdPort ];
        SetPort (port);
        
        /*
            BUG: would like to remove white flash when window kicks in
            {
                Rect r;
                SetRect (&r, 0, 0, SDL_VideoSurface->w, SDL_VideoSurface->h);
                PaintRect (&r);
                QDFlushPortBuffer (port, nil);
            }
        */
    }
    else {
        port = [ window_view qdPort ];
        SetPort (port);
    }
    
    SetIdentityMatrix (yuv_matrix);
    
    HLock ((Handle)yuv_idh);
    
    (**yuv_idh).idSize = sizeof(ImageDescription);
    (**yuv_idh).cType  = codec;
    (**yuv_idh).version = 1;
    (**yuv_idh).revisionLevel = 0;
    (**yuv_idh).width = width;
    (**yuv_idh).height = height;
    (**yuv_idh).hRes = Long2Fix(72);
    (**yuv_idh).vRes = Long2Fix(72);
    (**yuv_idh).spatialQuality = codecLosslessQuality;
    (**yuv_idh).frameCount = 1;
    (**yuv_idh).clutID = -1;
    (**yuv_idh).dataSize = 0;
    (**yuv_idh).depth = 24;
    
    HUnlock ((Handle)yuv_idh);
    
    err = DecompressSequenceBeginS (
                                    &yuv_seq,
                                    yuv_idh,
                                    NULL,
                                    0,
                                    port,
                                    NULL,
                                    NULL,
                                    yuv_matrix,
                                    0,
                                    NULL,
                                    codecFlagUseImageBuffer,
                                    codecLosslessQuality,
                                    yuv_codec);
    
    if (err != noErr) {
        SDL_SetError ("Error trying to start YUV codec.");
        return NULL;
    }
    
    overlay = (SDL_Overlay*) SDL_malloc (sizeof(*overlay));
    if (overlay == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }
    
    overlay->format      = format;
    overlay->w           = width;
    overlay->h           = height;
    overlay->planes      = 3;
    overlay->hw_overlay  = 1;
    {
        int      offset;
        Uint8  **pixels;
        Uint16  *pitches;
        int      plane2, plane3;

        if (format == SDL_IYUV_OVERLAY) {

            plane2 = 1; /* Native codec format */
            plane3 = 2;
        }
        else if (format == SDL_YV12_OVERLAY) {

            /* switch the U and V planes */
            plane2 = 2; /* U plane maps to plane 3 */
            plane3 = 1; /* V plane maps to plane 2 */
        }
        else {
            SDL_SetError("Unsupported YUV format");
            return NULL;
        }

        pixels = (Uint8**) SDL_malloc (sizeof(*pixels) * 3);
        pitches = (Uint16*) SDL_malloc (sizeof(*pitches) * 3);
        if (pixels == NULL || pitches == NULL) {
            SDL_OutOfMemory();
            return NULL;
        }

        yuv_pixmap = (PlanarPixmapInfoYUV420*)
            SDL_malloc (sizeof(PlanarPixmapInfoYUV420) +
                    (width * height * 2));
        if (yuv_pixmap == NULL) {
            SDL_OutOfMemory ();
            return NULL;
        }

        /* CHECK_ALIGN(yuv_pixmap); */
        offset  = sizeof(PlanarPixmapInfoYUV420);
        /* offset += ALIGN(offset); */
        /* CHECK_ALIGN(offset); */

        pixels[0] = (Uint8*)yuv_pixmap + offset;
        /* CHECK_ALIGN(pixels[0]); */

        pitches[0] = width;
        yuv_pixmap->componentInfoY.offset = offset;
        yuv_pixmap->componentInfoY.rowBytes = width;

        offset += width * height;
        pixels[plane2] = (Uint8*)yuv_pixmap + offset;
        pitches[plane2] = width / 2;
        yuv_pixmap->componentInfoCb.offset = offset;
        yuv_pixmap->componentInfoCb.rowBytes = width / 2;

        offset += (width * height / 4);
        pixels[plane3] = (Uint8*)yuv_pixmap + offset;
        pitches[plane3] = width / 2;
        yuv_pixmap->componentInfoCr.offset = offset;
        yuv_pixmap->componentInfoCr.rowBytes = width / 2;

        overlay->pixels = pixels;
        overlay->pitches = pitches;
    }

    overlay->hwfuncs = SDL_malloc (sizeof(*overlay->hwfuncs));
    if (overlay->hwfuncs == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }
    
    overlay->hwfuncs->Lock    = QZ_LockYUV;
    overlay->hwfuncs->Unlock  = QZ_UnlockYUV;
    overlay->hwfuncs->Display = QZ_DisplayYUV;
    overlay->hwfuncs->FreeHW  = QZ_FreeHWYUV;

    yuv_width = overlay->w;
    yuv_height = overlay->h;
    
    return overlay;
}
