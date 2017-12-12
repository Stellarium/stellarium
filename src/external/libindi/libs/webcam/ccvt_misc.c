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

/* This file contains CCVT functions that aren't available in assembly yet
   (or are not worth programming)
 */

/*
 * $Log$
 * Revision 1.2  2005/04/29 16:51:20  mutlaqja
 * Adding initial support for Video 4 Linux 2 drivers. This mean that KStars can probably control Meade Lunar Planetary Imager (LPI). V4L2 requires a fairly recent kernel (> 2.6.9) and many drivers don't fully support it yet. It will take sometime. KStars still supports V4L1 and will continue so until V4L1 is obselete. Please test KStars video drivers if you can. Any comments welcomed.
 *
 * CCMAIL: kstars-devel@kde.org
 *
 * Revision 1.1  2004/06/26 23:12:03  mutlaqja
 * Hopefully this will fix compile issues on 64bit archs, and FreeBSD, among others. The assembly code is replaced with a more portable, albeit slower C implementation. I imported the videodev.h header after cleaning it for user space.
 *
 * Anyone who has problems compiling this, please report the problem to kstars-devel@kde.org
 *
 * I noticed one odd thing after updating my kdelibs, the LEDs don't change color when state is changed. Try that by starting any INDI device, and hit connect, if the LED turns to yellow and back to grey then it works fine, otherwise, we've got a problem.
 *
 * CCMAIL: kstars-devel@kde.org
 *
 * Revision 1.7  2003/01/02 04:10:19  nemosoft
 * Adding ''upside down" conversion to rgb/bgr routines
 *
 * Revision 1.6  2002/12/03 23:29:11  nemosoft
 * *** empty log message ***
 *
 * Revision 1.5  2002/12/03 23:27:41  nemosoft
 * fixing log messages (gcc 3.2 complaining)
 *
   Revision 1.4  2002/12/03 22:29:07  nemosoft
   Fixing up FTP stuff and some video

   Revision 1.3  2002/11/03 22:46:25  nemosoft
   Adding various RGB to RGB functions.
   Adding proper copyright header too.
 */

#include "ccvt.h"
#include "ccvt_types.h"
#include "indidevapi.h"
#include "jpegutils.h"

#include <stdlib.h>
#include <string.h>

static float RGBYUV02990[256], RGBYUV05870[256], RGBYUV01140[256];
static float RGBYUV01684[256], RGBYUV03316[256];
static float RGBYUV04187[256], RGBYUV00813[256];

void InitLookupTable(void);

/* YUYV: two Y's and one U/V */
void ccvt_yuyv_rgb32(int width, int height, const void *src, void *dst)
{
    INDI_UNUSED(width);
    INDI_UNUSED(height);
    INDI_UNUSED(src);
    INDI_UNUSED(dst);
}

void ccvt_yuyv_bgr32(int width, int height, const void *src, void *dst)
{
    const unsigned char *s;
    PIXTYPE_bgr32 *d;
    int l, c;
    int r, g, b, cr, cg, cb, y1, y2;

    l = height;
    s = src;
    d = dst;
    while (l--)
    {
        c = width >> 1;
        while (c--)
        {
            y1 = *s++;
            cb = ((*s - 128) * 454) >> 8;
            cg = (*s++ - 128) * 88;
            y2 = *s++;
            cr = ((*s - 128) * 359) >> 8;
            cg = (cg + (*s++ - 128) * 183) >> 8;

            r = y1 + cr;
            b = y1 + cb;
            g = y1 - cg;
            SAT(r);
            SAT(g);
            SAT(b);
            d->b = b;
            d->g = g;
            d->r = r;
            d++;
            r = y2 + cr;
            b = y2 + cb;
            g = y2 - cg;
            SAT(r);
            SAT(g);
            SAT(b);
            d->b = b;
            d->g = g;
            d->r = r;
            d++;
        }
    }
}

