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
 * $Id: node.c,v 1.20 2007/06/20 17:04:08 jeh Exp $
 */
#include <lib3ds/node.h>
#include <lib3ds/file.h>
#include <lib3ds/io.h>
#include <lib3ds/chunk.h>
#include <lib3ds/matrix.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/*!
 * \defgroup node Animation Nodes
 */


/*!
 * Create and return a new ambient node.
 *
 * The node is returned with an identity matrix.  All other fields
 * are zero.
 *
 * \return Lib3dsNode
 *
 * \ingroup node
 */
Lib3dsNode*
lib3ds_node_new_ambient()
{
  Lib3dsNode *node=(Lib3dsNode*)calloc(sizeof(Lib3dsNode), 1);
  node->type=LIB3DS_AMBIENT_NODE;
  lib3ds_matrix_identity(node->matrix);
  return(node);
}


/*!
 * Create and return a new object node.
 *
 * The node is returned with an identity matrix.  All other fields
 * are zero.
 *
 * \return Lib3dsNode
 *
 * \ingroup node
 */
Lib3dsNode*
lib3ds_node_new_object()
{
  Lib3dsNode *node=(Lib3dsNode*)calloc(sizeof(Lib3dsNode), 1);
  node->type=LIB3DS_OBJECT_NODE;
  lib3ds_matrix_identity(node->matrix);
  return(node);
}


/*!
 * Create and return a new camera node.
 *
 * The node is returned with an identity matrix.  All other fields
 * are zero.
 *
 * \return Lib3dsNode
 *
 * \ingroup node
 */
Lib3dsNode*
lib3ds_node_new_camera()
{
  Lib3dsNode *node=(Lib3dsNode*)calloc(sizeof(Lib3dsNode), 1);
  node->type=LIB3DS_CAMERA_NODE;
  lib3ds_matrix_identity(node->matrix);
  return(node);
}


/*!
 * Create and return a new target node.
 *
 * The node is returned with an identity matrix.  All other fields
 * are zero.
 *
 * \return Lib3dsNode
 *
 * \ingroup node
 */
Lib3dsNode*
lib3ds_node_new_target()
{
  Lib3dsNode *node=(Lib3dsNode*)calloc(sizeof(Lib3dsNode), 1);
  node->type=LIB3DS_TARGET_NODE;
  lib3ds_matrix_identity(node->matrix);
  return(node);
}


/*!
 * Create and return a new light node.
 *
 * The node is returned with an identity matrix.  All other fields
 * are zero.
 *
 * \return Lib3dsNode
 *
 * \ingroup node
 */
Lib3dsNode*
lib3ds_node_new_light()
{
  Lib3dsNode *node=(Lib3dsNode*)calloc(sizeof(Lib3dsNode), 1);
  node->type=LIB3DS_LIGHT_NODE;
  lib3ds_matrix_identity(node->matrix);
  return(node);
}


/*!
 * Create and return a new spot node.
 *
 * The node is returned with an identity matrix.  All other fields
 * are zero.
 *
 * \return Lib3dsNode
 *
 * \ingroup node
 */
Lib3dsNode*
lib3ds_node_new_spot()
{
  Lib3dsNode *node=(Lib3dsNode*)calloc(sizeof(Lib3dsNode), 1);
  node->type=LIB3DS_SPOT_NODE;
  lib3ds_matrix_identity(node->matrix);
  return(node);
}


