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
 * $Id: mesh.c,v 1.29 2007/06/20 17:04:08 jeh Exp $
 */
#include <lib3ds/mesh.h>
#include <lib3ds/io.h>
#include <lib3ds/chunk.h>
#include <lib3ds/vector.h>
#include <lib3ds/matrix.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>


/*!
 * \defgroup mesh Meshes
 */


static Lib3dsBool
face_array_read(Lib3dsMesh *mesh, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;
  int i;
  int faces;

  if (!lib3ds_chunk_read_start(&c, LIB3DS_FACE_ARRAY, io)) {
    return(LIB3DS_FALSE);
  }
  lib3ds_mesh_free_face_list(mesh);
  
  faces=lib3ds_io_read_word(io);
  if (faces) {
    if (!lib3ds_mesh_new_face_list(mesh, faces)) {
      LIB3DS_ERROR_LOG;
      return(LIB3DS_FALSE);
    }
    for (i=0; i<faces; ++i) {
      strcpy(mesh->faceL[i].material, "");
      mesh->faceL[i].points[0]=lib3ds_io_read_word(io);
      mesh->faceL[i].points[1]=lib3ds_io_read_word(io);
      mesh->faceL[i].points[2]=lib3ds_io_read_word(io);
      mesh->faceL[i].flags=lib3ds_io_read_word(io);
    }
    lib3ds_chunk_read_tell(&c, io);

    while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
      switch (chunk) {
        case LIB3DS_SMOOTH_GROUP:
          {
            unsigned i;

            for (i=0; i<mesh->faces; ++i) {
              mesh->faceL[i].smoothing=lib3ds_io_read_dword(io);
            }
          }
          break;
        case LIB3DS_MSH_MAT_GROUP:
          {
            char name[64];
            unsigned faces;
            unsigned i;
            unsigned index;

            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            faces=lib3ds_io_read_word(io);
            for (i=0; i<faces; ++i) {
              index=lib3ds_io_read_word(io);
              ASSERT(index<mesh->faces);
              strcpy(mesh->faceL[index].material, name);
            }
          }
          break;
        case LIB3DS_MSH_BOXMAP:
          {
            char name[64];

            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.front, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.back, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.left, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.right, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.top, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.bottom, name);
          }
          break;
        default:
          lib3ds_chunk_unknown(chunk);
      }
    }
    
  }
  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


/*!
 * Create and return a new empty mesh object.
 *
 * Mesh is initialized with the name and an identity matrix; all
 * other fields are zero.
 *
 * See Lib3dsFaceFlag for definitions of per-face flags.
 *
 * \param name Mesh name.  Must not be NULL.  Must be < 64 characters.
 *
 * \return mesh object or NULL on error.
 *
 * \ingroup mesh
 */
Lib3dsMesh*
lib3ds_mesh_new(const char *name)
{
  Lib3dsMesh *mesh;

  ASSERT(name);
  ASSERT(strlen(name)<64);
  
  mesh=(Lib3dsMesh*)calloc(sizeof(Lib3dsMesh), 1);
  if (!mesh) {
    return(0);
  }
  strcpy(mesh->name, name);
  lib3ds_matrix_identity(mesh->matrix);
  mesh->map_data.maptype=LIB3DS_MAP_NONE;
  return(mesh);
}


/*!
 * Free a mesh object and all of its resources.
 *
 * \param mesh Mesh object to be freed.
 *
 * \ingroup mesh
 */
void
lib3ds_mesh_free(Lib3dsMesh *mesh)
{
  lib3ds_mesh_free_point_list(mesh);
  lib3ds_mesh_free_flag_list(mesh);
  lib3ds_mesh_free_texel_list(mesh);
  lib3ds_mesh_free_face_list(mesh);
  memset(mesh, 0, sizeof(Lib3dsMesh));
  free(mesh);
}


/*!
 * Allocate point list in mesh object.
 *
 * This function frees the current point list, if any, and allocates
 * a new one large enough to hold the specified number of points.
 *
 * \param mesh Mesh object for which points are to be allocated.
 * \param points Number of points in the new point list.
 *
 * \return LIB3DS_TRUE on success, LIB3DS_FALSE on failure.
 *
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_new_point_list(Lib3dsMesh *mesh, Lib3dsDword points)
{
  ASSERT(mesh);
  if (mesh->pointL) {
    ASSERT(mesh->points);
    lib3ds_mesh_free_point_list(mesh);
  }
  ASSERT(!mesh->pointL && !mesh->points);
  mesh->points=0;
  mesh->pointL=calloc(sizeof(Lib3dsPoint)*points,1);
  if (!mesh->pointL) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  mesh->points=points;
  return(LIB3DS_TRUE);
}


/*!
 * Free point list in mesh object.
 *
 * The current point list is freed and set to NULL.  mesh->points is
 * set to zero.
 *
 * \param mesh Mesh object to be modified.
 *
 * \ingroup mesh
 */