void ccvt_yuyv_bgr24(int width, int height, const void *src, void *dst)
{
    const unsigned char *s;
    PIXTYPE_bgr24 *d;
    int l, c;
    int r, g, b, cr, cg, cb, y1, y2;

    l = height;
    s = src;
    d = dst;
    while (l--)
    {
        c = width >> 1;
        while (c--)
        {
            y1 = *s++;
            cb = ((*s - 128) * 454) >> 8;
            cg = (*s++ - 128) * 88;
            y2 = *s++;
            cr = ((*s - 128) * 359) >> 8;
            cg = (cg + (*s++ - 128) * 183) >> 8;

            r = y1 + cr;
            b = y1 + cb;
            g = y1 - cg;
            SAT(r);
            SAT(g);
            SAT(b);
            d->b = b;
            d->g = g;
            d->r = r;
            d++;
            r = y2 + cr;
            b = y2 + cb;
            g = y2 - cg;
            SAT(r);
            SAT(g);
            SAT(b);
            d->b = b;
            d->g = g;
            d->r = r;
            d++;
        }
    }
}

void ccvt_yuyv_rgb24(int width, int height, const void *src, void *dst)
{
    const unsigned char *s;
    PIXTYPE_rgb24 *d;
    int l, c;
    int r, g, b, cr, cg, cb, y1, y2;

    l = height;
    s = src;
    d = dst;
    while (l--)
    {
        c = width >> 1;
        while (c--)
        {
            y1 = *s++;
            cb = ((*s - 128) * 454) >> 8;
            cg = (*s++ - 128) * 88;
            y2 = *s++;
            cr = ((*s - 128) * 359) >> 8;
            cg = (cg + (*s++ - 128) * 183) >> 8;

            r = y1 + cr;
            b = y1 + cb;
            g = y1 - cg;
            SAT(r);
            SAT(g);
            SAT(b);
            d->r = r;
            d->g = g;
            d->b = b;
            d++;
            r = y2 + cr;
            b = y2 + cb;
            g = y2 - cg;
            SAT(r);
            SAT(g);
            SAT(b);
            d->r = r;
            d->g = g;
            d->b = b;
            d++;
        }
    }
}

void ccvt_yuyv_420p(int width, int height, const void *src, void *dsty, void *dstu, void *dstv)
{
    int n, l, j;
    const unsigned char *s1, *s2;
    unsigned char *dy, *du, *dv;

    dy = (unsigned char *)dsty;
    du = (unsigned char *)dstu;
    dv = (unsigned char *)dstv;
    s1 = (unsigned char *)src;
    s2 = s1; /* keep pointer */
    n  = width * height;
    for (; n > 0; n--)
    {
        *dy = *s1;
        dy++;
        s1 += 2;
    }

    /* Two options here: average U/V values, or skip every second row */
    s1 = s2; /* restore pointer */
    s1++;    /* point to U */
    for (l = 0; l < height; l += 2)
    {
        s2 = s1 + width * 2; /* odd line */
        for (j = 0; j < width; j += 2)
        {
            *du = (*s1 + *s2) / 2;
            du++;
            s1 += 2;
            s2 += 2;
            *dv = (*s1 + *s2) / 2;
            dv++;
            s1 += 2;
            s2 += 2;
        }
        s1 = s2;
    }
}