static void
free_node_and_childs(Lib3dsNode *node)
{
  ASSERT(node);
  switch (node->type) {
    case LIB3DS_UNKNOWN_NODE:
      break;
    case LIB3DS_AMBIENT_NODE:
      {
        Lib3dsAmbientData *n=&node->data.ambient;
        lib3ds_lin3_track_free_keys(&n->col_track);
      }
      break;
    case LIB3DS_OBJECT_NODE:
      {
        Lib3dsObjectData *n=&node->data.object;

        lib3ds_lin3_track_free_keys(&n->pos_track);
        lib3ds_quat_track_free_keys(&n->rot_track);
        lib3ds_lin3_track_free_keys(&n->scl_track);
        lib3ds_bool_track_free_keys(&n->hide_track);
        lib3ds_morph_track_free_keys(&n->morph_track);
      }
      break;
    case LIB3DS_CAMERA_NODE:
      {
        Lib3dsCameraData *n=&node->data.camera;
        lib3ds_lin3_track_free_keys(&n->pos_track);
        lib3ds_lin1_track_free_keys(&n->fov_track);
        lib3ds_lin1_track_free_keys(&n->roll_track);
      }
      break;
    case LIB3DS_TARGET_NODE:
      {
        Lib3dsTargetData *n=&node->data.target;
        lib3ds_lin3_track_free_keys(&n->pos_track);
      }
      break;
    case LIB3DS_LIGHT_NODE:
      {
        Lib3dsLightData *n=&node->data.light;
        lib3ds_lin3_track_free_keys(&n->pos_track);
        lib3ds_lin3_track_free_keys(&n->col_track);
        lib3ds_lin1_track_free_keys(&n->hotspot_track);
        lib3ds_lin1_track_free_keys(&n->falloff_track);
        lib3ds_lin1_track_free_keys(&n->roll_track);
      }
      break;
    case LIB3DS_SPOT_NODE:
      {
        Lib3dsSpotData *n=&node->data.spot;
        lib3ds_lin3_track_free_keys(&n->pos_track);
      }
      break;
  }
  {
    Lib3dsNode *p,*q;
    for (p=node->childs; p; p=q) {
      q=p->next;
      free_node_and_childs(p);
    }
  }
  node->type=LIB3DS_UNKNOWN_NODE;
  free(node);
}


/*!
 * Free a node and all of its resources.
 *
 * \param node Lib3dsNode object to be freed.
 *
 * \ingroup node
 */
void
lib3ds_node_free(Lib3dsNode *node)
{
  ASSERT(node);
  free_node_and_childs(node);
}


/*!
 * Evaluate an animation node.
 *
 * Recursively sets node and its children to their appropriate values
 * for this point in the animation.
 *
 * \param node Node to be evaluated.
 * \param t time value, between 0. and file->frames
 *
 * \ingroup node
 */
void
lib3ds_node_eval(Lib3dsNode *node, Lib3dsFloat t)
{
  ASSERT(node);
  switch (node->type) {
    case LIB3DS_UNKNOWN_NODE:
      {
        ASSERT(LIB3DS_FALSE);
      }
      break;
    case LIB3DS_AMBIENT_NODE:
      {
        Lib3dsAmbientData *n=&node->data.ambient;
        if (node->parent) {
          lib3ds_matrix_copy(node->matrix, node->parent->matrix);
        }
        else {
          lib3ds_matrix_identity(node->matrix);
        }
        lib3ds_lin3_track_eval(&n->col_track, n->col, t);
      }
      break;
    case LIB3DS_OBJECT_NODE:
      {
        Lib3dsMatrix M;
        Lib3dsObjectData *n=&node->data.object;

        lib3ds_lin3_track_eval(&n->pos_track, n->pos, t);
        lib3ds_quat_track_eval(&n->rot_track, n->rot, t);
        if (n->scl_track.keyL) {
          lib3ds_lin3_track_eval(&n->scl_track, n->scl, t);
        }
        else {
          n->scl[0] = n->scl[1] = n->scl[2] = 1.0f;
        }
        lib3ds_bool_track_eval(&n->hide_track, &n->hide, t);
        lib3ds_morph_track_eval(&n->morph_track, n->morph, t);

        lib3ds_matrix_identity(M);
        lib3ds_matrix_translate(M, n->pos);
        lib3ds_matrix_rotate(M, n->rot);
        lib3ds_matrix_scale(M, n->scl);
        
        if (node->parent) {
          lib3ds_matrix_copy(node->matrix, node->parent->matrix);
          lib3ds_matrix_mult(node->matrix, M);
        }
        else {
          lib3ds_matrix_copy(node->matrix, M);
        }
      }
      break;
    case LIB3DS_CAMERA_NODE:
      {
        Lib3dsCameraData *n=&node->data.camera;
        lib3ds_lin3_track_eval(&n->pos_track, n->pos, t);
        lib3ds_lin1_track_eval(&n->fov_track, &n->fov, t);
        lib3ds_lin1_track_eval(&n->roll_track, &n->roll, t);
        if (node->parent) {
          lib3ds_matrix_copy(node->matrix, node->parent->matrix);
        }
        else {
          lib3ds_matrix_identity(node->matrix);
        }
        lib3ds_matrix_translate(node->matrix, n->pos);
      }
      break;
    case LIB3DS_TARGET_NODE:
      {
        Lib3dsTargetData *n=&node->data.target;
        lib3ds_lin3_track_eval(&n->pos_track, n->pos, t);
        if (node->parent) {
          lib3ds_matrix_copy(node->matrix, node->parent->matrix);
        }
        else {
          lib3ds_matrix_identity(node->matrix);
        }
        lib3ds_matrix_translate(node->matrix, n->pos);
      }
      break;
    case LIB3DS_LIGHT_NODE:
      {
        Lib3dsLightData *n=&node->data.light;
        lib3ds_lin3_track_eval(&n->pos_track, n->pos, t);
        lib3ds_lin3_track_eval(&n->col_track, n->col, t);
        lib3ds_lin1_track_eval(&n->hotspot_track, &n->hotspot, t);
        lib3ds_lin1_track_eval(&n->falloff_track, &n->falloff, t);
        lib3ds_lin1_track_eval(&n->roll_track, &n->roll, t);
        if (node->parent) {
          lib3ds_matrix_copy(node->matrix, node->parent->matrix);
        }
        else {
          lib3ds_matrix_identity(node->matrix);
        }
        lib3ds_matrix_translate(node->matrix, n->pos);
      }
      break;
    case LIB3DS_SPOT_NODE:
      {
        Lib3dsSpotData *n=&node->data.spot;
        lib3ds_lin3_track_eval(&n->pos_track, n->pos, t);
        if (node->parent) {
          lib3ds_matrix_copy(node->matrix, node->parent->matrix);
        }
        else {
          lib3ds_matrix_identity(node->matrix);
        }
        lib3ds_matrix_translate(node->matrix, n->pos);
      }
      break;
  }
  {
    Lib3dsNode *p;

    for (p=node->childs; p!=0; p=p->next) {
      lib3ds_node_eval(p, t);
    }
  }
}