void
lib3ds_mesh_free_point_list(Lib3dsMesh *mesh)
{
  ASSERT(mesh);
  if (mesh->pointL) {
    ASSERT(mesh->points);
    free(mesh->pointL);
    mesh->pointL=0;
    mesh->points=0;
  }
  else {
    ASSERT(!mesh->points);
  }
}


/*!
 * Allocate flag list in mesh object.
 *
 * This function frees the current flag list, if any, and allocates
 * a new one large enough to hold the specified number of flags.
 * All flags are initialized to 0
 *
 * \param mesh Mesh object for which points are to be allocated.
 * \param flags Number of flags in the new flag list.
 *
 * \return LIB3DS_TRUE on success, LIB3DS_FALSE on failure.
 *
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_new_flag_list(Lib3dsMesh *mesh, Lib3dsDword flags)
{
  ASSERT(mesh);
  if (mesh->flagL) {
    ASSERT(mesh->flags);
    lib3ds_mesh_free_flag_list(mesh);
  }
  ASSERT(!mesh->flagL && !mesh->flags);
  mesh->flags=0;
  mesh->flagL=calloc(sizeof(Lib3dsWord)*flags,1);
  if (!mesh->flagL) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  mesh->flags=flags;
  return(LIB3DS_TRUE);
}


/*!
 * Free flag list in mesh object.
 *
 * The current flag list is freed and set to NULL.  mesh->flags is
 * set to zero.
 *
 * \param mesh Mesh object to be modified.
 *
 * \ingroup mesh
 */
void
lib3ds_mesh_free_flag_list(Lib3dsMesh *mesh)
{
  ASSERT(mesh);
  if (mesh->flagL) {
    ASSERT(mesh->flags);
    free(mesh->flagL);
    mesh->flagL=0;
    mesh->flags=0;
  }
  else {
    ASSERT(!mesh->flags);
  }
}


/*!
 * Allocate texel list in mesh object.
 *
 * This function frees the current texel list, if any, and allocates
 * a new one large enough to hold the specified number of texels.
 *
 * \param mesh Mesh object for which points are to be allocated.
 * \param texels Number of texels in the new texel list.
 *
 * \return LIB3DS_TRUE on success, LIB3DS_FALSE on failure.
 *
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_new_texel_list(Lib3dsMesh *mesh, Lib3dsDword texels)
{
  ASSERT(mesh);
  if (mesh->texelL) {
    ASSERT(mesh->texels);
    lib3ds_mesh_free_texel_list(mesh);
  }
  ASSERT(!mesh->texelL && !mesh->texels);
  mesh->texels=0;
  mesh->texelL=calloc(sizeof(Lib3dsTexel)*texels,1);
  if (!mesh->texelL) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  mesh->texels=texels;
  return(LIB3DS_TRUE);
}


/*!
 * Free texel list in mesh object.
 *
 * The current texel list is freed and set to NULL.  mesh->texels is
 * set to zero.
 *
 * \param mesh Mesh object to be modified.
 *
 * \ingroup mesh
 */
void
lib3ds_mesh_free_texel_list(Lib3dsMesh *mesh)
{
  ASSERT(mesh);
  if (mesh->texelL) {
    ASSERT(mesh->texels);
    free(mesh->texelL);
    mesh->texelL=0;
    mesh->texels=0;
  }
  else {
    ASSERT(!mesh->texels);
  }
}


