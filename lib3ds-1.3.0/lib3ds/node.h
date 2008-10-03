/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_NODE_H
#define INCLUDED_LIB3DS_NODE_H
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
 * $Id: node.h,v 1.12 2007/06/20 17:04:09 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TRACKS_H
#include <lib3ds/tracks.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Scene graph ambient color node data
 * \ingroup node
 */
typedef struct Lib3dsAmbientData {
    Lib3dsRgb col;
    Lib3dsLin3Track col_track;
} Lib3dsAmbientData;

/**
 * Scene graph object instance node data
 * \ingroup node
 */
typedef struct Lib3dsObjectData {
    Lib3dsVector pivot;
    char instance[64];
    Lib3dsVector bbox_min;
    Lib3dsVector bbox_max;
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
    Lib3dsQuat rot;
    Lib3dsQuatTrack rot_track;
    Lib3dsVector scl;
    Lib3dsLin3Track scl_track;
    Lib3dsFloat morph_smooth;
    char morph[64];
    Lib3dsMorphTrack morph_track;
    Lib3dsBool hide;
    Lib3dsBoolTrack hide_track;
} Lib3dsObjectData;

/**
 * Scene graph camera node data
 * \ingroup node
 */
typedef struct Lib3dsCameraData {
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
    Lib3dsFloat fov;
    Lib3dsLin1Track fov_track;
    Lib3dsFloat roll;
    Lib3dsLin1Track roll_track;
} Lib3dsCameraData;

/**
 * Scene graph camera target node data
 * \ingroup node
 */
typedef struct Lib3dsTargetData {
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
} Lib3dsTargetData;

/**
 * Scene graph light node data
 * \ingroup node
 */
typedef struct Lib3dsLightData {
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
    Lib3dsRgb col;
    Lib3dsLin3Track col_track;
    Lib3dsFloat hotspot;
    Lib3dsLin1Track hotspot_track;
    Lib3dsFloat falloff;
    Lib3dsLin1Track falloff_track;
    Lib3dsFloat roll;
    Lib3dsLin1Track roll_track;
} Lib3dsLightData;

/**
 * Scene graph spotlight target node data
 * \ingroup node
 */
typedef struct Lib3dsSpotData {
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
} Lib3dsSpotData;

/**
 * Scene graph node data union
 * \ingroup node
 */
typedef union Lib3dsNodeData {
    Lib3dsAmbientData ambient;
    Lib3dsObjectData object;
    Lib3dsCameraData camera;
    Lib3dsTargetData target;
    Lib3dsLightData light;
    Lib3dsSpotData spot;
} Lib3dsNodeData;

/*!
 * \ingroup node
 */
#define LIB3DS_NO_PARENT 65535

/**
 * Scene graph node
 * \ingroup node
 */
struct Lib3dsNode {
    Lib3dsUserData user;
    Lib3dsNode *next;
    Lib3dsNode *childs;
    Lib3dsNode *parent;
    Lib3dsNodeTypes type;
    Lib3dsWord node_id;
    char name[64];
    Lib3dsWord flags1;
    Lib3dsWord flags2;
    Lib3dsWord parent_id;
    Lib3dsMatrix matrix;
    Lib3dsNodeData data;
};

/**
 * Node flags #1
 * \ingroup node
 */
typedef enum {
  LIB3DS_HIDDEN = 0x800
} Lib3dsNodeFlags1;

/**
 * Node flags #2
 * \ingroup node
 */
typedef enum {
  LIB3DS_SHOW_PATH = 0x1,
  LIB3DS_SMOOTHING = 0x2,
  LIB3DS_MOTION_BLUR = 0x10,
  LIB3DS_MORPH_MATERIALS = 0x40
} Lib3dsNodeFlags2;

extern LIB3DSAPI Lib3dsNode* lib3ds_node_new_ambient();
extern LIB3DSAPI Lib3dsNode* lib3ds_node_new_object();
extern LIB3DSAPI Lib3dsNode* lib3ds_node_new_camera();
extern LIB3DSAPI Lib3dsNode* lib3ds_node_new_target();
extern LIB3DSAPI Lib3dsNode* lib3ds_node_new_light();
extern LIB3DSAPI Lib3dsNode* lib3ds_node_new_spot();
extern LIB3DSAPI void lib3ds_node_free(Lib3dsNode *node);
extern LIB3DSAPI void lib3ds_node_eval(Lib3dsNode *node, Lib3dsFloat t);
extern LIB3DSAPI Lib3dsNode* lib3ds_node_by_name(Lib3dsNode *node, const char* name,
  Lib3dsNodeTypes type);
extern LIB3DSAPI Lib3dsNode* lib3ds_node_by_id(Lib3dsNode *node, Lib3dsWord node_id);
extern LIB3DSAPI void lib3ds_node_dump(Lib3dsNode *node, Lib3dsIntd level);
extern LIB3DSAPI Lib3dsBool lib3ds_node_read(Lib3dsNode *node, Lib3dsFile *file, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_node_write(Lib3dsNode *node, Lib3dsFile *file, Lib3dsIo *io);

#ifdef __cplusplus
}
#endif
#endif

