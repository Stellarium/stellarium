#if 0
    INDI
    Copyright (C) 2003 Elwood C. Downey

    Complete rewrite of to64frombits() - gives 2x the performance
    Complete rewrite of from64tobits_fast() - gives 3x the performance
    Keeping from64tobits() for compatibility - gives 2.5x the performance
    of the old implementation (Aug, 2016 by Rumen G.Bogdanovski)

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Adapted from code written by Eric S. Raymond <esr@snark.thyrsus.com>
#endif

/* Pair of functions to convert to/from base64.
 * Also can be used to build a standalone utility and a loopback test.
 * see http://www.faqs.org/rfcs/rfc3548.html
 */

/** \file base64.c
    \brief Pair of functions to convert to/from base64.

*/

#include <ctype.h>
#include <stdint.h>
#include "base64.h"
#include "base64_luts.h"
#include <stdio.h>

/* convert inlen raw bytes at in to base64 string (NUL-terminated) at out. 
 * out size should be at least 4*inlen/3 + 4.
 * return length of out (sans trailing NUL).
 */
int to64frombits(unsigned char *out, const unsigned char *in, int inlen)
{
    uint16_t *b64lut = (uint16_t *)base64lut;
    int dlen         = ((inlen + 2) / 3) * 4; /* 4/3, rounded up */
    uint16_t *wbuf   = (uint16_t *)out;

    for (; inlen > 2; inlen -= 3)
    {
        uint32_t n = in[0] << 16 | in[1] << 8 | in[2];

        wbuf[0] = b64lut[n >> 12];
        wbuf[1] = b64lut[n & 0x00000fff];

        wbuf += 2;
        in += 3;
    }

    out = (unsigned char *)wbuf;
    if (inlen > 0)
    {
        unsigned char fragment;
        *out++   = base64digits[in[0] >> 2];
        fragment = (in[0] << 4) & 0x30;
        if (inlen > 1)
            fragment |= in[1] >> 4;
        *out++ = base64digits[fragment];
        *out++ = (inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
        *out++ = '=';
    }
    *out = 0; // NULL terminate
    return dlen;
}

/* convert base64 at in to raw bytes out, returning count or <0 on error.
 * base64 should not contain whitespaces.
 * out should be at least 3/4 the length of in.
 */
int from64tobits(char *out, const char *in)
{
    char *cp = (char *)in;
    while (*cp != 0)
        cp += 4;
    return from64tobits_fast(out, in, cp - in);
}

int from64tobits_fast(char *out, const char *in, int inlen)
{
    int outlen = 0;
    uint8_t b1, b2, b3;
    uint16_t s1, s2;
    uint32_t n32;
    int j;
    int n         = (inlen / 4) - 1;
    uint16_t *inp = (uint16_t *)in;

    for (j = 0; j < n; j++)
    {
        if (in[0] == '\n')
            in++;
        inp = (uint16_t *)in;

        s1 = rbase64lut[inp[0]];
        s2 = rbase64lut[inp[1]];

        n32 = s1;
        n32 <<= 10;
        n32 |= s2 >> 2;

        b3 = (n32 & 0x00ff);
        n32 >>= 8;
        b2 = (n32 & 0x00ff);
        n32 >>= 8;
        b1 = (n32 & 0x00ff);

        out[0] = b1;
        out[1] = b2;
        out[2] = b3;

        in += 4;
        out += 3;
    }
    outlen = (inlen / 4 - 1) * 3;
    if (in[0] == '\n')
        in++;
    inp = (uint16_t *)in;

    s1 = rbase64lut[inp[0]];
    s2 = rbase64lut[inp[1]];

    n32 = s1;
    n32 <<= 10;
    n32 |= s2 >> 2;

    b3 = (n32 & 0x00ff);
    n32 >>= 8;
    b2 = (n32 & 0x00ff);
    n32 >>= 8;
    b1 = (n32 & 0x00ff);

    *out++ = b1;
    outlen++;
    if ((inp[1] & 0x00FF) != 0x003D)
    {
        *out++ = b2;
        outlen++;
        if ((inp[1] & 0xFF00) != 0x3D00)
        {
            *out++ = b3;
            outlen++;
        }
    }
    return outlen;
}

#ifdef BASE64_PROGRAM
/* standalone program that converts to/from base64.
 * cc -o base64 -DBASE64_PROGRAM base64.c
 */

#include <stdio.h>
#include <stdlib.h>
u#include <cstring>

static void usage(char *me)
{
    fprintf(stderr, "Purpose: convert stdin to/from base64 on stdout\n");
    fprintf(stderr, "Usage: %s {-t,-f}\n", me);
    exit(1);
}

int main(int ac, char *av[])
{
    int to64 = 1;

    /* decide whether to or from base64 */
    if (ac == 2 && strcmp(av[1], "-f") == 0)
        to64 = 0;
    else if (ac != 1 && (ac != 2 || strcmp(av[1], "-t")))
        usage(av[0]);

    if (to64)
    {
        unsigned char *rawin, *b64;
        int i, n, nrawin, nb64;

        /* read raw on stdin until EOF */
        rawin  = malloc(4096);
        nrawin = 0;
        while ((n = fread(rawin + nrawin, 1, 4096, stdin)) > 0)
            rawin = realloc(rawin, (nrawin += n) + 4096);

        /* convert to base64 */
        b64  = malloc(4 * nrawin / 3 + 4);
        nb64 = to64frombits(b64, rawin, nrawin);

        size_t written = 0;
        size_t towrite = nb64;
        while (written < nb64)
        {
            size_t wr = fwrite(b64 + written, 1, nb64, stdout);
            if (wr > 0)
            {
                towrite -= wr;
                written += wr;
            }
        }
    }
    else
    {
        unsigned char *raw, *b64;
        int n, nraw, nb64;

        /* read base64 on stdin until EOF */
        b64  = malloc(4096);
        nb64 = 0;
        while ((n = fread(b64 + nb64, 1, 4096, stdin)) > 0)
            b64 = realloc(b64, (nb64 += n) + 4096);
        b64[nb64] = '\0';

        /* convert to raw */
        raw  = malloc(3 * nb64 / 4);
        nraw = from64tobits_fast(raw, b64, nb64);
        if (nraw < 0)
        {
            fprintf(stderr, "base64 conversion error: %d\n", nraw);
            return (1);
        }

        /* write */
        fwrite(raw, 1, nraw, stdout);
    }

    return (0);
}

#endif

#ifdef LOOPBACK_TEST
/* standalone test that reads binary on stdin, converts to base64 and back,
 * then compares. exit 0 if compares the same else 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

int main(int ac, char *av[])
{
    unsigned char *rawin, *b64, *rawback;
    int n, nrawin, nrawback, nb64;

    /* read raw on stdin until EOF */
    rawin  = malloc(4096);
    nrawin = 0;
    while ((n = fread(rawin + nrawin, 1, 4096, stdin)) > 0)
        rawin = realloc(rawin, (nrawin += n) + 4096);

    /* convert to base64 */
    b64  = malloc(4 * nrawin * 3 + 4);
    nb64 = to64frombits(b64, rawin, nrawin);

    /* convert back to raw */
    rawback  = malloc(3 * nb64 / 4);
    nrawback = from64tobits_fast(rawback, b64, nrawin);
    if (nrawback < 0)
    {
        fprintf(stderr, "base64 error: %d\n", nrawback);
        return (1);
    }
    if (nrawback != nrawin)
    {
        fprintf(stderr, "base64 back length %d != %d\n", nrawback, nrawin);
        return (1);
    }

    /* compare */
    if (memcmp(rawback, rawin, nrawin))
    {
        fprintf(stderr, "compare error\n");
        return (1);
    }

    /* success */
    return (0);
}
#endif