/*!
 * Allocate face list in mesh object.
 *
 * This function frees the current face list, if any, and allocates
 * a new one large enough to hold the specified number of faces.
 *
 * \param mesh Mesh object for which points are to be allocated.
 * \param faces Number of faces in the new face list.
 *
 * \return LIB3DS_TRUE on success, LIB3DS_FALSE on failure.
 *
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_new_face_list(Lib3dsMesh *mesh, Lib3dsDword faces)
{
  ASSERT(mesh);
  if (mesh->faceL) {
    ASSERT(mesh->faces);
    lib3ds_mesh_free_face_list(mesh);
  }
  ASSERT(!mesh->faceL && !mesh->faces);
  mesh->faces=0;
  mesh->faceL=calloc(sizeof(Lib3dsFace)*faces,1);
  if (!mesh->faceL) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  mesh->faces=faces;
  return(LIB3DS_TRUE);
}


/*!
 * Free face list in mesh object.
 *
 * The current face list is freed and set to NULL.  mesh->faces is
 * set to zero.
 *
 * \param mesh Mesh object to be modified.
 *
 * \ingroup mesh
 */
void
lib3ds_mesh_free_face_list(Lib3dsMesh *mesh)
{
  ASSERT(mesh);
  if (mesh->faceL) {
    ASSERT(mesh->faces);
    free(mesh->faceL);
    mesh->faceL=0;
    mesh->faces=0;
  }
  else {
    ASSERT(!mesh->faces);
  }
}


/*!
 * Find the bounding box of a mesh object.
 *
 * \param mesh The mesh object
 * \param bmin Returned bounding box
 * \param bmax Returned bounding box
 *
 * \ingroup mesh
 */
void
lib3ds_mesh_bounding_box(Lib3dsMesh *mesh, Lib3dsVector bmin, Lib3dsVector bmax)
{
  unsigned i;
  bmin[0] = bmin[1] = bmin[2] = FLT_MAX; 
  bmax[0] = bmax[1] = bmax[2] = FLT_MIN; 

  for (i=0; i<mesh->points; ++i) {
    lib3ds_vector_min(bmin, mesh->pointL[i].pos);
    lib3ds_vector_max(bmax, mesh->pointL[i].pos);
  }
}


typedef struct _Lib3dsFaces Lib3dsFaces; 

struct _Lib3dsFaces {
  Lib3dsFaces *next;
  Lib3dsFace *face;
};


/*!
 * Calculates the vertex normals corresponding to the smoothing group
 * settings for each face of a mesh.
 *
 * \param mesh      A pointer to the mesh to calculate the normals for.
 * \param normalL   A pointer to a buffer to store the calculated
 *                  normals. The buffer must have the size:
 *                  3*sizeof(Lib3dsVector)*mesh->faces. 
 *
 * To allocate the normal buffer do for example the following:
 * \code
 *  Lib3dsVector *normalL = malloc(3*sizeof(Lib3dsVector)*mesh->faces);
 * \endcode
 *
 * To access the normal of the i-th vertex of the j-th face do the 
 * following:
 * \code
 *   normalL[3*j+i]
 * \endcode
 *
 * \ingroup mesh
 */
void
lib3ds_mesh_calculate_normals(Lib3dsMesh *mesh, Lib3dsVector *normalL)
{
  Lib3dsFaces **fl; 
  Lib3dsFaces *fa; 
  unsigned i,j,k;

  if (!mesh->faces) {
    return;
  }

  fl=calloc(sizeof(Lib3dsFaces*),mesh->points);
  ASSERT(fl);
  fa=calloc(sizeof(Lib3dsFaces),3*mesh->faces);
  ASSERT(fa);
  k=0;
  for (i=0; i<mesh->faces; ++i) {
    Lib3dsFace *f=&mesh->faceL[i];
    for (j=0; j<3; ++j) {
      Lib3dsFaces* l=&fa[k++];
      ASSERT(f->points[j]<mesh->points);
      l->face=f;
      l->next=fl[f->points[j]];
      fl[f->points[j]]=l;
    }
  }
  
  for (i=0; i<mesh->faces; ++i) {
    Lib3dsFace *f=&mesh->faceL[i];
    for (j=0; j<3; ++j) {
      // FIXME: static array needs at least check!!
      Lib3dsVector n,N[128];
      Lib3dsFaces *p;
      int k,l;
      int found;

      ASSERT(f->points[j]<mesh->points);

      if (f->smoothing) {
        lib3ds_vector_zero(n);
        k=0;
        for (p=fl[f->points[j]]; p; p=p->next) {
          found=0;
          for (l=0; l<k; ++l) {
	    if( l >= 128 )
	      printf("array N overflow: i=%d, j=%d, k=%d\n", i,j,k);
            if (fabs(lib3ds_vector_dot(N[l], p->face->normal)-1.0)<1e-5) {
              found=1;
              break;
            }
          }
          if (!found) {
            if (f->smoothing & p->face->smoothing) {
              lib3ds_vector_add(n,n, p->face->normal);
              lib3ds_vector_copy(N[k], p->face->normal);
              ++k;
            }
          }
        }
      } 
      else {
        lib3ds_vector_copy(n, f->normal);
      }
      lib3ds_vector_normalize(n);

      lib3ds_vector_copy(normalL[3*i+j], n);
    }
  }

  free(fa);
  free(fl);
}


