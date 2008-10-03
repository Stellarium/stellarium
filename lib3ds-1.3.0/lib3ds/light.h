/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_LIGHT_H
#define INCLUDED_LIB3DS_LIGHT_H
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
 * $Id: light.h,v 1.13 2007/06/20 17:04:08 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include <lib3ds/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Light
 * \ingroup light
 */
struct Lib3dsLight {
    Lib3dsLight *next;
    char name[64];
    Lib3dsDword object_flags; /*< @see Lib3dsObjectFlags */ 
    Lib3dsBool spot_light;
    Lib3dsBool see_cone;
    Lib3dsRgb color;
    Lib3dsVector position;
    Lib3dsVector spot;
    Lib3dsFloat roll;
    Lib3dsBool off;
    Lib3dsFloat outer_range;
    Lib3dsFloat inner_range;
    Lib3dsFloat multiplier;
    /*const char** excludes;*/
    Lib3dsFloat attenuation;
    Lib3dsBool rectangular_spot;
    Lib3dsBool shadowed;
    Lib3dsFloat shadow_bias;
    Lib3dsFloat shadow_filter;
    Lib3dsIntw shadow_size;
    Lib3dsFloat spot_aspect;
    Lib3dsBool use_projector;
    char projector[64];
    Lib3dsIntd spot_overshoot;
    Lib3dsBool ray_shadows;
    Lib3dsFloat ray_bias;
    Lib3dsFloat hot_spot;
    Lib3dsFloat fall_off;
}; 

extern LIB3DSAPI Lib3dsLight* lib3ds_light_new(const char *name);
extern LIB3DSAPI void lib3ds_light_free(Lib3dsLight *mesh);
extern LIB3DSAPI void lib3ds_light_dump(Lib3dsLight *light);
extern LIB3DSAPI Lib3dsBool lib3ds_light_read(Lib3dsLight *light, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_light_write(Lib3dsLight *light, Lib3dsIo *io);

#ifdef __cplusplus
}
#endif
#endif

