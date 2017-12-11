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

#pragma once

typedef struct
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char z;
} PIXTYPE_bgr32;

typedef struct
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
} PIXTYPE_bgr24;

typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char z;
} PIXTYPE_rgb32;

typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} PIXTYPE_rgb24;

#define SAT(c)       \
    if (c & (~255))  \
    {                \
        if (c < 0)   \
            c = 0;   \
        else         \
            c = 255; \
    }
