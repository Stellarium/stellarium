/*  CCVT: ColourConVerT: simple library for converting colourspaces
    Copyright (C) 2002 Nemosoft Unv.
    Email:athomas@nemsoft.co.uk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    For questions, remarks, patches, etc. for this program, the author can be
    reached at nemosoft@smcc.demon.nl.
*/

/*
 $Log$
 Revision 1.4  2005/04/29 16:51:20  mutlaqja
 Adding initial support for Video 4 Linux 2 drivers. This mean that KStars can probably control Meade Lunar Planetary Imager (LPI). V4L2 requires a fairly recent kernel (> 2.6.9) and many drivers don't fully support it yet. It will take sometime. KStars still supports V4L1 and will continue so until V4L1 is obselete. Please test KStars video drivers if you can. Any comments welcomed.

 CCMAIL: kstars-devel@kde.org

 Revision 1.3  2004/06/26 23:12:03  mutlaqja
 Hopefully this will fix compile issues on 64bit archs, and FreeBSD, among others. The assembly code is replaced with a more portable, albeit slower C implementation. I imported the videodev.h header after cleaning it for user space.

 Anyone who has problems compiling this, please report the problem to kstars-devel@kde.org

 I noticed one odd thing after updating my kdelibs, the LEDs don't change color when state is changed. Try that by starting any INDI device, and hit connect, if the LED turns to yellow and back to grey then it works fine, otherwise, we've got a problem.

 CCMAIL: kstars-devel@kde.org

 Revision 1.10  2003/10/24 16:55:18  nemosoft
 removed erronous log messages

 Revision 1.9  2002/11/03 22:46:25  nemosoft
 Adding various RGB to RGB functions.
 Adding proper copyright header too.

 Revision 1.8  2002/04/14 01:00:27  nemosoft
 Finishing touches: adding const, adding libs for 'show'
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup colorSpace Color space conversion functions
    Colour ConVerT: going from one colour space to another.

   Format descriptions:\n
   420i = "4:2:0 interlaced"\n
           YYYY UU YYYY UU   even lines\n
           YYYY VV YYYY VV   odd lines\n
           U/V data is subsampled by 2 both in horizontal and vertical directions, and intermixed with the Y values.\n\n

   420p = "4:2:0 planar"\n
           YYYYYYYY      N lines\n
           UUUU          N/2 lines\n
           VVVV          N/2 lines\n
           U/V is again subsampled, but all the Ys, Us and Vs are placed together in separate buffers. The buffers may be placed in one piece of contiguous memory though, with Y buffer first,
           followed by U, followed by V.

   yuyv = "4:2:2 interlaced"\n
           YUYV YUYV YUYV ...   N lines\n
           The U/V data is subsampled by 2 in horizontal direction only.\n\n

   bgr24 = 3 bytes per pixel, in the order Blue Green Red (whoever came up with that idea...)\n
   rgb24 = 3 bytes per pixel, in the order Red Green Blue (which is sensible)\n
   rgb32 = 4 bytes per pixel, in the order Red Green Blue Alpha, with Alpha really being a filler byte (0)\n
   bgr32 = last but not least, 4 bytes per pixel, in the order Blue Green Red Alpha, Alpha again a filler byte (0)\n
 */

/*@{*/

/** 4:2:0 YUV planar to RGB/BGR     */
void ccvt_420p_bgr24(int width, int height, const void *src, void *dst);
/** 4:2:0 YUV planar to RGB/BGR     */
void ccvt_420p_rgb24(int width, int height, const void *src, void *dst);
/** 4:2:0 YUV planar to RGB/BGR     */
void ccvt_420p_bgr32(int width, int height, const void *src, void *dst);
/** 4:2:0 YUV planar to RGB/BGR     */
void ccvt_420p_rgb32(int width, int height, const void *src, void *dst);

