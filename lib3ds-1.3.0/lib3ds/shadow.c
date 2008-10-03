/*
 * The 3D Studio File Format Library
 * Copyright (C) 1996-2007 by Jan Eric Kyprianidis <www.kyprianidis.com>
 * All rights reserved.
 *
 * This program is  free  software;  you can redistribute it and/or modify it
 * under the terms of the  GNU Lesser General Public License  as published by 
 * the  Free Software Foundation;  either version 2.1 of the License,  or (at 
 * your option) any later version.
 *
 * This  program  is  distributed in  the  hope that it will  be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A  PARTICULAR PURPOSE.  See the  GNU Lesser General Public  
 * License for more details.
 *
 * You should  have received  a copy of the GNU Lesser General Public License
 * along with  this program;  if not, write to the  Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: shadow.c,v 1.11 2007/06/20 17:04:09 jeh Exp $
 */
#include <lib3ds/shadow.h>
#include <lib3ds/chunk.h>
#include <lib3ds/io.h>
#include <math.h>


/*!
 * \defgroup shadow Shadow Map Settings
 */


/*!
 * \ingroup shadow 
 */
Lib3dsBool
lib3ds_shadow_read(Lib3dsShadow *shadow, Lib3dsIo *io)
{
  Lib3dsChunk c;

  if (!lib3ds_chunk_read(&c, io)) {
    return(LIB3DS_FALSE);
  }
  
  switch (c.chunk) {
    case LIB3DS_SHADOW_MAP_SIZE:
      {
        shadow->map_size=lib3ds_io_read_intw(io);
      }
      break;
    case LIB3DS_LO_SHADOW_BIAS:
      {
          shadow->lo_bias=lib3ds_io_read_float(io);
      }
      break;
    case LIB3DS_HI_SHADOW_BIAS:
      {
        shadow->hi_bias=lib3ds_io_read_float(io);
      }
      break;
    case LIB3DS_SHADOW_SAMPLES:
      {
        shadow->samples=lib3ds_io_read_intw(io);
      }
      break;
    case LIB3DS_SHADOW_RANGE:
      {
        shadow->range=lib3ds_io_read_intd(io);
      }
      break;
    case LIB3DS_SHADOW_FILTER:
      {
        shadow->filter=lib3ds_io_read_float(io);
      }
      break;
    case LIB3DS_RAY_BIAS:
      {
        shadow->ray_bias=lib3ds_io_read_float(io);
      }
      break;
  }
  
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup shadow 
 */
Lib3dsBool
lib3ds_shadow_write(Lib3dsShadow *shadow, Lib3dsIo *io)
{
  if (fabs(shadow->lo_bias)>LIB3DS_EPSILON) { /*---- LIB3DS_LO_SHADOW_BIAS ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_LO_SHADOW_BIAS;
    c.size=10;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_float(io, shadow->lo_bias);
  }

  if (fabs(shadow->hi_bias)>LIB3DS_EPSILON) { /*---- LIB3DS_HI_SHADOW_BIAS ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_HI_SHADOW_BIAS;
    c.size=10;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_float(io, shadow->hi_bias);
  }

  if (shadow->map_size) { /*---- LIB3DS_SHADOW_MAP_SIZE ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_SHADOW_MAP_SIZE;
    c.size=8;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_intw(io, shadow->map_size);
  }
  
  if (shadow->samples) { /*---- LIB3DS_SHADOW_SAMPLES ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_SHADOW_SAMPLES;
    c.size=8;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_intw(io, shadow->samples);
  }

  if (shadow->range) { /*---- LIB3DS_SHADOW_RANGE ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_SHADOW_RANGE;
    c.size=10;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_intd(io, shadow->range);
  }

  if (fabs(shadow->filter)>LIB3DS_EPSILON) { /*---- LIB3DS_SHADOW_FILTER ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_SHADOW_FILTER;
    c.size=10;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_float(io, shadow->filter);
  }
  if (fabs(shadow->ray_bias)>LIB3DS_EPSILON) { /*---- LIB3DS_RAY_BIAS ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_RAY_BIAS;
    c.size=10;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_float(io, shadow->ray_bias);
  }
  return(LIB3DS_TRUE);
}

