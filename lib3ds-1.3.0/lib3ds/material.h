/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_MATERIAL_H
#define INCLUDED_LIB3DS_MATERIAL_H
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
 * $Id: material.h,v 1.18 2007/06/20 17:04:08 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include <lib3ds/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup material 
 */
typedef enum Lib3dsTextureMapFlags {
  LIB3DS_DECALE       =0x0001,
  LIB3DS_MIRROR       =0x0002,
  LIB3DS_NEGATE       =0x0008,
  LIB3DS_NO_TILE      =0x0010,
  LIB3DS_SUMMED_AREA  =0x0020,
  LIB3DS_ALPHA_SOURCE =0x0040,
  LIB3DS_TINT         =0x0080,
  LIB3DS_IGNORE_ALPHA =0x0100,
  LIB3DS_RGB_TINT     =0x0200
} Lib3dsTextureMapFlags;

/**
 * Mateial texture map
 * \ingroup material 
 */
typedef struct Lib3dsTextureMap {
    Lib3dsUserData user;
    char name[64];
    Lib3dsDword flags;
    Lib3dsFloat percent;
    Lib3dsFloat blur;
    Lib3dsFloat scale[2];
    Lib3dsFloat offset[2];
    Lib3dsFloat rotation;
    Lib3dsRgb tint_1;
    Lib3dsRgb tint_2;
    Lib3dsRgb tint_r;
    Lib3dsRgb tint_g;
    Lib3dsRgb tint_b;
} Lib3dsTextureMap;

/**
 * \ingroup material 
 */
typedef enum Lib3dsAutoReflMapFlags {
  LIB3DS_USE_REFL_MAP          =0x0001,
  LIB3DS_READ_FIRST_FRAME_ONLY =0x0004,
  LIB3DS_FLAT_MIRROR           =0x0008 
} Lib3dsAutoReflectionMapFlags;

/**
 * \ingroup material 
 */
typedef enum Lib3dsAutoReflMapAntiAliasLevel {
  LIB3DS_ANTI_ALIAS_NONE   =0,
  LIB3DS_ANTI_ALIAS_LOW    =1,
  LIB3DS_ANTI_ALIAS_MEDIUM =2,
  LIB3DS_ANTI_ALIAS_HIGH   =3
} Lib3dsAutoReflMapAntiAliasLevel;

/**
 * Auto reflection map settings
 * \ingroup material 
 */
typedef struct Lib3dsAutoReflMap {
    Lib3dsDword flags;
    Lib3dsIntd level;
    Lib3dsIntd size;
    Lib3dsIntd frame_step;
} Lib3dsAutoReflMap;

/**
 * \ingroup material 
 */
typedef enum Lib3dsMaterialShading {
  LIB3DS_WIRE_FRAME =0,
  LIB3DS_FLAT       =1, 
  LIB3DS_GOURAUD    =2, 
  LIB3DS_PHONG      =3, 
  LIB3DS_METAL      =4
} Lib3dsMaterialShading; 

/**
 * Material
 * \ingroup material 
 */
struct Lib3dsMaterial {
    Lib3dsUserData user;		/*! Arbitrary user data */
    Lib3dsMaterial *next;
    char name[64];			/*! Material name */
    Lib3dsRgba ambient;			/*! Material ambient reflectivity */
    Lib3dsRgba diffuse;			/*! Material diffuse reflectivity */
    Lib3dsRgba specular;		/*! Material specular reflectivity */
    Lib3dsFloat shininess;		/*! Material specular exponent */
    Lib3dsFloat shin_strength;
    Lib3dsBool use_blur;
    Lib3dsFloat blur;
    Lib3dsFloat transparency;
    Lib3dsFloat falloff;
    Lib3dsBool additive;
    Lib3dsFloat self_ilpct;
    Lib3dsBool use_falloff;
    Lib3dsBool self_illum;
    Lib3dsIntw shading;
    Lib3dsBool soften;
    Lib3dsBool face_map;
    Lib3dsBool two_sided;		/*! Material visible from back */
    Lib3dsBool map_decal;
    Lib3dsBool use_wire;
    Lib3dsBool use_wire_abs;
    Lib3dsFloat wire_size;
    Lib3dsTextureMap texture1_map;
    Lib3dsTextureMap texture1_mask;
    Lib3dsTextureMap texture2_map;
    Lib3dsTextureMap texture2_mask;
    Lib3dsTextureMap opacity_map;
    Lib3dsTextureMap opacity_mask;
    Lib3dsTextureMap bump_map;
    Lib3dsTextureMap bump_mask;
    Lib3dsTextureMap specular_map;
    Lib3dsTextureMap specular_mask;
    Lib3dsTextureMap shininess_map;
    Lib3dsTextureMap shininess_mask;
    Lib3dsTextureMap self_illum_map;
    Lib3dsTextureMap self_illum_mask;
    Lib3dsTextureMap reflection_map;
    Lib3dsTextureMap reflection_mask;
    Lib3dsAutoReflMap autorefl_map;
};

extern LIB3DSAPI Lib3dsMaterial* lib3ds_material_new();
extern LIB3DSAPI void lib3ds_material_free(Lib3dsMaterial *material);
extern LIB3DSAPI void lib3ds_material_dump(Lib3dsMaterial *material);
extern LIB3DSAPI Lib3dsBool lib3ds_material_read(Lib3dsMaterial *material, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_material_write(Lib3dsMaterial *material, Lib3dsIo *io);

#ifdef __cplusplus
}
#endif
#endif



