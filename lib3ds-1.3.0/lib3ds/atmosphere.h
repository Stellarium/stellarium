/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_ATMOSPHERE_H
#define INCLUDED_LIB3DS_ATMOSPHERE_H
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
 * $Id: atmosphere.h,v 1.8 2007/06/20 17:04:08 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include <lib3ds/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Fog atmosphere settings
 * \ingroup atmosphere
 */
typedef struct Lib3dsFog {
    Lib3dsBool use;
    Lib3dsRgb col;
    Lib3dsBool fog_background;
    Lib3dsFloat near_plane;
    Lib3dsFloat near_density;
    Lib3dsFloat far_plane;
    Lib3dsFloat far_density;
} Lib3dsFog;

/**
 * Layer fog atmosphere flags
 * \ingroup atmosphere
 */
typedef enum Lib3dsLayerFogFlags {
  LIB3DS_BOTTOM_FALL_OFF =0x00000001,
  LIB3DS_TOP_FALL_OFF    =0x00000002,
  LIB3DS_FOG_BACKGROUND  =0x00100000
} Lib3dsLayerFogFlags;

/**
 * Layer fog atmosphere settings
 * \ingroup atmosphere
 */
typedef struct Lib3dsLayerFog {
    Lib3dsBool use;
    Lib3dsDword flags;
    Lib3dsRgb col;
    Lib3dsFloat near_y;
    Lib3dsFloat far_y;
    Lib3dsFloat density;
} Lib3dsLayerFog;

/**
 * Distance cue atmosphere settings
 * \ingroup atmosphere
 */
typedef struct Lib3dsDistanceCue {
    Lib3dsBool use;
    Lib3dsBool cue_background;
    Lib3dsFloat near_plane;
    Lib3dsFloat near_dimming;
    Lib3dsFloat far_plane;
    Lib3dsFloat far_dimming;
} Lib3dsDistanceCue;

/**
 * Atmosphere settings
 * \ingroup atmosphere
 */
struct Lib3dsAtmosphere {
    Lib3dsFog fog;
    Lib3dsLayerFog layer_fog;
    Lib3dsDistanceCue dist_cue;
};

extern LIB3DSAPI Lib3dsBool lib3ds_atmosphere_read(Lib3dsAtmosphere *atmosphere, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_atmosphere_write(Lib3dsAtmosphere *atmosphere, Lib3dsIo *io);

#ifdef __cplusplus
}
#endif
#endif