/*!
 * This function prints data associated with the specified mesh such as
 * vertex and point lists.
 *
 * \param mesh  Points to a mesh that you wish to view the data for.
 *
 * \return None
 *
 * \warning WIN32: Should only be used in a console window not in a GUI.
 *
 * \ingroup mesh
 */
void
lib3ds_mesh_dump(Lib3dsMesh *mesh)
{
  unsigned i;
  Lib3dsVector p;

  ASSERT(mesh);
  printf("  %s vertices=%ld faces=%ld\n",
    mesh->name,
    mesh->points,
    mesh->faces
  );
  printf("  matrix:\n");
  lib3ds_matrix_dump(mesh->matrix);
  printf("  point list:\n");
  for (i=0; i<mesh->points; ++i) {
    lib3ds_vector_copy(p, mesh->pointL[i].pos);
    printf ("    %8f %8f %8f\n", p[0], p[1], p[2]);
  }
  printf("  facelist:\n");
  for (i=0; i<mesh->faces; ++i) {
    printf ("    %4d %4d %4d  smoothing:%X  flags:%X  material:\"%s\"\n",
      mesh->faceL[i].points[0],
      mesh->faceL[i].points[1],
      mesh->faceL[i].points[2],
      (unsigned)mesh->faceL[i].smoothing,
      mesh->faceL[i].flags,
      mesh->faceL[i].material
    );
  }
}


