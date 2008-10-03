/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_MESH_H
#define INCLUDED_LIB3DS_MESH_H
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
 * $Id: mesh.h,v 1.20 2007/06/20 17:04:08 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include <lib3ds/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Triangular mesh point
 * \ingroup mesh
 */
typedef struct Lib3dsPoint {
    Lib3dsVector pos;
} Lib3dsPoint;

/**
 * Triangular mesh face
 * \ingroup mesh
 * \sa Lib3dsFaceFlag
 */
struct Lib3dsFace {
    Lib3dsUserData user;	/*! Arbitrary user data */
    char material[64];		/*! Material name */
    Lib3dsWord points[3];	/*! Indices into mesh points list */
    Lib3dsWord flags;		/*! See Lib3dsFaceFlag, below */
    Lib3dsDword smoothing;	/*! Bitmask; each bit identifies a group */
    Lib3dsVector normal;
};


/**
 * Vertex flags
 * Meaning of _Lib3dsFace::flags. ABC are points of the current face 
 * (A: is 1st vertex, B is 2nd vertex, C is 3rd vertex) 
 */
typedef enum {
  LIB3DS_FACE_FLAG_VIS_AC = 0x1,       /*!< Bit 0: Edge visibility AC */
  LIB3DS_FACE_FLAG_VIS_BC = 0x2,       /*!< Bit 1: Edge visibility BC */
  LIB3DS_FACE_FLAG_VIS_AB = 0x4,       /*!< Bit 2: Edge visibility AB */
  LIB3DS_FACE_FLAG_WRAP_U = 0x8,       /*!< Bit 3: Face is at tex U wrap seam */
  LIB3DS_FACE_FLAG_WRAP_V = 0x10,      /*!< Bit 4: Face is at tex V wrap seam */
  LIB3DS_FACE_FLAG_UNK7 = 0x80,        /* Bit 5-8: Unused ? */
  LIB3DS_FACE_FLAG_UNK10 = 0x400,      /* Bit 9-10: Random ? */
                                       /* Bit 11-12: Unused ? */
  LIB3DS_FACE_FLAG_SELECT_3 = (1<<13),   /*!< Bit 13: Selection of the face in selection 3*/
  LIB3DS_FACE_FLAG_SELECT_2 = (1<<14),   /*!< Bit 14: Selection of the face in selection 2*/
  LIB3DS_FACE_FLAG_SELECT_1 = (1<<15),   /*!< Bit 15: Selection of the face in selection 1*/
} Lib3dsFaceFlag;

/**
 * Triangular mesh box mapping settings
 * \ingroup mesh
 */
struct Lib3dsBoxMap {
    char front[64];
    char back[64];
    char left[64];
    char right[64];
    char top[64];
    char bottom[64];
};

/**
 * Texture projection type
 * \ingroup tracks
 */
typedef enum {
  LIB3DS_MAP_NONE        =0xFFFF,
  LIB3DS_MAP_PLANAR      =0,
  LIB3DS_MAP_CYLINDRICAL =1,
  LIB3DS_MAP_SPHERICAL   =2
} Lib3dsMapType;

/**
 * Triangular mesh texture mapping data
 * \ingroup mesh
 */
struct Lib3dsMapData {
    Lib3dsWord maptype;
    Lib3dsVector pos;
    Lib3dsMatrix matrix;
    Lib3dsFloat scale;
    Lib3dsFloat tile[2];
    Lib3dsFloat planar_size[2];
    Lib3dsFloat cylinder_height;
};

/**
 * Triangular mesh object
 * \ingroup mesh
 */
struct Lib3dsMesh {
    Lib3dsUserData user;    	/*< Arbitrary user data */
    Lib3dsMesh *next;
    char name[64];		        /*< Mesh name. Don't use more than 8 characters  */
    Lib3dsDword object_flags; /*< @see Lib3dsObjectFlags */ 
    Lib3dsByte color;
    Lib3dsMatrix matrix;    	/*< Transformation matrix for mesh data */
    Lib3dsDword points;		    /*< Number of points in point list */
    Lib3dsPoint *pointL;	    /*< Point list */
    Lib3dsDword flags;		    /*< Number of flags in per-point flags list */
    Lib3dsWord *flagL;		    /*< Per-point flags list */
    Lib3dsDword texels;		    /*< Number of U-V texture coordinates */
    Lib3dsTexel *texelL;	    /*< U-V texture coordinates */
    Lib3dsDword faces;	    	/*< Number of faces in face list */
    Lib3dsFace *faceL;		    /*< Face list */
    Lib3dsBoxMap box_map;
    Lib3dsMapData map_data;
}; 

extern LIB3DSAPI Lib3dsMesh* lib3ds_mesh_new(const char *name);
extern LIB3DSAPI void lib3ds_mesh_free(Lib3dsMesh *mesh);
extern LIB3DSAPI Lib3dsBool lib3ds_mesh_new_point_list(Lib3dsMesh *mesh, Lib3dsDword points);
extern LIB3DSAPI void lib3ds_mesh_free_point_list(Lib3dsMesh *mesh);
extern LIB3DSAPI Lib3dsBool lib3ds_mesh_new_flag_list(Lib3dsMesh *mesh, Lib3dsDword flags);
extern LIB3DSAPI void lib3ds_mesh_free_flag_list(Lib3dsMesh *mesh);
extern LIB3DSAPI Lib3dsBool lib3ds_mesh_new_texel_list(Lib3dsMesh *mesh, Lib3dsDword texels);
extern LIB3DSAPI void lib3ds_mesh_free_texel_list(Lib3dsMesh *mesh);
extern LIB3DSAPI Lib3dsBool lib3ds_mesh_new_face_list(Lib3dsMesh *mesh, Lib3dsDword flags);
extern LIB3DSAPI void lib3ds_mesh_free_face_list(Lib3dsMesh *mesh);
extern LIB3DSAPI void lib3ds_mesh_bounding_box(Lib3dsMesh *mesh, Lib3dsVector bmin, Lib3dsVector bmax);
extern LIB3DSAPI void lib3ds_mesh_calculate_normals(Lib3dsMesh *mesh, Lib3dsVector *normalL);
extern LIB3DSAPI void lib3ds_mesh_dump(Lib3dsMesh *mesh);
extern LIB3DSAPI Lib3dsBool lib3ds_mesh_read(Lib3dsMesh *mesh, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_mesh_write(Lib3dsMesh *mesh, Lib3dsIo *io);

#ifdef __cplusplus
}
#endif
#endif

