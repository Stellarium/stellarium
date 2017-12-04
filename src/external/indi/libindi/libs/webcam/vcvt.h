/*
   (C) 2001 Nemosoft Unv.    <nemosoft@smcc.demon.nl>

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

*/

/* 'Viewport' conversion routines. These functions convert from one colour
   space to another, taking into account that the source image has a smaller
   size than the view, and is placed inside the view:

        +-------view.x------------+
        |                         |
        |     +---image.x---+     |
        |     |             |     |
        |     |             |     |
        |     +-------------+     |
        |                         |
        +-------------------------+

   The image should always be smaller than the view. The offset (top-left
   corner of the image) should be precomputed, so you can place the image
   anywhere in the view.

   The functions take these parameters:
   - width 	image width (in pixels)
   - height	image height (in pixels)
   - plus	view width (in pixels)
   *src		pointer at start of image
   *dst		pointer at offset (!) in view
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Functions in vcvt_i386.S/vcvt_c.c */
/* 4:2:0 YUV interlaced to RGB/BGR */
void vcvt_420i_bgr24(int width, int height, int plus, void *src, void *dst);
void vcvt_420i_rgb24(int width, int height, int plus, void *src, void *dst);
void vcvt_420i_bgr32(int width, int height, int plus, void *src, void *dst);
void vcvt_420i_rgb32(int width, int height, int plus, void *src, void *dst);

/* Go from 420i to other yuv formats */
void vcvt_420i_420p(int width, int height, int plus, void *src, void *dsty, void *dstu, void *dstv);
void vcvt_420i_yuyv(int width, int height, int plus, void *src, void *dst);

#if 0
#endif

#ifdef __cplusplus
}
#endif