/*!
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_read(Lib3dsMesh *mesh, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;

  if (!lib3ds_chunk_read_start(&c, LIB3DS_N_TRI_OBJECT, io)) {
    return(LIB3DS_FALSE);
  }

  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_MESH_MATRIX:
        {
          int i,j;
          
          lib3ds_matrix_identity(mesh->matrix);
          for (i=0; i<4; i++) {
            for (j=0; j<3; j++) {
              mesh->matrix[i][j]=lib3ds_io_read_float(io);
            }
          }
        }
        break;
      case LIB3DS_MESH_COLOR:
        {
          mesh->color=lib3ds_io_read_byte(io);
        }
        break;
      case LIB3DS_POINT_ARRAY:
        {
          unsigned i,j;
          unsigned points;
          
          lib3ds_mesh_free_point_list(mesh);
          points=lib3ds_io_read_word(io);
          if (points) {
            if (!lib3ds_mesh_new_point_list(mesh, points)) {
              LIB3DS_ERROR_LOG;
              return(LIB3DS_FALSE);
            }
            for (i=0; i<mesh->points; ++i) {
              for (j=0; j<3; ++j) {
                mesh->pointL[i].pos[j]=lib3ds_io_read_float(io);
              }
            }
            ASSERT((!mesh->flags) || (mesh->points==mesh->flags));
            ASSERT((!mesh->texels) || (mesh->points==mesh->texels));
          }
        }
        break;
      case LIB3DS_POINT_FLAG_ARRAY:
        {
          unsigned i;
          unsigned flags;
          
          lib3ds_mesh_free_flag_list(mesh);
          flags=lib3ds_io_read_word(io);
          if (flags) {
            if (!lib3ds_mesh_new_flag_list(mesh, flags)) {
              LIB3DS_ERROR_LOG;
              return(LIB3DS_FALSE);
            }
            for (i=0; i<mesh->flags; ++i) {
              mesh->flagL[i]=lib3ds_io_read_word(io);
            }
            ASSERT((!mesh->points) || (mesh->flags==mesh->points));
            ASSERT((!mesh->texels) || (mesh->flags==mesh->texels));
          }
        }
        break;
      case LIB3DS_FACE_ARRAY:
        {
          lib3ds_chunk_read_reset(&c, io);
          if (!face_array_read(mesh, io)) {
            return(LIB3DS_FALSE);
          }
        }
        break;
      case LIB3DS_MESH_TEXTURE_INFO:
        {
          int i,j;

          for (i=0; i<2; ++i) {
            mesh->map_data.tile[i]=lib3ds_io_read_float(io);
          }
          for (i=0; i<3; ++i) {
            mesh->map_data.pos[i]=lib3ds_io_read_float(io);
          }
          mesh->map_data.scale=lib3ds_io_read_float(io);

          lib3ds_matrix_identity(mesh->map_data.matrix);
          for (i=0; i<4; i++) {
            for (j=0; j<3; j++) {
              mesh->map_data.matrix[i][j]=lib3ds_io_read_float(io);
            }
          }
          for (i=0; i<2; ++i) {
            mesh->map_data.planar_size[i]=lib3ds_io_read_float(io);
          }
          mesh->map_data.cylinder_height=lib3ds_io_read_float(io);
        }
        break;
      case LIB3DS_TEX_VERTS:
        {
          unsigned i;
          unsigned texels;
          
          lib3ds_mesh_free_texel_list(mesh);
          texels=lib3ds_io_read_word(io);
          if (texels) {
            if (!lib3ds_mesh_new_texel_list(mesh, texels)) {
              LIB3DS_ERROR_LOG;
              return(LIB3DS_FALSE);
            }
            for (i=0; i<mesh->texels; ++i) {
              mesh->texelL[i][0]=lib3ds_io_read_float(io);
              mesh->texelL[i][1]=lib3ds_io_read_float(io);
            }
            ASSERT((!mesh->points) || (mesh->texels==mesh->points));
            ASSERT((!mesh->flags) || (mesh->texels==mesh->flags));
          }
        }
        break;
      default:
        lib3ds_chunk_unknown(chunk);
    }
  }
  {
    unsigned j;

    for (j=0; j<mesh->faces; ++j) {
      ASSERT(mesh->faceL[j].points[0]<mesh->points);
      ASSERT(mesh->faceL[j].points[1]<mesh->points);
      ASSERT(mesh->faceL[j].points[2]<mesh->points);
      lib3ds_vector_normal(
        mesh->faceL[j].normal,
        mesh->pointL[mesh->faceL[j].points[0]].pos,
        mesh->pointL[mesh->faceL[j].points[1]].pos,
        mesh->pointL[mesh->faceL[j].points[2]].pos
      );
    }
  }

  if (lib3ds_matrix_det(mesh->matrix) < 0.0)
  {
    /* Flip X coordinate of vertices if mesh matrix 
       has negative determinant */
    Lib3dsMatrix inv_matrix, M;
    Lib3dsVector tmp;
    unsigned i;

    lib3ds_matrix_copy(inv_matrix, mesh->matrix);
    lib3ds_matrix_inv(inv_matrix);

    lib3ds_matrix_copy(M, mesh->matrix);
    lib3ds_matrix_scale_xyz(M, -1.0f, 1.0f, 1.0f);
    lib3ds_matrix_mult(M, inv_matrix);

    for (i=0; i<mesh->points; ++i) {
      lib3ds_vector_transform(tmp, M, mesh->pointL[i].pos);
      lib3ds_vector_copy(mesh->pointL[i].pos, tmp);
    }
  }

  lib3ds_chunk_read_end(&c, io);

  return(LIB3DS_TRUE);
}


static Lib3dsBool
point_array_write(Lib3dsMesh *mesh, Lib3dsIo *io)
{
  Lib3dsChunk c;
  unsigned i;

  if (!mesh->points || !mesh->pointL) {
    return(LIB3DS_TRUE);
  }
  ASSERT(mesh->points<0x10000);
  c.chunk=LIB3DS_POINT_ARRAY;
  c.size=8+12*mesh->points;
  lib3ds_chunk_write(&c, io);
  
  lib3ds_io_write_word(io, (Lib3dsWord)mesh->points);

  if (lib3ds_matrix_det(mesh->matrix) >= 0.0f) {
    for (i=0; i<mesh->points; ++i) {
      lib3ds_io_write_vector(io, mesh->pointL[i].pos);
    }
  }
  else {
    /* Flip X coordinate of vertices if mesh matrix 
       has negative determinant */
    Lib3dsMatrix inv_matrix, M;
    Lib3dsVector tmp;

    lib3ds_matrix_copy(inv_matrix, mesh->matrix);
    lib3ds_matrix_inv(inv_matrix);
    lib3ds_matrix_copy(M, mesh->matrix);
    lib3ds_matrix_scale_xyz(M, -1.0f, 1.0f, 1.0f);
    lib3ds_matrix_mult(M, inv_matrix);

    for (i=0; i<mesh->points; ++i) {
      lib3ds_vector_transform(tmp, M, mesh->pointL[i].pos);
      lib3ds_io_write_vector(io, tmp);
    }
  }

  return(LIB3DS_TRUE);
}


