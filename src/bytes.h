// bytes.h
// 
// Copyright (C) 2001, Colin Walters <walters@verbum.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _BYTES_H_
#define _BYTES_H_

#ifndef _WIN32
#include <config.h>
#endif /* _WIN32 */

/* Use the system byteswap.h definitions if we have them */
#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#else
static unsigned short bswap_16 (unsigned short val)
{
  return ((((val) >> 8) & 0xff) | (((val) & 0xff) << 8));
}

static unsigned int bswap_32(unsigned int val) {
  return (((val) & 0xff000000) >> 24) | (((val) & 0x00ff0000) >>  8) |
    (((val) & 0x0000ff00) <<  8) | (((val) & 0x000000ff) << 24);
}
#endif

#define SWAP_FLOAT(x, y) do { *(unsigned int *)&x = bswap_32( *(unsigned int *)&y ); } while (0)

#ifdef WORDS_BIGENDIAN

#define LE_TO_CPU_INT16(ret, val) (ret = bswap_16(val))

#define	LE_TO_CPU_INT32(ret, val) (ret = bswap_32(val))

#define LE_TO_CPU_FLOAT(ret, val) SWAP_FLOAT(ret, val)

#define BE_TO_CPU_INT16(ret, val) (ret = val)

#define BE_TO_CPU_INT32(ret, val) (ret = val)

#define BE_TO_CPU_FLOAT(ret, val) (ret = val)

#else

#define BE_TO_CPU_INT16(ret, val) (ret = bswap_16(val))

#define	BE_TO_CPU_INT32(ret, val) (ret = bswap_32(val))

#define BE_TO_CPU_FLOAT(ret, val) SWAP_FLOAT(ret, val)

#define LE_TO_CPU_INT16(ret, val) (ret = val)

#define LE_TO_CPU_INT32(ret, val) (ret = val)

#define LE_TO_CPU_FLOAT(ret, val) (ret = val)

#endif

#endif // _BYTES_H_