void bayer2rgb24(unsigned char *dst, unsigned char *src, long int WIDTH, long int HEIGHT)
{
    long int i;
    unsigned char *rawpt, *scanpt;
    long int size;

    rawpt  = src;
    scanpt = dst;
    size   = WIDTH * HEIGHT;

    for (i = 0; i < size; i++)
    {
        if ((i / WIDTH) % 2 == 0)
        {
            if ((i % 2) == 0)
            {
                /* B */
                if ((i > WIDTH) && ((i % WIDTH) > 0))
                {
                    *scanpt++ =
                        (*(rawpt - WIDTH - 1) + *(rawpt - WIDTH + 1) + *(rawpt + WIDTH - 1) + *(rawpt + WIDTH + 1)) /
                        4;                                                                               /* R */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1) + *(rawpt + WIDTH) + *(rawpt - WIDTH)) / 4; /* G */
                    *scanpt++ = *rawpt;                                                                  /* B */
                }
                else
                {
                    /* first line or left column */
                    *scanpt++ = *(rawpt + WIDTH + 1);                  /* R */
                    *scanpt++ = (*(rawpt + 1) + *(rawpt + WIDTH)) / 2; /* G */
                    *scanpt++ = *rawpt;                                /* B */
                }
            }
            else
            {
                /* (B)G */
                if ((i > WIDTH) && ((i % WIDTH) < (WIDTH - 1)))
                {
                    *scanpt++ = (*(rawpt + WIDTH) + *(rawpt - WIDTH)) / 2; /* R */
                    *scanpt++ = *rawpt;                                    /* G */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1)) / 2;         /* B */
                }
                else
                {
                    /* first line or right column */
                    *scanpt++ = *(rawpt + WIDTH); /* R */
                    *scanpt++ = *rawpt;           /* G */
                    *scanpt++ = *(rawpt - 1);     /* B */
                }
            }
        }
        else
        {
            if ((i % 2) == 0)
            {
                /* G(R) */
                if ((i < (WIDTH * (HEIGHT - 1))) && ((i % WIDTH) > 0))
                {
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1)) / 2;         /* R */
                    *scanpt++ = *rawpt;                                    /* G */
                    *scanpt++ = (*(rawpt + WIDTH) + *(rawpt - WIDTH)) / 2; /* B */
                }
                else
                {
                    /* bottom line or left column */
                    *scanpt++ = *(rawpt + 1);     /* R */
                    *scanpt++ = *rawpt;           /* G */
                    *scanpt++ = *(rawpt - WIDTH); /* B */
                }
            }
            else
            {
                /* R */
                if (i < (WIDTH * (HEIGHT - 1)) && ((i % WIDTH) < (WIDTH - 1)))
                {
                    *scanpt++ = *rawpt;                                                                  /* R */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1) + *(rawpt - WIDTH) + *(rawpt + WIDTH)) / 4; /* G */
                    *scanpt++ =
                        (*(rawpt - WIDTH - 1) + *(rawpt - WIDTH + 1) + *(rawpt + WIDTH - 1) + *(rawpt + WIDTH + 1)) /
                        4; /* B */
                }
                else
                {
                    /* bottom line or right column */
                    *scanpt++ = *rawpt;                                /* R */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt - WIDTH)) / 2; /* G */
                    *scanpt++ = *(rawpt - WIDTH - 1);                  /* B */
                }
            }
        }
        rawpt++;
    }
}

void bayer16_2_rgb24(unsigned short *dst, unsigned short *src, long int WIDTH, long int HEIGHT)
{
    long int i;
    unsigned short *rawpt, *scanpt;
    long int size;

    rawpt  = src;
    scanpt = dst;
    size   = WIDTH * HEIGHT;

    for (i = 0; i < size; i++)
    {
        if ((i / WIDTH) % 2 == 0)
        {
            if ((i % 2) == 0)
            {
                /* B */
                if ((i > WIDTH) && ((i % WIDTH) > 0))
                {
                    *scanpt++ =
                        (*(rawpt - WIDTH - 1) + *(rawpt - WIDTH + 1) + *(rawpt + WIDTH - 1) + *(rawpt + WIDTH + 1)) /
                        4;                                                                               /* R */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1) + *(rawpt + WIDTH) + *(rawpt - WIDTH)) / 4; /* G */
                    *scanpt++ = *rawpt;                                                                  /* B */
                }
                else
                {
                    /* first line or left column */
                    *scanpt++ = *(rawpt + WIDTH + 1);                  /* R */
                    *scanpt++ = (*(rawpt + 1) + *(rawpt + WIDTH)) / 2; /* G */
                    *scanpt++ = *rawpt;                                /* B */
                }
            }
            else
            {
                /* (B)G */
                if ((i > WIDTH) && ((i % WIDTH) < (WIDTH - 1)))
                {
                    *scanpt++ = (*(rawpt + WIDTH) + *(rawpt - WIDTH)) / 2; /* R */
                    *scanpt++ = *rawpt;                                    /* G */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1)) / 2;         /* B */
                }
                else
                {
                    /* first line or right column */
                    *scanpt++ = *(rawpt + WIDTH); /* R */
                    *scanpt++ = *rawpt;           /* G */
                    *scanpt++ = *(rawpt - 1);     /* B */
                }
            }
        }
        else
        {
            if ((i % 2) == 0)
            {
                /* G(R) */
                if ((i < (WIDTH * (HEIGHT - 1))) && ((i % WIDTH) > 0))
                {
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1)) / 2;         /* R */
                    *scanpt++ = *rawpt;                                    /* G */
                    *scanpt++ = (*(rawpt + WIDTH) + *(rawpt - WIDTH)) / 2; /* B */
                }
                else
                {
                    /* bottom line or left column */
                    *scanpt++ = *(rawpt + 1);     /* R */
                    *scanpt++ = *rawpt;           /* G */
                    *scanpt++ = *(rawpt - WIDTH); /* B */
                }
            }
            else
            {
                /* R */
                if (i < (WIDTH * (HEIGHT - 1)) && ((i % WIDTH) < (WIDTH - 1)))
                {
                    *scanpt++ = *rawpt;                                                                  /* R */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1) + *(rawpt - WIDTH) + *(rawpt + WIDTH)) / 4; /* G */
                    *scanpt++ =
                        (*(rawpt - WIDTH - 1) + *(rawpt - WIDTH + 1) + *(rawpt + WIDTH - 1) + *(rawpt + WIDTH + 1)) /
                        4; /* B */
                }
                else
                {
                    /* bottom line or right column */
                    *scanpt++ = *rawpt;                                /* R */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt - WIDTH)) / 2; /* G */
                    *scanpt++ = *(rawpt - WIDTH - 1);                  /* B */
                }
            }
        }
        rawpt++;
    }
}