static Lib3dsBool
flag_array_write(Lib3dsMesh *mesh, Lib3dsIo *io)
{
  Lib3dsChunk c;
  unsigned i;
  
  if (!mesh->flags || !mesh->flagL) {
    return(LIB3DS_TRUE);
  }
  ASSERT(mesh->flags<0x10000);
  c.chunk=LIB3DS_POINT_FLAG_ARRAY;
  c.size=8+2*mesh->flags;
  lib3ds_chunk_write(&c, io);
  
  lib3ds_io_write_word(io, (Lib3dsWord)mesh->flags);
  for (i=0; i<mesh->flags; ++i) {
    lib3ds_io_write_word(io, mesh->flagL[i]);
  }
  return(LIB3DS_TRUE);
}


static Lib3dsBool
face_array_write(Lib3dsMesh *mesh, Lib3dsIo *io)
{
  Lib3dsChunk c;
  
  if (!mesh->faces || !mesh->faceL) {
    return(LIB3DS_TRUE);
  }
  ASSERT(mesh->faces<0x10000);
  c.chunk=LIB3DS_FACE_ARRAY;
  if (!lib3ds_chunk_write_start(&c, io)) {
    return(LIB3DS_FALSE);
  }
  {
    unsigned i;

    lib3ds_io_write_word(io, (Lib3dsWord)mesh->faces);
    for (i=0; i<mesh->faces; ++i) {
      lib3ds_io_write_word(io, mesh->faceL[i].points[0]);
      lib3ds_io_write_word(io, mesh->faceL[i].points[1]);
      lib3ds_io_write_word(io, mesh->faceL[i].points[2]);
      lib3ds_io_write_word(io, mesh->faceL[i].flags);
    }
  }

  { /*---- MSH_MAT_GROUP ----*/
    Lib3dsChunk c;
    unsigned i,j;
    Lib3dsWord num;
    char *matf=calloc(sizeof(char), mesh->faces);
    if (!matf) {
      return(LIB3DS_FALSE);
    }
    
    for (i=0; i<mesh->faces; ++i) {
      if (!matf[i] && strlen(mesh->faceL[i].material)) {
        matf[i]=1;
        num=1;
        
        for (j=i+1; j<mesh->faces; ++j) {
          if (strcmp(mesh->faceL[i].material, mesh->faceL[j].material)==0) ++num;
        }
        
        c.chunk=LIB3DS_MSH_MAT_GROUP;
        c.size=6+ (Lib3dsDword)strlen(mesh->faceL[i].material)+1 +2+2*num;
        lib3ds_chunk_write(&c, io);
        lib3ds_io_write_string(io, mesh->faceL[i].material);
        lib3ds_io_write_word(io, num);
        lib3ds_io_write_word(io, (Lib3dsWord)i);
        
        for (j=i+1; j<mesh->faces; ++j) {
          if (strcmp(mesh->faceL[i].material, mesh->faceL[j].material)==0) {
            lib3ds_io_write_word(io, (Lib3dsWord)j);
            matf[j]=1;
          }
        }
      }      
    }
    free(matf);
  }

  { /*---- SMOOTH_GROUP ----*/
    Lib3dsChunk c;
    unsigned i;
    
    c.chunk=LIB3DS_SMOOTH_GROUP;
    c.size=6+4*mesh->faces;
    lib3ds_chunk_write(&c, io);
    
    for (i=0; i<mesh->faces; ++i) {
      lib3ds_io_write_dword(io, mesh->faceL[i].smoothing);
    }
  }
  
  { /*---- MSH_BOXMAP ----*/
    Lib3dsChunk c;

    if (strlen(mesh->box_map.front) ||
      strlen(mesh->box_map.back) ||
      strlen(mesh->box_map.left) ||
      strlen(mesh->box_map.right) ||
      strlen(mesh->box_map.top) ||
      strlen(mesh->box_map.bottom)) {
    
      c.chunk=LIB3DS_MSH_BOXMAP;
      if (!lib3ds_chunk_write_start(&c, io)) {
        return(LIB3DS_FALSE);
      }
      
      lib3ds_io_write_string(io, mesh->box_map.front);
      lib3ds_io_write_string(io, mesh->box_map.back);
      lib3ds_io_write_string(io, mesh->box_map.left);
      lib3ds_io_write_string(io, mesh->box_map.right);
      lib3ds_io_write_string(io, mesh->box_map.top);
      lib3ds_io_write_string(io, mesh->box_map.bottom);
      
      if (!lib3ds_chunk_write_end(&c, io)) {
        return(LIB3DS_FALSE);
      }
    }
  }

  if (!lib3ds_chunk_write_end(&c, io)) {
    return(LIB3DS_FALSE);
  }
  return(LIB3DS_TRUE);
}


