/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_TCB_H
#define INCLUDED_LIB3DS_TCB_H
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
 * $Id: tcb.h,v 1.11 2007/06/20 17:04:09 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include <lib3ds/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Lib3dsTcbFlags{
  LIB3DS_USE_TENSION    =0x0001,
  LIB3DS_USE_CONTINUITY =0x0002,
  LIB3DS_USE_BIAS       =0x0004,
  LIB3DS_USE_EASE_TO    =0x0008,
  LIB3DS_USE_EASE_FROM  =0x0010
} Lib3dsTcbFlags;

typedef struct Lib3dsTcb {
    Lib3dsIntd frame;
    Lib3dsWord flags;
    Lib3dsFloat tens;
    Lib3dsFloat cont;
    Lib3dsFloat bias;
    Lib3dsFloat ease_to;
    Lib3dsFloat ease_from;
} Lib3dsTcb;

extern LIB3DSAPI void lib3ds_tcb(Lib3dsTcb *p, Lib3dsTcb *pc, Lib3dsTcb *c,
  Lib3dsTcb *nc, Lib3dsTcb *n, Lib3dsFloat *ksm, Lib3dsFloat *ksp,
  Lib3dsFloat *kdm, Lib3dsFloat *kdp);
extern LIB3DSAPI Lib3dsBool lib3ds_tcb_read(Lib3dsTcb *tcb, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_tcb_write(Lib3dsTcb *tcb, Lib3dsIo *io);

#ifdef __cplusplus
}
#endif
#endif