void bayer_rggb_2rgb24(unsigned char *dst, unsigned char *src, long int WIDTH, long int HEIGHT)
{
    long int i;
    unsigned char *rawpt, *scanpt;
    long int size;

    rawpt  = src;
    scanpt = dst;
    size   = WIDTH * HEIGHT;

    for (i = 0; i < size; i++)
    {
        if ((i / WIDTH) % 2 == 0) //wenn zeile grade
        {
            if ((i % 2) == 0) //spalte gerade
            {
                /* B */
                if ((i > WIDTH) && ((i % WIDTH) > 0)) // wenn nicht erste zeile oder linke spalte
                {
                    *scanpt++ = *rawpt;                                                                  /* R */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1) + *(rawpt + WIDTH) + *(rawpt - WIDTH)) / 4; /* G */
                    *scanpt++ =
                        (*(rawpt - WIDTH - 1) + *(rawpt - WIDTH + 1) + *(rawpt + WIDTH - 1) + *(rawpt + WIDTH + 1)) /
                        4; /* B */
                }
                else
                {
                    /* first line or left column */
                    *scanpt++ = *rawpt;                                /* R */
                    *scanpt++ = (*(rawpt + 1) + *(rawpt + WIDTH)) / 2; /* G */
                    *scanpt++ = *(rawpt + WIDTH + 1);                  /* B */
                }
            }
            else
            {
                /* (B)G */
                if ((i > WIDTH) && ((i % WIDTH) < (WIDTH - 1)))
                {
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1)) / 2;         /* R */
                    *scanpt++ = *rawpt;                                    /* G */
                    *scanpt++ = (*(rawpt + WIDTH) + *(rawpt - WIDTH)) / 2; /* B */
                }
                else
                {
                    /* first line or right column */
                    *scanpt++ = *(rawpt - 1);     /* R */
                    *scanpt++ = *rawpt;           /* G */
                    *scanpt++ = *(rawpt + WIDTH); /* B */
                }
            }
        }
        else
        {
            if ((i % 2) == 0)
            {
                /* G(R) */
                if ((i < (WIDTH * (HEIGHT - 1))) && ((i % WIDTH) > 0))
                {
                    *scanpt++ = (*(rawpt + WIDTH) + *(rawpt - WIDTH)) / 2; /* R */
                    *scanpt++ = *rawpt;                                    /* G */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1)) / 2;         /* B */
                }
                else
                {
                    /* bottom line or left column */
                    *scanpt++ = *(rawpt - WIDTH); /* R */
                    *scanpt++ = *rawpt;           /* G */
                    *scanpt++ = *(rawpt + 1);     /* B */
                }
            }
            else
            {
                /* R */
                if (i < (WIDTH * (HEIGHT - 1)) && ((i % WIDTH) < (WIDTH - 1)))
                {
                    *scanpt++ =
                        (*(rawpt - WIDTH - 1) + *(rawpt - WIDTH + 1) + *(rawpt + WIDTH - 1) + *(rawpt + WIDTH + 1)) /
                        4;                                                                               /* R */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1) + *(rawpt - WIDTH) + *(rawpt + WIDTH)) / 4; /* G */
                    *scanpt++ = *rawpt;                                                                  /* B */
                }
                else
                {
                    /* bottom line or right column */
                    *scanpt++ = *(rawpt - WIDTH - 1);                  /* R */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt - WIDTH)) / 2; /* G */
                    *scanpt++ = *rawpt;                                /* B */
                }
            }
        }
        rawpt++;
    }
}

