/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_CHUNK_H
#define INCLUDED_LIB3DS_CHUNK_H
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
 * $Id: chunk.h,v 1.16 2007/06/20 17:04:08 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include <lib3ds/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _Lib3dsChunks {
  LIB3DS_NULL_CHUNK             =0x0000,
  LIB3DS_M3DMAGIC               =0x4D4D,    /*3DS file*/
  LIB3DS_SMAGIC                 =0x2D2D,    
  LIB3DS_LMAGIC                 =0x2D3D,    
  LIB3DS_MLIBMAGIC              =0x3DAA,    /*MLI file*/
  LIB3DS_MATMAGIC               =0x3DFF,    
  LIB3DS_CMAGIC                 =0xC23D,    /*PRJ file*/
  LIB3DS_M3D_VERSION            =0x0002,
  LIB3DS_M3D_KFVERSION          =0x0005,

  LIB3DS_COLOR_F                =0x0010,
  LIB3DS_COLOR_24               =0x0011,
  LIB3DS_LIN_COLOR_24           =0x0012,
  LIB3DS_LIN_COLOR_F            =0x0013,
  LIB3DS_INT_PERCENTAGE         =0x0030,
  LIB3DS_FLOAT_PERCENTAGE       =0x0031,

  LIB3DS_MDATA                  =0x3D3D,
  LIB3DS_MESH_VERSION           =0x3D3E,
  LIB3DS_MASTER_SCALE           =0x0100,
  LIB3DS_LO_SHADOW_BIAS         =0x1400,
  LIB3DS_HI_SHADOW_BIAS         =0x1410,
  LIB3DS_SHADOW_MAP_SIZE        =0x1420,
  LIB3DS_SHADOW_SAMPLES         =0x1430,
  LIB3DS_SHADOW_RANGE           =0x1440,
  LIB3DS_SHADOW_FILTER          =0x1450,
  LIB3DS_RAY_BIAS               =0x1460,
  LIB3DS_O_CONSTS               =0x1500,
  LIB3DS_AMBIENT_LIGHT          =0x2100,
  LIB3DS_BIT_MAP                =0x1100,
  LIB3DS_SOLID_BGND             =0x1200,
  LIB3DS_V_GRADIENT             =0x1300,
  LIB3DS_USE_BIT_MAP            =0x1101,
  LIB3DS_USE_SOLID_BGND         =0x1201,
  LIB3DS_USE_V_GRADIENT         =0x1301,
  LIB3DS_FOG                    =0x2200,
  LIB3DS_FOG_BGND               =0x2210,
  LIB3DS_LAYER_FOG              =0x2302,
  LIB3DS_DISTANCE_CUE           =0x2300,
  LIB3DS_DCUE_BGND              =0x2310,
  LIB3DS_USE_FOG                =0x2201,
  LIB3DS_USE_LAYER_FOG          =0x2303,
  LIB3DS_USE_DISTANCE_CUE       =0x2301,

  LIB3DS_MAT_ENTRY              =0xAFFF,
  LIB3DS_MAT_NAME               =0xA000,
  LIB3DS_MAT_AMBIENT            =0xA010,
  LIB3DS_MAT_DIFFUSE            =0xA020,
  LIB3DS_MAT_SPECULAR           =0xA030,
  LIB3DS_MAT_SHININESS          =0xA040,
  LIB3DS_MAT_SHIN2PCT           =0xA041,
  LIB3DS_MAT_TRANSPARENCY       =0xA050,
  LIB3DS_MAT_XPFALL             =0xA052,
  LIB3DS_MAT_USE_XPFALL         =0xA240,
  LIB3DS_MAT_REFBLUR            =0xA053,
  LIB3DS_MAT_SHADING            =0xA100,
  LIB3DS_MAT_USE_REFBLUR        =0xA250,
  LIB3DS_MAT_SELF_ILLUM         =0xA080,
  LIB3DS_MAT_TWO_SIDE           =0xA081,
  LIB3DS_MAT_DECAL              =0xA082,
  LIB3DS_MAT_ADDITIVE           =0xA083,
  LIB3DS_MAT_SELF_ILPCT         =0xA084,
  LIB3DS_MAT_WIRE               =0xA085,
  LIB3DS_MAT_FACEMAP            =0xA088,
  LIB3DS_MAT_PHONGSOFT          =0xA08C,
  LIB3DS_MAT_WIREABS            =0xA08E,
  LIB3DS_MAT_WIRE_SIZE          =0xA087,
  LIB3DS_MAT_TEXMAP             =0xA200,
  LIB3DS_MAT_SXP_TEXT_DATA      =0xA320,
  LIB3DS_MAT_TEXMASK            =0xA33E,
  LIB3DS_MAT_SXP_TEXTMASK_DATA  =0xA32A,
  LIB3DS_MAT_TEX2MAP            =0xA33A,
  LIB3DS_MAT_SXP_TEXT2_DATA     =0xA321,
  LIB3DS_MAT_TEX2MASK           =0xA340,
  LIB3DS_MAT_SXP_TEXT2MASK_DATA =0xA32C,
  LIB3DS_MAT_OPACMAP            =0xA210,
  LIB3DS_MAT_SXP_OPAC_DATA      =0xA322,
  LIB3DS_MAT_OPACMASK           =0xA342,
  LIB3DS_MAT_SXP_OPACMASK_DATA  =0xA32E,
  LIB3DS_MAT_BUMPMAP            =0xA230,
  LIB3DS_MAT_SXP_BUMP_DATA      =0xA324,
  LIB3DS_MAT_BUMPMASK           =0xA344,
  LIB3DS_MAT_SXP_BUMPMASK_DATA  =0xA330,
  LIB3DS_MAT_SPECMAP            =0xA204,
  LIB3DS_MAT_SXP_SPEC_DATA      =0xA325,
  LIB3DS_MAT_SPECMASK           =0xA348,
  LIB3DS_MAT_SXP_SPECMASK_DATA  =0xA332,
  LIB3DS_MAT_SHINMAP            =0xA33C,
  LIB3DS_MAT_SXP_SHIN_DATA      =0xA326,
  LIB3DS_MAT_SHINMASK           =0xA346,
  LIB3DS_MAT_SXP_SHINMASK_DATA  =0xA334,
  LIB3DS_MAT_SELFIMAP           =0xA33D,
  LIB3DS_MAT_SXP_SELFI_DATA     =0xA328,
  LIB3DS_MAT_SELFIMASK          =0xA34A,
  LIB3DS_MAT_SXP_SELFIMASK_DATA =0xA336,
  LIB3DS_MAT_REFLMAP            =0xA220,
  LIB3DS_MAT_REFLMASK           =0xA34C,
  LIB3DS_MAT_SXP_REFLMASK_DATA  =0xA338,
  LIB3DS_MAT_ACUBIC             =0xA310,
  LIB3DS_MAT_MAPNAME            =0xA300,
  LIB3DS_MAT_MAP_TILING         =0xA351,
  LIB3DS_MAT_MAP_TEXBLUR        =0xA353,
  LIB3DS_MAT_MAP_USCALE         =0xA354,
  LIB3DS_MAT_MAP_VSCALE         =0xA356,
  LIB3DS_MAT_MAP_UOFFSET        =0xA358,
  LIB3DS_MAT_MAP_VOFFSET        =0xA35A,
  LIB3DS_MAT_MAP_ANG            =0xA35C,
  LIB3DS_MAT_MAP_COL1           =0xA360,
  LIB3DS_MAT_MAP_COL2           =0xA362,
  LIB3DS_MAT_MAP_RCOL           =0xA364,
  LIB3DS_MAT_MAP_GCOL           =0xA366,
  LIB3DS_MAT_MAP_BCOL           =0xA368,

  LIB3DS_NAMED_OBJECT           =0x4000,
  LIB3DS_N_DIRECT_LIGHT         =0x4600,
  LIB3DS_DL_OFF                 =0x4620,
  LIB3DS_DL_OUTER_RANGE         =0x465A,
  LIB3DS_DL_INNER_RANGE         =0x4659,
  LIB3DS_DL_MULTIPLIER          =0x465B,
  LIB3DS_DL_EXCLUDE             =0x4654,
  LIB3DS_DL_ATTENUATE           =0x4625,
  LIB3DS_DL_SPOTLIGHT           =0x4610,
  LIB3DS_DL_SPOT_ROLL           =0x4656,
  LIB3DS_DL_SHADOWED            =0x4630,
  LIB3DS_DL_LOCAL_SHADOW2       =0x4641,
  LIB3DS_DL_SEE_CONE            =0x4650,
  LIB3DS_DL_SPOT_RECTANGULAR    =0x4651,
  LIB3DS_DL_SPOT_ASPECT         =0x4657,
  LIB3DS_DL_SPOT_PROJECTOR      =0x4653,
  LIB3DS_DL_SPOT_OVERSHOOT      =0x4652,
  LIB3DS_DL_RAY_BIAS            =0x4658,
  LIB3DS_DL_RAYSHAD             =0x4627,
  LIB3DS_N_CAMERA               =0x4700,
  LIB3DS_CAM_SEE_CONE           =0x4710,
  LIB3DS_CAM_RANGES             =0x4720,
  LIB3DS_OBJ_HIDDEN             =0x4010,
  LIB3DS_OBJ_VIS_LOFTER         =0x4011,
  LIB3DS_OBJ_DOESNT_CAST        =0x4012,
  LIB3DS_OBJ_DONT_RCVSHADOW     =0x4017,
  LIB3DS_OBJ_MATTE              =0x4013,
  LIB3DS_OBJ_FAST               =0x4014,
  LIB3DS_OBJ_PROCEDURAL         =0x4015,
  LIB3DS_OBJ_FROZEN             =0x4016,
  LIB3DS_N_TRI_OBJECT           =0x4100,
  LIB3DS_POINT_ARRAY            =0x4110,
  LIB3DS_POINT_FLAG_ARRAY       =0x4111,
  LIB3DS_FACE_ARRAY             =0x4120,
  LIB3DS_MSH_MAT_GROUP          =0x4130,
  LIB3DS_SMOOTH_GROUP           =0x4150,
  LIB3DS_MSH_BOXMAP             =0x4190,
  LIB3DS_TEX_VERTS              =0x4140,
  LIB3DS_MESH_MATRIX            =0x4160,
  LIB3DS_MESH_COLOR             =0x4165,
  LIB3DS_MESH_TEXTURE_INFO      =0x4170,

  LIB3DS_KFDATA                 =0xB000,
  LIB3DS_KFHDR                  =0xB00A,
  LIB3DS_KFSEG                  =0xB008,
  LIB3DS_KFCURTIME              =0xB009,
  LIB3DS_AMBIENT_NODE_TAG       =0xB001,
  LIB3DS_OBJECT_NODE_TAG        =0xB002,
  LIB3DS_CAMERA_NODE_TAG        =0xB003,
  LIB3DS_TARGET_NODE_TAG        =0xB004,
  LIB3DS_LIGHT_NODE_TAG         =0xB005,
  LIB3DS_L_TARGET_NODE_TAG      =0xB006,
  LIB3DS_SPOTLIGHT_NODE_TAG     =0xB007,
  LIB3DS_NODE_ID                =0xB030,
  LIB3DS_NODE_HDR               =0xB010,
  LIB3DS_PIVOT                  =0xB013,
  LIB3DS_INSTANCE_NAME          =0xB011,
  LIB3DS_MORPH_SMOOTH           =0xB015,
  LIB3DS_BOUNDBOX               =0xB014,
  LIB3DS_POS_TRACK_TAG          =0xB020,
  LIB3DS_COL_TRACK_TAG          =0xB025,
  LIB3DS_ROT_TRACK_TAG          =0xB021,
  LIB3DS_SCL_TRACK_TAG          =0xB022,
  LIB3DS_MORPH_TRACK_TAG        =0xB026,
  LIB3DS_FOV_TRACK_TAG          =0xB023,
  LIB3DS_ROLL_TRACK_TAG         =0xB024,
  LIB3DS_HOT_TRACK_TAG          =0xB027,
  LIB3DS_FALL_TRACK_TAG         =0xB028,
  LIB3DS_HIDE_TRACK_TAG         =0xB029,

  LIB3DS_POLY_2D                = 0x5000,
  LIB3DS_SHAPE_OK               = 0x5010,
  LIB3DS_SHAPE_NOT_OK           = 0x5011,
  LIB3DS_SHAPE_HOOK             = 0x5020,
  LIB3DS_PATH_3D                = 0x6000,
  LIB3DS_PATH_MATRIX            = 0x6005,
  LIB3DS_SHAPE_2D               = 0x6010,
  LIB3DS_M_SCALE                = 0x6020,
  LIB3DS_M_TWIST                = 0x6030,
  LIB3DS_M_TEETER               = 0x6040,
  LIB3DS_M_FIT                  = 0x6050,
  LIB3DS_M_BEVEL                = 0x6060,
  LIB3DS_XZ_CURVE               = 0x6070,
  LIB3DS_YZ_CURVE               = 0x6080,
  LIB3DS_INTERPCT               = 0x6090,
  LIB3DS_DEFORM_LIMIT           = 0x60A0,

  LIB3DS_USE_CONTOUR            = 0x6100,
  LIB3DS_USE_TWEEN              = 0x6110,
  LIB3DS_USE_SCALE              = 0x6120,
  LIB3DS_USE_TWIST              = 0x6130,
  LIB3DS_USE_TEETER             = 0x6140,
  LIB3DS_USE_FIT                = 0x6150,
  LIB3DS_USE_BEVEL              = 0x6160,

  LIB3DS_DEFAULT_VIEW           = 0x3000,
  LIB3DS_VIEW_TOP               = 0x3010,
  LIB3DS_VIEW_BOTTOM            = 0x3020,
  LIB3DS_VIEW_LEFT              = 0x3030,
  LIB3DS_VIEW_RIGHT             = 0x3040,
  LIB3DS_VIEW_FRONT             = 0x3050,
  LIB3DS_VIEW_BACK              = 0x3060,
  LIB3DS_VIEW_USER              = 0x3070,
  LIB3DS_VIEW_CAMERA            = 0x3080,
  LIB3DS_VIEW_WINDOW            = 0x3090,

  LIB3DS_VIEWPORT_LAYOUT_OLD    = 0x7000,
  LIB3DS_VIEWPORT_DATA_OLD      = 0x7010,
  LIB3DS_VIEWPORT_LAYOUT        = 0x7001,
  LIB3DS_VIEWPORT_DATA          = 0x7011,
  LIB3DS_VIEWPORT_DATA_3        = 0x7012,
  LIB3DS_VIEWPORT_SIZE          = 0x7020,
  LIB3DS_NETWORK_VIEW           = 0x7030
} Lib3dsChunks;

