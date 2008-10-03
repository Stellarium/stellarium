/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_VECTOR_H
#define INCLUDED_LIB3DS_VECTOR_H
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
 * $Id: vector.h,v 1.7 2007/06/14 09:59:10 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include <lib3ds/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern LIB3DSAPI void lib3ds_vector_zero(Lib3dsVector c);
extern LIB3DSAPI void lib3ds_vector_copy(Lib3dsVector dest, Lib3dsVector src);
extern LIB3DSAPI void lib3ds_vector_neg(Lib3dsVector c);
extern LIB3DSAPI void lib3ds_vector_add(Lib3dsVector c, Lib3dsVector a, Lib3dsVector b);
extern LIB3DSAPI void lib3ds_vector_sub(Lib3dsVector c, Lib3dsVector a, Lib3dsVector b);
extern LIB3DSAPI void lib3ds_vector_scalar(Lib3dsVector c, Lib3dsFloat k);
extern LIB3DSAPI void lib3ds_vector_cross(Lib3dsVector c, Lib3dsVector a, Lib3dsVector b);
extern LIB3DSAPI Lib3dsFloat lib3ds_vector_dot(Lib3dsVector a, Lib3dsVector b);
extern LIB3DSAPI Lib3dsFloat lib3ds_vector_squared(Lib3dsVector c);
extern LIB3DSAPI Lib3dsFloat lib3ds_vector_length(Lib3dsVector c);
extern LIB3DSAPI void lib3ds_vector_normalize(Lib3dsVector c);
extern LIB3DSAPI void lib3ds_vector_normal(Lib3dsVector n, Lib3dsVector a,
  Lib3dsVector b, Lib3dsVector c);
extern LIB3DSAPI void lib3ds_vector_transform(Lib3dsVector c, Lib3dsMatrix m, Lib3dsVector a);
extern LIB3DSAPI void lib3ds_vector_cubic(Lib3dsVector c, Lib3dsVector a, Lib3dsVector p,
  Lib3dsVector q, Lib3dsVector b, Lib3dsFloat t);
extern LIB3DSAPI void lib3ds_vector_min(Lib3dsVector c, Lib3dsVector a);
extern LIB3DSAPI void lib3ds_vector_max(Lib3dsVector c, Lib3dsVector a);
extern LIB3DSAPI void lib3ds_vector_dump(Lib3dsVector c);

#ifdef __cplusplus
}
#endif
#endif