int mjpegtoyuv420p(unsigned char *map, unsigned char *cap_map, int width, int height, unsigned int size)
{
    unsigned char *yuv[3];
    unsigned char *y, *u, *v;
    int loop, ret;

    yuv[0] = malloc(width * height * sizeof(yuv[0][0]));
    yuv[1] = malloc(width * height / 4 * sizeof(yuv[1][0]));
    yuv[2] = malloc(width * height / 4 * sizeof(yuv[2][0]));

    ret = decode_jpeg_raw(cap_map, size, 0, 420, width, height, yuv[0], yuv[1], yuv[2]);

    y = map;
    u = y + width * height;
    v = u + (width * height) / 4;
    memset(y, 0, width * height);
    memset(u, 0, width * height / 4);
    memset(v, 0, width * height / 4);

    for (loop = 0; loop < width * height; loop++)
        *map++ = yuv[0][loop];

    for (loop = 0; loop < width * height / 4; loop++)
        *map++ = yuv[1][loop];

    for (loop = 0; loop < width * height / 4; loop++)
        *map++ = yuv[2][loop];

    free(yuv[0]);
    free(yuv[1]);
    free(yuv[2]);

    return ret;
}

/************************************************************************
 *
 *  int RGB2YUV (int x_dim, int y_dim, void *bmp, YUV *yuv)
 *
 *	Purpose :	It takes a 24-bit RGB bitmap and convert it into
 *				YUV (4:2:0) format
 *
 *  Input :		x_dim	the x dimension of the bitmap
 *				y_dim	the y dimension of the bitmap
 *				bmp		pointer to the buffer of the bitmap
 *				yuv		pointer to the YUV structure
 *
 *  Output :	0		OK
 *				1		wrong dimension
 *				2		memory allocation error
 *
 *	Side Effect :
 *				None
 *
 *	Date :		09/28/2000
 *
 *  Contacts:
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 ************************************************************************/

int RGB2YUV(int x_dim, int y_dim, void *bmp, void *y_out, void *u_out, void *v_out, int flip)
{
    static int init_done = 0;

    long i, j, size;
    unsigned char *r, *g, *b;
    unsigned char *y, *u, *v;
    unsigned char *pu1, *pu2, *pv1, *pv2, *psu, *psv;
    unsigned char *y_buffer, *u_buffer, *v_buffer;
    unsigned char *sub_u_buf, *sub_v_buf;

    if (init_done == 0)
    {
        InitLookupTable();
        init_done = 1;
    }

    /* check to see if x_dim and y_dim are divisible by 2*/
    if ((x_dim % 2) || (y_dim % 2))
        return 1;
    size = x_dim * y_dim;

    /* allocate memory*/
    y_buffer  = (unsigned char *)y_out;
    sub_u_buf = (unsigned char *)u_out;
    sub_v_buf = (unsigned char *)v_out;
    u_buffer  = (unsigned char *)malloc(size * sizeof(unsigned char));
    v_buffer  = (unsigned char *)malloc(size * sizeof(unsigned char));
    if (!(u_buffer && v_buffer))
    {
        if (u_buffer)
            free(u_buffer);
        if (v_buffer)
            free(v_buffer);
        return 2;
    }

    b = (unsigned char *)bmp;
    y = y_buffer;
    u = u_buffer;
    v = v_buffer;

    /* convert RGB to YUV*/
    if (!flip)
    {
        for (j = 0; j < y_dim; j++)
        {
            y = y_buffer + (y_dim - j - 1) * x_dim;
            u = u_buffer + (y_dim - j - 1) * x_dim;
            v = v_buffer + (y_dim - j - 1) * x_dim;

            for (i = 0; i < x_dim; i++)
            {
                g  = b + 1;
                r  = b + 2;
                *y = (unsigned char)(RGBYUV02990[*r] + RGBYUV05870[*g] + RGBYUV01140[*b]);
                *u = (unsigned char)(-RGBYUV01684[*r] - RGBYUV03316[*g] + (*b) / 2 + 128);
                *v = (unsigned char)((*r) / 2 - RGBYUV04187[*g] - RGBYUV00813[*b] + 128);
                b += 3;
                y++;
                u++;
                v++;
            }
        }
    }
    else
    {
        for (i = 0; i < size; i++)
        {
            g  = b + 1;
            r  = b + 2;
            *y = (unsigned char)(RGBYUV02990[*r] + RGBYUV05870[*g] + RGBYUV01140[*b]);
            *u = (unsigned char)(-RGBYUV01684[*r] - RGBYUV03316[*g] + (*b) / 2 + 128);
            *v = (unsigned char)((*r) / 2 - RGBYUV04187[*g] - RGBYUV00813[*b] + 128);
            b += 3;
            y++;
            u++;
            v++;
        }
    }

    /* subsample UV*/
    for (j = 0; j < y_dim / 2; j++)
    {
        psu = sub_u_buf + j * x_dim / 2;
        psv = sub_v_buf + j * x_dim / 2;
        pu1 = u_buffer + 2 * j * x_dim;
        pu2 = u_buffer + (2 * j + 1) * x_dim;
        pv1 = v_buffer + 2 * j * x_dim;
        pv2 = v_buffer + (2 * j + 1) * x_dim;
        for (i = 0; i < x_dim / 2; i++)
        {
            *psu = (*pu1 + *(pu1 + 1) + *pu2 + *(pu2 + 1)) / 4;
            *psv = (*pv1 + *(pv1 + 1) + *pv2 + *(pv2 + 1)) / 4;
            psu++;
            psv++;
            pu1 += 2;
            pu2 += 2;
            pv1 += 2;
            pv2 += 2;
        }
    }

    free(u_buffer);
    free(v_buffer);

    return 0;
}