typedef struct Lib3dsChunk {
    Lib3dsWord chunk;
    Lib3dsDword size;
    Lib3dsDword end;
    Lib3dsDword cur;
} Lib3dsChunk; 

extern LIB3DSAPI void lib3ds_chunk_enable_dump(Lib3dsBool enable, Lib3dsBool unknown);
extern LIB3DSAPI Lib3dsBool lib3ds_chunk_read(Lib3dsChunk *c, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_chunk_read_start(Lib3dsChunk *c, Lib3dsWord chunk, Lib3dsIo *io);
extern LIB3DSAPI void lib3ds_chunk_read_tell(Lib3dsChunk *c, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsWord lib3ds_chunk_read_next(Lib3dsChunk *c, Lib3dsIo *io);
extern LIB3DSAPI void lib3ds_chunk_read_reset(Lib3dsChunk *c, Lib3dsIo *io);
extern LIB3DSAPI void lib3ds_chunk_read_end(Lib3dsChunk *c, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_chunk_write(Lib3dsChunk *c, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_chunk_write_start(Lib3dsChunk *c, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_chunk_write_end(Lib3dsChunk *c, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_chunk_write_switch(Lib3dsWord chunk, Lib3dsIo *io);
extern LIB3DSAPI const char* lib3ds_chunk_name(Lib3dsWord chunk);
extern LIB3DSAPI void lib3ds_chunk_unknown(Lib3dsWord chunk);
extern LIB3DSAPI void lib3ds_chunk_dump_info(const char *format, ...);

#ifdef __cplusplus
}
#endif
#endif