static Lib3dsBool
texel_array_write(Lib3dsMesh *mesh, Lib3dsIo *io)
{
  Lib3dsChunk c;
  unsigned i;
  
  if (!mesh->texels || !mesh->texelL) {
    return(LIB3DS_TRUE);
  }
  ASSERT(mesh->texels<0x10000);
  c.chunk=LIB3DS_TEX_VERTS;
  c.size=8+8*mesh->texels;
  lib3ds_chunk_write(&c, io);
  
  lib3ds_io_write_word(io, (Lib3dsWord)mesh->texels);
  for (i=0; i<mesh->texels; ++i) {
    lib3ds_io_write_float(io, mesh->texelL[i][0]);
    lib3ds_io_write_float(io, mesh->texelL[i][1]);
  }
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_write(Lib3dsMesh *mesh, Lib3dsIo *io)
{
  Lib3dsChunk c;

  c.chunk=LIB3DS_N_TRI_OBJECT;
  if (!lib3ds_chunk_write_start(&c,io)) {
    return(LIB3DS_FALSE);
  }
  if (!point_array_write(mesh, io)) {
    return(LIB3DS_FALSE);
  }
  if (!texel_array_write(mesh, io)) {
    return(LIB3DS_FALSE);
  }

  if (mesh->map_data.maptype!=LIB3DS_MAP_NONE) { /*---- LIB3DS_MESH_TEXTURE_INFO ----*/
    Lib3dsChunk c;
    int i,j;
    
    c.chunk=LIB3DS_MESH_TEXTURE_INFO;
    c.size=92;
    if (!lib3ds_chunk_write(&c,io)) {
      return(LIB3DS_FALSE);
    }

    lib3ds_io_write_word(io, mesh->map_data.maptype);

    for (i=0; i<2; ++i) {
      lib3ds_io_write_float(io, mesh->map_data.tile[i]);
    }
    for (i=0; i<3; ++i) {
      lib3ds_io_write_float(io, mesh->map_data.pos[i]);
    }
    lib3ds_io_write_float(io, mesh->map_data.scale);

    for (i=0; i<4; i++) {
      for (j=0; j<3; j++) {
        lib3ds_io_write_float(io, mesh->map_data.matrix[i][j]);
      }
    }
    for (i=0; i<2; ++i) {
      lib3ds_io_write_float(io, mesh->map_data.planar_size[i]);
    }
    lib3ds_io_write_float(io, mesh->map_data.cylinder_height);
  }

  if (!flag_array_write(mesh, io)) {
    return(LIB3DS_FALSE);
  }
  { /*---- LIB3DS_MESH_MATRIX ----*/
    Lib3dsChunk c;
    int i,j;

    c.chunk=LIB3DS_MESH_MATRIX;
    c.size=54;
    if (!lib3ds_chunk_write(&c,io)) {
      return(LIB3DS_FALSE);
    }
    for (i=0; i<4; i++) {
      for (j=0; j<3; j++) {
        lib3ds_io_write_float(io, mesh->matrix[i][j]);
      }
    }
  }

  if (mesh->color) { /*---- LIB3DS_MESH_COLOR ----*/
    Lib3dsChunk c;
    
    c.chunk=LIB3DS_MESH_COLOR;
    c.size=7;
    if (!lib3ds_chunk_write(&c,io)) {
      return(LIB3DS_FALSE);
    }
    lib3ds_io_write_byte(io, mesh->color);
  }
  if (!face_array_write(mesh, io)) {
    return(LIB3DS_FALSE);
  }

  if (!lib3ds_chunk_write_end(&c,io)) {
    return(LIB3DS_FALSE);
  }
  return(LIB3DS_TRUE);
}