void InitLookupTable()
{
    int i;

    for (i = 0; i < 256; i++)
        RGBYUV02990[i] = (float)0.2990 * i;
    for (i = 0; i < 256; i++)
        RGBYUV05870[i] = (float)0.5870 * i;
    for (i = 0; i < 256; i++)
        RGBYUV01140[i] = (float)0.1140 * i;
    for (i = 0; i < 256; i++)
        RGBYUV01684[i] = (float)0.1684 * i;
    for (i = 0; i < 256; i++)
        RGBYUV03316[i] = (float)0.3316 * i;
    for (i = 0; i < 256; i++)
        RGBYUV04187[i] = (float)0.4187 * i;
    for (i = 0; i < 256; i++)
        RGBYUV00813[i] = (float)0.0813 * i;
}

/* RGB/BGR to RGB/BGR */

#define RGBBGR_BODY24(TIN, TOUT)                                                      \
    void ccvt_##TIN##_##TOUT(int width, int height, const void *const src, void *dst) \
    {                                                                                 \
        const PIXTYPE_##TIN *in = src;                                                \
        PIXTYPE_##TOUT *out     = dst;                                                \
        int l, c, stride = 0;                                                         \
                                                                                      \
        stride = width;                                                               \
        out += ((height - 1) * width);                                                \
        stride *= 2;                                                                  \
        for (l = 0; l < height; l++)                                                  \
        {                                                                             \
            for (c = 0; c < width; c++)                                               \
            {                                                                         \
                out->r = in->r;                                                       \
                out->g = in->g;                                                       \
                out->b = in->b;                                                       \
                in++;                                                                 \
                out++;                                                                \
            }                                                                         \
            out -= stride;                                                            \
        }                                                                             \
    }

#define RGBBGR_BODY32(TIN, TOUT)                                                      \
    void ccvt_##TIN##_##TOUT(int width, int height, const void *const src, void *dst) \
    {                                                                                 \
        const PIXTYPE_##TIN *in = src;                                                \
        PIXTYPE_##TOUT *out     = dst;                                                \
        int l, c, stride = 0;                                                         \
                                                                                      \
        stride = width;                                                               \
        out += ((height - 1) * width);                                                \
        stride *= 2;                                                                  \
        for (l = 0; l < height; l++)                                                  \
        {                                                                             \
            for (c = 0; c < width; c++)                                               \
            {                                                                         \
                out->r = in->r;                                                       \
                out->g = in->g;                                                       \
                out->b = in->b;                                                       \
                out->z = 0;                                                           \
                in++;                                                                 \
                out++;                                                                \
            }                                                                         \
            out -= stride;                                                            \
        }                                                                             \
    }

RGBBGR_BODY32(bgr24, bgr32)
RGBBGR_BODY32(bgr24, rgb32)
RGBBGR_BODY32(rgb24, bgr32)
RGBBGR_BODY32(rgb24, rgb32)

RGBBGR_BODY24(bgr32, bgr24)
RGBBGR_BODY24(bgr32, rgb24)
RGBBGR_BODY24(rgb32, bgr24)
RGBBGR_BODY24(rgb32, rgb24)
