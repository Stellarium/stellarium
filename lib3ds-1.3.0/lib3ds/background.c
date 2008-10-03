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
 * $Id: background.c,v 1.12 2007/06/20 17:04:08 jeh Exp $
 */
#include <lib3ds/background.h>
#include <lib3ds/chunk.h>
#include <lib3ds/io.h>
#include <string.h>
#include <math.h>


/*!
 * \defgroup background Background Settings
 */


static Lib3dsBool
solid_bgnd_read(Lib3dsBackground *background, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;
  Lib3dsBool have_lin=LIB3DS_FALSE;
          
  if (!lib3ds_chunk_read_start(&c, LIB3DS_SOLID_BGND, io)) {
    return(LIB3DS_FALSE);
  }

  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_LIN_COLOR_F:
        lib3ds_io_read_rgb(io, background->solid.col);
        have_lin=LIB3DS_TRUE;
        break;
      case LIB3DS_COLOR_F:
        lib3ds_io_read_rgb(io, background->solid.col);
        break;
      default:
        lib3ds_chunk_unknown(chunk);
    }
  }
  
  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


static Lib3dsBool
v_gradient_read(Lib3dsBackground *background, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;
  int index[2];
  Lib3dsRgb col[2][3];
  int have_lin=0;
  

  if (!lib3ds_chunk_read_start(&c, LIB3DS_V_GRADIENT, io)) {
    return(LIB3DS_FALSE);
  }
  background->gradient.percent=lib3ds_io_read_float(io);
  lib3ds_chunk_read_tell(&c, io);

  index[0]=index[1]=0;
  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_COLOR_F:
        lib3ds_io_read_rgb(io, col[0][index[0]]);
        index[0]++;
        break;
      case LIB3DS_LIN_COLOR_F:
        lib3ds_io_read_rgb(io, col[1][index[1]]);
        index[1]++;
        have_lin=1;
        break;
      default:
        lib3ds_chunk_unknown(chunk);
    }
  }
  {
    int i;
    for (i=0; i<3; ++i) {
      background->gradient.top[i]=col[have_lin][0][i];
      background->gradient.middle[i]=col[have_lin][1][i];
      background->gradient.bottom[i]=col[have_lin][2][i];
    }
  }
  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup background
 */
Lib3dsBool
lib3ds_background_read(Lib3dsBackground *background, Lib3dsIo *io)
{
  Lib3dsChunk c;

  if (!lib3ds_chunk_read(&c, io)) {
    return(LIB3DS_FALSE);
  }
  
  switch (c.chunk) {
    case LIB3DS_BIT_MAP:
      {
        if (!lib3ds_io_read_string(io, background->bitmap.name, 64)) {
            return(LIB3DS_FALSE);
        }
      }
      break;
    case LIB3DS_SOLID_BGND:
      {
        lib3ds_chunk_read_reset(&c, io);
        if (!solid_bgnd_read(background, io)) {
          return(LIB3DS_FALSE);
        }
      }
      break;
    case LIB3DS_V_GRADIENT:
      {
        lib3ds_chunk_read_reset(&c, io);
        if (!v_gradient_read(background, io)) {
          return(LIB3DS_FALSE);
        }
      }
      break;
    case LIB3DS_USE_BIT_MAP:
      {
        background->bitmap.use=LIB3DS_TRUE;
      }
      break;
    case LIB3DS_USE_SOLID_BGND:
      {
        background->solid.use=LIB3DS_TRUE;
      }
      break;
    case LIB3DS_USE_V_GRADIENT:
      {
        background->gradient.use=LIB3DS_TRUE;
      }
      break;
  }
  
  return(LIB3DS_TRUE);
}


static Lib3dsBool
colorf_write(Lib3dsRgba rgb, Lib3dsIo *io)
{
  Lib3dsChunk c;

  c.chunk=LIB3DS_COLOR_F;
  c.size=18;
  lib3ds_chunk_write(&c,io);
  lib3ds_io_write_rgb(io, rgb);

  c.chunk=LIB3DS_LIN_COLOR_F;
  c.size=18;
  lib3ds_chunk_write(&c,io);
  lib3ds_io_write_rgb(io, rgb);
  return(LIB3DS_TRUE);
}


static Lib3dsBool
colorf_defined(Lib3dsRgba rgb)
{
  int i;
  for (i=0; i<3; ++i) {
    if (fabs(rgb[i])>LIB3DS_EPSILON) {
      break;
    }
  }
  return(i<3);
}


/*!
 * \ingroup background
 */
Lib3dsBool
lib3ds_background_write(Lib3dsBackground *background, Lib3dsIo *io)
{
  if (strlen(background->bitmap.name)) { /*---- LIB3DS_BIT_MAP ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_BIT_MAP;
    c.size=6+1+(Lib3dsDword)strlen(background->bitmap.name);
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_string(io, background->bitmap.name);
  }

  if (colorf_defined(background->solid.col)) { /*---- LIB3DS_SOLID_BGND ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_SOLID_BGND;
    c.size=42;
    lib3ds_chunk_write(&c,io);
    colorf_write(background->solid.col, io);
  }

  if (colorf_defined(background->gradient.top) ||
    colorf_defined(background->gradient.middle) ||
    colorf_defined(background->gradient.bottom)) { /*---- LIB3DS_V_GRADIENT ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_V_GRADIENT;
    c.size=118;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_float(io, background->gradient.percent);
    colorf_write(background->gradient.top,io);
    colorf_write(background->gradient.middle,io);
    colorf_write(background->gradient.bottom,io);
  }

  if (background->bitmap.use) { /*---- LIB3DS_USE_BIT_MAP ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_USE_BIT_MAP;
    c.size=6;
    lib3ds_chunk_write(&c,io);
  }

  if (background->solid.use) { /*---- LIB3DS_USE_SOLID_BGND ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_USE_SOLID_BGND;
    c.size=6;
    lib3ds_chunk_write(&c,io);
  }

  if (background->gradient.use) { /*---- LIB3DS_USE_V_GRADIENT ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_USE_V_GRADIENT;
    c.size=6;
    lib3ds_chunk_write(&c,io);
  }
  
  return(LIB3DS_TRUE);
}