/*!
 * Return a node object by name and type.
 *
 * This function performs a recursive search for the specified node.
 * Both name and type must match.
 *
 * \param node The parent node for the search
 * \param name The target node name.
 * \param type The target node type
 *
 * \return A pointer to the first matching node, or NULL if not found.
 *
 * \ingroup node
 */
Lib3dsNode*
lib3ds_node_by_name(Lib3dsNode *node, const char* name, Lib3dsNodeTypes type)
{
  Lib3dsNode *p,*q;

  for (p=node->childs; p!=0; p=p->next) {
    if ((p->type==type) && (strcmp(p->name, name)==0)) {
      return(p);
    }
    q=lib3ds_node_by_name(p, name, type);
    if (q) {
      return(q);
    }
  }
  return(0);
}


/*!
 * Return a node object by id.
 *
 * This function performs a recursive search for the specified node.
 *
 * \param node The parent node for the search
 * \param node_id The target node id.
 *
 * \return A pointer to the first matching node, or NULL if not found.
 *
 * \ingroup node
 */
Lib3dsNode*
lib3ds_node_by_id(Lib3dsNode *node, Lib3dsWord node_id)
{
  Lib3dsNode *p,*q;

  for (p=node->childs; p!=0; p=p->next) {
    if (p->node_id==node_id) {
      return(p);
    }
    q=lib3ds_node_by_id(p, node_id);
    if (q) {
      return(q);
    }
  }
  return(0);
}


static const char* node_names_table[]= {
  "***Unknown***",
  "Ambient",
  "Object",
  "Camera",
  "Target",
  "Light",
  "Spot"
};


/*!
 * Dump node and all descendants recursively.
 *
 * \param node The top-level node to be dumped.
 * \param level current recursion depth
 *
 * \ingroup node
 */
void
lib3ds_node_dump(Lib3dsNode *node, Lib3dsIntd level)
{
  Lib3dsNode *p;
  char l[128];

  ASSERT(node);
  memset(l, ' ', 2*level);
  l[2*level]=0;

  if (node->type==LIB3DS_OBJECT_NODE) {
    printf("%s%s [%s] (%s)\n",
      l,
      node->name,
      node->data.object.instance,
      node_names_table[node->type]
    );
  }
  else {
    printf("%s%s (%s)\n",
      l,
      node->name,
      node_names_table[node->type]
    );
  }
  
  for (p=node->childs; p!=0; p=p->next) {
    lib3ds_node_dump(p, level+1);
  }
}