/** 4:2:2 YUYV interlaced to RGB/BGR */
void ccvt_yuyv_rgb32(int width, int height, const void *src, void *dst);
/** 4:2:2 YUYV interlaced to RGB/BGR */
void ccvt_yuyv_bgr32(int width, int height, const void *src, void *dst);
/** 4:2:2 YUYV interlaced to BGR24 */
void ccvt_yuyv_bgr24(int width, int height, const void *src, void *dst);
/** 4:2:2 YUYV interlaced to RGB24 */
void ccvt_yuyv_rgb24(int width, int height, const void *src, void *dst);

/** 4:2:2 YUYV interlaced to 4:2:0 YUV planar */
void ccvt_yuyv_420p(int width, int height, const void *src, void *dsty, void *dstu, void *dstv);

/* RGB/BGR to 4:2:0 YUV interlaced */

/** RGB/BGR to 4:2:0 YUV planar     */
void ccvt_rgb24_420p(int width, int height, const void *src, void *dsty, void *dstu, void *dstv);
/** RGB/BGR to 4:2:0 YUV planar     */
void ccvt_bgr24_420p(int width, int height, const void *src, void *dsty, void *dstu, void *dstv);

/** RGB/BGR to RGB/BGR */
void ccvt_bgr24_bgr32(int width, int height, const void *const src, void *const dst);
/** RGB/BGR to RGB/BGR */
void ccvt_bgr24_rgb32(int width, int height, const void *const src, void *const dst);
/** RGB/BGR to RGB/BGR */
void ccvt_bgr32_bgr24(int width, int height, const void *const src, void *const dst);
/** RGB/BGR to RGB/BGR */
void ccvt_bgr32_rgb24(int width, int height, const void *const src, void *const dst);
/** RGB/BGR to RGB/BGR */
void ccvt_rgb24_bgr32(int width, int height, const void *const src, void *const dst);
/** RGB/BGR to RGB/BGR */
void ccvt_rgb24_rgb32(int width, int height, const void *const src, void *const dst);
/** RGB/BGR to RGB/BGR */
void ccvt_rgb32_bgr24(int width, int height, const void *const src, void *const dst);
/** RGB/BGR to RGB/BGR */
void ccvt_rgb32_rgb24(int width, int height, const void *const src, void *const dst);

/** RGB to YUV */
int RGB2YUV(int x_dim, int y_dim, void *bmp, void *y_out, void *u_out, void *v_out, int flip);

/**
 * @short mjpegtoyuv420p MPEG to YUV 420 P
 *
 * Return values
 *  -1 on fatal error
 *  0  on success
 *  2  if jpeg lib threw a "corrupt jpeg data" warning.
 *     in this case, "a damaged output image is likely."
 *
 * Copyright 2000 by Jeroen Vreeken (pe1rxq@amsat.org)
 * 2006 by Krzysztof Blaszkowski (kb@sysmikro.com.pl)
 * 2007 by Angel Carpinteo (ack@telefonica.net)
 */
int mjpegtoyuv420p(unsigned char *map, unsigned char *cap_map, int width, int height, unsigned int size);

/*
 * BAYER2RGB24 ROUTINE TAKEN FROM:
 *
 * Sonix SN9C101 based webcam basic I/F routines
 * Copyright (C) 2004 Takafumi Mizuno <taka-qce@ls-a.jp>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/** Bayer 8bit to RGB 24 */
void bayer2rgb24(unsigned char *dst, unsigned char *src, long int WIDTH, long int HEIGHT);
/** Bayer 16 bit to RGB 24 */
void bayer16_2_rgb24(unsigned short *dst, unsigned short *src, long int WIDTH, long int HEIGHT);
/** Bayer RGGB to RGB 24 */
void bayer_rggb_2rgb24(unsigned char *dst, unsigned char *srcc, long int WIDTH, long int HEIGHT);

/*@}*/

#ifdef __cplusplus
}
#endif

enum Options
{
    ioNoBlock      = (1 << 0),
    ioUseSelect    = (1 << 1),
    haveBrightness = (1 << 2),
    haveContrast   = (1 << 3),
    haveHue        = (1 << 4),
    haveColor      = (1 << 5),
    haveWhiteness  = (1 << 6)
};
