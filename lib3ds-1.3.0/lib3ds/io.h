/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_IO_H
#define INCLUDED_LIB3DS_IO_H
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
 * $Id: io.h,v 1.6 2007/06/20 17:04:08 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include <lib3ds/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Lib3dsIoSeek {
  LIB3DS_SEEK_SET  =0,
  LIB3DS_SEEK_CUR  =1,
  LIB3DS_SEEK_END  =2
} Lib3dsIoSeek;
  
typedef Lib3dsBool (*Lib3dsIoErrorFunc)(void *self);
typedef long (*Lib3dsIoSeekFunc)(void *self, long offset, Lib3dsIoSeek origin);
typedef long (*Lib3dsIoTellFunc)(void *self);
typedef size_t (*Lib3dsIoReadFunc)(void *self, void *buffer, size_t size);
typedef size_t (*Lib3dsIoWriteFunc)(void *self, const void *buffer, size_t size);

extern LIB3DSAPI Lib3dsIo* lib3ds_io_new(void *self, Lib3dsIoErrorFunc error_func,
  Lib3dsIoSeekFunc seek_func, Lib3dsIoTellFunc tell_func,
  Lib3dsIoReadFunc read_func, Lib3dsIoWriteFunc write_func);
extern LIB3DSAPI void lib3ds_io_free(Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_io_error(Lib3dsIo *io);
extern LIB3DSAPI long lib3ds_io_seek(Lib3dsIo *io, long offset, Lib3dsIoSeek origin);
extern LIB3DSAPI long lib3ds_io_tell(Lib3dsIo *io);
extern LIB3DSAPI size_t lib3ds_io_read(Lib3dsIo *io, void *buffer, size_t size);
extern LIB3DSAPI size_t lib3ds_io_write(Lib3dsIo *io, const void *buffer, size_t size);

extern LIB3DSAPI Lib3dsByte lib3ds_io_read_byte(Lib3dsIo *io);
extern LIB3DSAPI Lib3dsWord lib3ds_io_read_word(Lib3dsIo *io);
extern LIB3DSAPI Lib3dsDword lib3ds_io_read_dword(Lib3dsIo *io);
extern LIB3DSAPI Lib3dsIntb lib3ds_io_read_intb(Lib3dsIo *io);
extern LIB3DSAPI Lib3dsIntw lib3ds_io_read_intw(Lib3dsIo *io);
extern LIB3DSAPI Lib3dsIntd lib3ds_io_read_intd(Lib3dsIo *io);
extern LIB3DSAPI Lib3dsFloat lib3ds_io_read_float(Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_io_read_vector(Lib3dsIo *io, Lib3dsVector v);
extern LIB3DSAPI Lib3dsBool lib3ds_io_read_rgb(Lib3dsIo *io, Lib3dsRgb rgb);
extern LIB3DSAPI Lib3dsBool lib3ds_io_read_string(Lib3dsIo *io, char *s, int buflen);

extern LIB3DSAPI Lib3dsBool lib3ds_io_write_byte(Lib3dsIo *io, Lib3dsByte b);
extern LIB3DSAPI Lib3dsBool lib3ds_io_write_word(Lib3dsIo *io, Lib3dsWord w);
extern LIB3DSAPI Lib3dsBool lib3ds_io_write_dword(Lib3dsIo *io, Lib3dsDword d);
extern LIB3DSAPI Lib3dsBool lib3ds_io_write_intb(Lib3dsIo *io, Lib3dsIntb b);
extern LIB3DSAPI Lib3dsBool lib3ds_io_write_intw(Lib3dsIo *io, Lib3dsIntw w);
extern LIB3DSAPI Lib3dsBool lib3ds_io_write_intd(Lib3dsIo *io, Lib3dsIntd d);
extern LIB3DSAPI Lib3dsBool lib3ds_io_write_float(Lib3dsIo *io, Lib3dsFloat l);
extern LIB3DSAPI Lib3dsBool lib3ds_io_write_vector(Lib3dsIo *io, Lib3dsVector v);
extern LIB3DSAPI Lib3dsBool lib3ds_io_write_rgb(Lib3dsIo *io, Lib3dsRgb rgb);
extern LIB3DSAPI Lib3dsBool lib3ds_io_write_string(Lib3dsIo *io, const char *s);

#ifdef __cplusplus
}
#endif
#endif