/*!
 * \ingroup node
 */
Lib3dsBool
lib3ds_node_read(Lib3dsNode *node, Lib3dsFile *file, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;

  ASSERT(node);
  if (!lib3ds_chunk_read_start(&c, 0, io)) {
    return(LIB3DS_FALSE);
  }
  switch (c.chunk) {
    case LIB3DS_AMBIENT_NODE_TAG:
    case LIB3DS_OBJECT_NODE_TAG:
    case LIB3DS_CAMERA_NODE_TAG:
    case LIB3DS_TARGET_NODE_TAG:
    case LIB3DS_LIGHT_NODE_TAG:
    case LIB3DS_SPOTLIGHT_NODE_TAG:
    case LIB3DS_L_TARGET_NODE_TAG:
      break;
    default:
      return(LIB3DS_FALSE);
  }

  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_NODE_ID:
        {
          node->node_id=lib3ds_io_read_word(io);
          lib3ds_chunk_dump_info("  ID = %d", (short)node->node_id);
        }
        break;
      case LIB3DS_NODE_HDR:
        {
          if (!lib3ds_io_read_string(io, node->name, 64)) {
            return(LIB3DS_FALSE);
          }
          node->flags1=lib3ds_io_read_word(io);
          node->flags2=lib3ds_io_read_word(io);
          node->parent_id=lib3ds_io_read_word(io);
          lib3ds_chunk_dump_info("  NAME =%s", node->name);
          lib3ds_chunk_dump_info("  PARENT=%d", (short)node->parent_id);
        }
        break;
      case LIB3DS_PIVOT:
        {
          if (node->type==LIB3DS_OBJECT_NODE) {
            int i;
            for (i=0; i<3; ++i) {
              node->data.object.pivot[i]=lib3ds_io_read_float(io);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_INSTANCE_NAME:
        {
          if (node->type==LIB3DS_OBJECT_NODE) {
            if (!lib3ds_io_read_string(io, node->data.object.instance, 64)) {
              return(LIB3DS_FALSE);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_BOUNDBOX:
        {
          if (node->type==LIB3DS_OBJECT_NODE) {
            int i;
            for (i=0; i<3; ++i) {
              node->data.object.bbox_min[i]=lib3ds_io_read_float(io);
            }
            for (i=0; i<3; ++i) {
              node->data.object.bbox_max[i]=lib3ds_io_read_float(io);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_COL_TRACK_TAG:
        {
          Lib3dsBool result=LIB3DS_TRUE;
          
          switch (node->type) {
            case LIB3DS_AMBIENT_NODE:
              result=lib3ds_lin3_track_read(&node->data.ambient.col_track, io);
              break;
            case LIB3DS_LIGHT_NODE:
              result=lib3ds_lin3_track_read(&node->data.light.col_track, io);
              break;
            default:
              lib3ds_chunk_unknown(chunk);
          }
          if (!result) {
            return(LIB3DS_FALSE);
          }
        }
        break;
      case LIB3DS_POS_TRACK_TAG:
        {
          Lib3dsBool result=LIB3DS_TRUE;

          switch (node->type) {
            case LIB3DS_OBJECT_NODE:
              result=lib3ds_lin3_track_read(&node->data.object.pos_track, io);
              break;
            case LIB3DS_CAMERA_NODE:
              result=lib3ds_lin3_track_read(&node->data.camera.pos_track, io);
              break;
            case LIB3DS_TARGET_NODE:
              result=lib3ds_lin3_track_read(&node->data.target.pos_track, io);
              break;
            case LIB3DS_LIGHT_NODE:
              result=lib3ds_lin3_track_read(&node->data.light.pos_track, io);
              break;
            case LIB3DS_SPOT_NODE:
              result=lib3ds_lin3_track_read(&node->data.spot.pos_track, io);
              break;
            default:
              lib3ds_chunk_unknown(chunk);
          }
          if (!result) {
            return(LIB3DS_FALSE);
          }
        }
        break;
      case LIB3DS_ROT_TRACK_TAG:
        {
          if (node->type==LIB3DS_OBJECT_NODE) {
            if (!lib3ds_quat_track_read(&node->data.object.rot_track, io)) {
              return(LIB3DS_FALSE);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_SCL_TRACK_TAG:
        {
          if (node->type==LIB3DS_OBJECT_NODE) {
            if (!lib3ds_lin3_track_read(&node->data.object.scl_track, io)) {
              return(LIB3DS_FALSE);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_FOV_TRACK_TAG:
        {
          if (node->type==LIB3DS_CAMERA_NODE) {
            if (!lib3ds_lin1_track_read(&node->data.camera.fov_track, io)) {
              return(LIB3DS_FALSE);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_HOT_TRACK_TAG:
        {
          if (node->type==LIB3DS_LIGHT_NODE) {
            if (!lib3ds_lin1_track_read(&node->data.light.hotspot_track, io)) {
              return(LIB3DS_FALSE);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_FALL_TRACK_TAG:
        {
          if (node->type==LIB3DS_LIGHT_NODE) {
            if (!lib3ds_lin1_track_read(&node->data.light.falloff_track, io)) {
              return(LIB3DS_FALSE);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_ROLL_TRACK_TAG:
        {
          Lib3dsBool result=LIB3DS_TRUE;

          switch (node->type) {
            case LIB3DS_CAMERA_NODE:
              result=lib3ds_lin1_track_read(&node->data.camera.roll_track, io);
              break;
            case LIB3DS_LIGHT_NODE:
              result=lib3ds_lin1_track_read(&node->data.light.roll_track, io);
              break;
            default:
              lib3ds_chunk_unknown(chunk);
          }
          if (!result) {
            return(LIB3DS_FALSE);
          }
        }
        break;
      case LIB3DS_HIDE_TRACK_TAG:
        {
          if (node->type==LIB3DS_OBJECT_NODE) {
            if (!lib3ds_bool_track_read(&node->data.object.hide_track, io)) {
              return(LIB3DS_FALSE);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_MORPH_SMOOTH:
        {
          if (node->type==LIB3DS_OBJECT_NODE) {
            node->data.object.morph_smooth=lib3ds_io_read_float(io);
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      case LIB3DS_MORPH_TRACK_TAG:
        {
          if (node->type==LIB3DS_OBJECT_NODE) {
            if (!lib3ds_morph_track_read(&node->data.object.morph_track, io)) {
              return(LIB3DS_FALSE);
            }
          }
          else {
            lib3ds_chunk_unknown(chunk);
          }
        }
        break;
      default:
        lib3ds_chunk_unknown(chunk);
    }
  }

  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup node
 */
Lib3dsBool
lib3ds_node_write(Lib3dsNode *node, Lib3dsFile *file, Lib3dsIo *io)
{
  Lib3dsChunk c;

  switch (node->type) {
    case LIB3DS_AMBIENT_NODE:
      c.chunk=LIB3DS_AMBIENT_NODE_TAG;
      break;
    case LIB3DS_OBJECT_NODE:
      c.chunk=LIB3DS_OBJECT_NODE_TAG;
      break;
    case LIB3DS_CAMERA_NODE:
      c.chunk=LIB3DS_CAMERA_NODE_TAG;
      break;
    case LIB3DS_TARGET_NODE:
      c.chunk=LIB3DS_TARGET_NODE_TAG;
      break;
    case LIB3DS_LIGHT_NODE:
      if (lib3ds_file_node_by_name(file, node->name, LIB3DS_SPOT_NODE)) {
        c.chunk=LIB3DS_SPOTLIGHT_NODE_TAG;
      }
      else {
        c.chunk=LIB3DS_LIGHT_NODE_TAG;
      }
      break;
    case LIB3DS_SPOT_NODE:
      c.chunk=LIB3DS_L_TARGET_NODE_TAG;
      break;
    default:
      return(LIB3DS_FALSE);
  }
  if (!lib3ds_chunk_write_start(&c,io)) {
    return(LIB3DS_FALSE);
  }

  { /*---- LIB3DS_NODE_ID ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_NODE_ID;
    c.size=8;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_intw(io, node->node_id);
  }

  { /*---- LIB3DS_NODE_HDR ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_NODE_HDR;
    c.size=6+ 1+(Lib3dsDword)strlen(node->name) +2+2+2;
    lib3ds_chunk_write(&c,io);
    lib3ds_io_write_string(io, node->name);
    lib3ds_io_write_word(io, node->flags1);
    lib3ds_io_write_word(io, node->flags2);
    lib3ds_io_write_word(io, node->parent_id);
  }

  switch (c.chunk) {
    case LIB3DS_AMBIENT_NODE_TAG:
      { /*---- LIB3DS_COL_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_COL_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.ambient.col_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      break;
    case LIB3DS_OBJECT_NODE_TAG:
      { /*---- LIB3DS_PIVOT ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_PIVOT;
        c.size=18;
        lib3ds_chunk_write(&c,io);
        lib3ds_io_write_vector(io, node->data.object.pivot);
      }
      { /*---- LIB3DS_INSTANCE_NAME ----*/
        Lib3dsChunk c;
        const char *name;
        if (strlen(node->data.object.instance)) {
          name=node->data.object.instance;

          c.chunk=LIB3DS_INSTANCE_NAME;
          c.size=6+1+(Lib3dsDword)strlen(name);
          lib3ds_chunk_write(&c,io);
          lib3ds_io_write_string(io, name);
        }
      }
      {
        int i;
        for (i=0; i<3; ++i) {
          if ((fabs(node->data.object.bbox_min[i])>LIB3DS_EPSILON) ||
            (fabs(node->data.object.bbox_max[i])>LIB3DS_EPSILON)) {
            break;
          }
        }
        
        if (i<3) { /*---- LIB3DS_BOUNDBOX ----*/
          Lib3dsChunk c;
          c.chunk=LIB3DS_BOUNDBOX;
          c.size=30;
          lib3ds_chunk_write(&c,io);
          lib3ds_io_write_vector(io, node->data.object.bbox_min);
          lib3ds_io_write_vector(io, node->data.object.bbox_max);
        }
      }
      { /*---- LIB3DS_POS_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_POS_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.object.pos_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      { /*---- LIB3DS_ROT_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_ROT_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_quat_track_write(&node->data.object.rot_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      { /*---- LIB3DS_SCL_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_SCL_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.object.scl_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      if (node->data.object.hide_track.keyL) { /*---- LIB3DS_HIDE_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_HIDE_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_bool_track_write(&node->data.object.hide_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      if (fabs(node->data.object.morph_smooth)>LIB3DS_EPSILON){ /*---- LIB3DS_MORPH_SMOOTH ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_MORPH_SMOOTH;
        c.size=10;
        lib3ds_chunk_write(&c,io);
        lib3ds_io_write_float(io, node->data.object.morph_smooth);
      }
      break;
    case LIB3DS_CAMERA_NODE_TAG:
      { /*---- LIB3DS_POS_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_POS_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.camera.pos_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      { /*---- LIB3DS_FOV_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_FOV_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin1_track_write(&node->data.camera.fov_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      { /*---- LIB3DS_ROLL_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_ROLL_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin1_track_write(&node->data.camera.roll_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      break;
    case LIB3DS_TARGET_NODE_TAG:
      { /*---- LIB3DS_POS_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_POS_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.target.pos_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      break;
    case LIB3DS_LIGHT_NODE_TAG:
      { /*---- LIB3DS_POS_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_POS_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.light.pos_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      { /*---- LIB3DS_COL_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_COL_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.light.col_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      break;
    case LIB3DS_SPOTLIGHT_NODE_TAG:
      { /*---- LIB3DS_POS_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_POS_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.light.pos_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      { /*---- LIB3DS_COL_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_COL_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.light.col_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      { /*---- LIB3DS_HOT_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_HOT_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin1_track_write(&node->data.light.hotspot_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      { /*---- LIB3DS_FALL_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_FALL_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin1_track_write(&node->data.light.falloff_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      { /*---- LIB3DS_ROLL_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_ROLL_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin1_track_write(&node->data.light.roll_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      break;
    case LIB3DS_L_TARGET_NODE_TAG:
      { /*---- LIB3DS_POS_TRACK_TAG ----*/
        Lib3dsChunk c;
        c.chunk=LIB3DS_POS_TRACK_TAG;
        if (!lib3ds_chunk_write_start(&c,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_lin3_track_write(&node->data.spot.pos_track,io)) {
          return(LIB3DS_FALSE);
        }
        if (!lib3ds_chunk_write_end(&c,io)) {
          return(LIB3DS_FALSE);
        }
      }
      break;
    default:
      return(LIB3DS_FALSE);
  }

  if (!lib3ds_chunk_write_end(&c,io)) {
    return(LIB3DS_FALSE);
  }
  return(LIB3DS_TRUE);
}

