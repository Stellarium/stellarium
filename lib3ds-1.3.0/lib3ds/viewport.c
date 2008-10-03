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
 * $Id: viewport.c,v 1.11 2007/06/20 17:04:09 jeh Exp $
 */
#include <lib3ds/viewport.h>
#include <lib3ds/chunk.h>
#include <lib3ds/io.h>
#include <stdlib.h>
#include <string.h>


/*!
 * \defgroup viewport Viewport and default view settings
 */


/*!
 * \ingroup viewport 
 */
Lib3dsBool
lib3ds_viewport_read(Lib3dsViewport *viewport, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;

  if (!lib3ds_chunk_read_start(&c, 0, io)) {
    return(LIB3DS_FALSE);
  }
  
  switch (c.chunk) {
    case LIB3DS_VIEWPORT_LAYOUT:
      {
        int cur=0;
        viewport->layout.style=lib3ds_io_read_word(io);
        viewport->layout.active=lib3ds_io_read_intw(io);
        lib3ds_io_read_intw(io);
        viewport->layout.swap=lib3ds_io_read_intw(io);
        lib3ds_io_read_intw(io);
        viewport->layout.swap_prior=lib3ds_io_read_intw(io);
        viewport->layout.swap_view=lib3ds_io_read_intw(io);
        lib3ds_chunk_read_tell(&c, io);
        while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
          switch (chunk) {
            case LIB3DS_VIEWPORT_SIZE:
              {
                viewport->layout.position[0]=lib3ds_io_read_word(io);
                viewport->layout.position[1]=lib3ds_io_read_word(io);
                viewport->layout.size[0]=lib3ds_io_read_word(io);
                viewport->layout.size[1]=lib3ds_io_read_word(io);
              }
              break;
            case LIB3DS_VIEWPORT_DATA_3:
              {
                lib3ds_viewport_set_views(viewport,cur+1);
                lib3ds_io_read_intw(io);
                viewport->layout.viewL[cur].axis_lock=lib3ds_io_read_word(io);
                viewport->layout.viewL[cur].position[0]=lib3ds_io_read_intw(io);
                viewport->layout.viewL[cur].position[1]=lib3ds_io_read_intw(io);
                viewport->layout.viewL[cur].size[0]=lib3ds_io_read_intw(io);
                viewport->layout.viewL[cur].size[1]=lib3ds_io_read_intw(io);
                viewport->layout.viewL[cur].type=lib3ds_io_read_word(io);
                viewport->layout.viewL[cur].zoom=lib3ds_io_read_float(io);
                lib3ds_io_read_vector(io, viewport->layout.viewL[cur].center);
                viewport->layout.viewL[cur].horiz_angle=lib3ds_io_read_float(io);
                viewport->layout.viewL[cur].vert_angle=lib3ds_io_read_float(io);
                lib3ds_io_read(io, viewport->layout.viewL[cur].camera, 11);
                ++cur;
              }
              break;
            case LIB3DS_VIEWPORT_DATA:
              /* 3DS R2 & R3 chunk
                 unsupported */
              break;
            default:
              lib3ds_chunk_unknown(chunk);
          }
        }
      }
      break;
    case LIB3DS_DEFAULT_VIEW:
      {
        memset(&viewport->default_view,0,sizeof(Lib3dsDefaultView));
        while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
          switch (chunk) {
            case LIB3DS_VIEW_TOP:
              {
                viewport->default_view.type=LIB3DS_VIEW_TYPE_TOP;
                lib3ds_io_read_vector(io, viewport->default_view.position);
                viewport->default_view.width=lib3ds_io_read_float(io);
              }
              break;
            case LIB3DS_VIEW_BOTTOM:
              {
                viewport->default_view.type=LIB3DS_VIEW_TYPE_BOTTOM;
                lib3ds_io_read_vector(io, viewport->default_view.position);
                viewport->default_view.width=lib3ds_io_read_float(io);
              }
              break;
            case LIB3DS_VIEW_LEFT:
              {
                viewport->default_view.type=LIB3DS_VIEW_TYPE_LEFT;
                lib3ds_io_read_vector(io, viewport->default_view.position);
                viewport->default_view.width=lib3ds_io_read_float(io);
              }
              break;
            case LIB3DS_VIEW_RIGHT:
              {
                viewport->default_view.type=LIB3DS_VIEW_TYPE_RIGHT;
                lib3ds_io_read_vector(io, viewport->default_view.position);
                viewport->default_view.width=lib3ds_io_read_float(io);
              }
              break;
            case LIB3DS_VIEW_FRONT:
              {
                viewport->default_view.type=LIB3DS_VIEW_TYPE_FRONT;
                lib3ds_io_read_vector(io, viewport->default_view.position);
                viewport->default_view.width=lib3ds_io_read_float(io);
              }
              break;
            case LIB3DS_VIEW_BACK:
              {
                viewport->default_view.type=LIB3DS_VIEW_TYPE_BACK;
                lib3ds_io_read_vector(io, viewport->default_view.position);
                viewport->default_view.width=lib3ds_io_read_float(io);
              }
              break;
            case LIB3DS_VIEW_USER:
              {
                viewport->default_view.type=LIB3DS_VIEW_TYPE_USER;
                lib3ds_io_read_vector(io, viewport->default_view.position);
                viewport->default_view.width=lib3ds_io_read_float(io);
                viewport->default_view.horiz_angle=lib3ds_io_read_float(io);
                viewport->default_view.vert_angle=lib3ds_io_read_float(io);
                viewport->default_view.roll_angle=lib3ds_io_read_float(io);
              }
              break;
            case LIB3DS_VIEW_CAMERA:
              {
                viewport->default_view.type=LIB3DS_VIEW_TYPE_CAMERA;
                lib3ds_io_read(io, viewport->default_view.camera, 11);
              }
              break;
            default:
              lib3ds_chunk_unknown(chunk);
          }
        }
      }
      break;
  }

  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup viewport 
 */
void
lib3ds_viewport_set_views(Lib3dsViewport *viewport, Lib3dsDword views)
{
  ASSERT(viewport);
  if (viewport->layout.views) {
    if (views) {
      viewport->layout.views=views;
      viewport->layout.viewL=(Lib3dsView*)realloc(viewport->layout.viewL, sizeof(Lib3dsView)*views);
    }
    else {
      free(viewport->layout.viewL);
      viewport->layout.views=0;
      viewport->layout.viewL=0;
    }
  }
  else {
    if (views) {
      viewport->layout.views=views;
      viewport->layout.viewL=(Lib3dsView*)calloc(sizeof(Lib3dsView),views);
    }
  }
}


/*!
 * \ingroup viewport 
 */
Lib3dsBool
lib3ds_viewport_write(Lib3dsViewport *viewport, Lib3dsIo *io)
{
  if (viewport->layout.views) {
    Lib3dsChunk c;
    unsigned i;

    c.chunk=LIB3DS_VIEWPORT_LAYOUT;
    if (!lib3ds_chunk_write_start(&c,io)) {
      return(LIB3DS_FALSE);
    }

    lib3ds_io_write_word(io, viewport->layout.style);
    lib3ds_io_write_intw(io, viewport->layout.active);
    lib3ds_io_write_intw(io, 0);
    lib3ds_io_write_intw(io, viewport->layout.swap);
    lib3ds_io_write_intw(io, 0);
    lib3ds_io_write_intw(io, viewport->layout.swap_prior);
    lib3ds_io_write_intw(io, viewport->layout.swap_view);
    
    {
      Lib3dsChunk c;
      c.chunk=LIB3DS_VIEWPORT_SIZE;
      c.size=14;
      lib3ds_chunk_write(&c,io);
      lib3ds_io_write_intw(io, viewport->layout.position[0]);
      lib3ds_io_write_intw(io, viewport->layout.position[1]);
      lib3ds_io_write_intw(io, viewport->layout.size[0]);
      lib3ds_io_write_intw(io, viewport->layout.size[1]);
    }

    for (i=0; i<viewport->layout.views; ++i) {
      Lib3dsChunk c;
      c.chunk=LIB3DS_VIEWPORT_DATA_3;
      c.size=55;
      lib3ds_chunk_write(&c,io);

      lib3ds_io_write_intw(io, 0);
      lib3ds_io_write_word(io, viewport->layout.viewL[i].axis_lock);
      lib3ds_io_write_intw(io, viewport->layout.viewL[i].position[0]);
      lib3ds_io_write_intw(io, viewport->layout.viewL[i].position[1]);
      lib3ds_io_write_intw(io, viewport->layout.viewL[i].size[0]);
      lib3ds_io_write_intw(io, viewport->layout.viewL[i].size[1]);
      lib3ds_io_write_word(io, viewport->layout.viewL[i].type);
      lib3ds_io_write_float(io, viewport->layout.viewL[i].zoom);
      lib3ds_io_write_vector(io, viewport->layout.viewL[i].center);
      lib3ds_io_write_float(io, viewport->layout.viewL[i].horiz_angle);
      lib3ds_io_write_float(io, viewport->layout.viewL[i].vert_angle);
      lib3ds_io_write(io, viewport->layout.viewL[i].camera,11);
    }

    if (!lib3ds_chunk_write_end(&c,io)) {
      return(LIB3DS_FALSE);
    }
  }

  if (viewport->default_view.type) {
    Lib3dsChunk c;

    c.chunk=LIB3DS_DEFAULT_VIEW;
    if (!lib3ds_chunk_write_start(&c,io)) {
      return(LIB3DS_FALSE);
    }

    switch (viewport->default_view.type) {
      case LIB3DS_VIEW_TYPE_TOP:
        {
          Lib3dsChunk c;
          c.chunk=LIB3DS_VIEW_TOP;
          c.size=22;
          lib3ds_chunk_write(&c,io);
          lib3ds_io_write_vector(io, viewport->default_view.position);
          lib3ds_io_write_float(io, viewport->default_view.width);
        }
        break;
      case LIB3DS_VIEW_TYPE_BOTTOM:
        {
          Lib3dsChunk c;
          c.chunk=LIB3DS_VIEW_BOTTOM;
          c.size=22;
          lib3ds_chunk_write(&c,io);
          lib3ds_io_write_vector(io, viewport->default_view.position);
          lib3ds_io_write_float(io, viewport->default_view.width);
        }
        break;
      case LIB3DS_VIEW_TYPE_LEFT:
        {
          Lib3dsChunk c;
          c.chunk=LIB3DS_VIEW_LEFT;
          c.size=22;
          lib3ds_chunk_write(&c,io);
          lib3ds_io_write_vector(io, viewport->default_view.position);
          lib3ds_io_write_float(io, viewport->default_view.width);
        }
        break;
      case LIB3DS_VIEW_TYPE_RIGHT:
        {
          Lib3dsChunk c;
          c.chunk=LIB3DS_VIEW_RIGHT;
          c.size=22;
          lib3ds_chunk_write(&c,io);
          lib3ds_io_write_vector(io, viewport->default_view.position);
          lib3ds_io_write_float(io, viewport->default_view.width);
        }
        break;
      case LIB3DS_VIEW_TYPE_FRONT:
        {
          Lib3dsChunk c;
          c.chunk=LIB3DS_VIEW_FRONT;
          c.size=22;
          lib3ds_chunk_write(&c,io);
          lib3ds_io_write_vector(io, viewport->default_view.position);
          lib3ds_io_write_float(io, viewport->default_view.width);
        }
        break;
      case LIB3DS_VIEW_TYPE_BACK:
        {
          Lib3dsChunk c;
          c.chunk=LIB3DS_VIEW_BACK;
          c.size=22;
          lib3ds_chunk_write(&c,io);
          lib3ds_io_write_vector(io, viewport->default_view.position);
          lib3ds_io_write_float(io, viewport->default_view.width);
        }
        break;
      case LIB3DS_VIEW_TYPE_USER:
        {
          Lib3dsChunk c;
          c.chunk=LIB3DS_VIEW_USER;
          c.size=34;
          lib3ds_chunk_write(&c,io);
          lib3ds_io_write_vector(io, viewport->default_view.position);
          lib3ds_io_write_float(io, viewport->default_view.width);
          lib3ds_io_write_float(io, viewport->default_view.horiz_angle);
          lib3ds_io_write_float(io, viewport->default_view.vert_angle);
          lib3ds_io_write_float(io, viewport->default_view.roll_angle);
        }
        break;
      case LIB3DS_VIEW_TYPE_CAMERA:
        {
          Lib3dsChunk c;
          c.chunk=LIB3DS_VIEW_CAMERA;
          c.size=17;
          lib3ds_chunk_write(&c, io);
          lib3ds_io_write(io, viewport->default_view.camera, 11);
        }
        break;
    }

    if (!lib3ds_chunk_write_end(&c, io)) {
      return(LIB3DS_FALSE);
    }
  }
  return(LIB3DS_TRUE);
}


/*!
 * Dump viewport.
 *
 * \param vp The viewport to be dumped.
 *
 * \ingroup node
 */
void
lib3ds_viewport_dump(Lib3dsViewport *vp)
{
  Lib3dsView *view;
  unsigned i;
  ASSERT(vp);

  printf("  viewport:\n");
  printf("    layout:\n");
  printf("      style:       %d\n", vp->layout.style);
  printf("      active:      %d\n", vp->layout.active);
  printf("      swap:        %d\n", vp->layout.swap);
  printf("      swap_prior:  %d\n", vp->layout.swap_prior);
  printf("      position:    %d,%d\n",
  	vp->layout.position[0], vp->layout.position[1]);
  printf("      size:        %d,%d\n", vp->layout.size[0], vp->layout.size[1]);
  printf("      views:       %ld\n", vp->layout.views);
  if (vp->layout.views > 0 && vp->layout.viewL != NULL) {
    for(i=0, view=vp->layout.viewL; i < vp->layout.views; ++i, ++view) {
      printf("        view %d:\n", i);
      printf("          type:         %d\n", view->type);
      printf("          axis_lock:    %d\n", view->axis_lock);
      printf("          position:     (%d,%d)\n",
        view->position[0], view->position[1]);
      printf("          size:         (%d,%d)\n", view->size[0], view->size[1]);
      printf("          zoom:         %g\n", view->zoom);
      printf("          center:       (%g,%g,%g)\n",
        view->center[0], view->center[1], view->center[2]);
      printf("          horiz_angle:  %g\n", view->horiz_angle);
      printf("          vert_angle:   %g\n", view->vert_angle);
      printf("          camera:       %s\n", view->camera);
    }
  }

  printf("    default_view:\n");
  printf("	type:         %d\n", vp->default_view.type);
  printf("	position:     (%g,%g,%g)\n",
    vp->default_view.position[0],
    vp->default_view.position[1],
    vp->default_view.position[2]);
  printf("	width:        %g\n", vp->default_view.width);
  printf("	horiz_angle:  %g\n", vp->default_view.horiz_angle);
  printf("	vert_angle:   %g\n", vp->default_view.vert_angle);
  printf("	roll_angle:   %g\n", vp->default_view.roll_angle);
  printf("	camera:       %s\n", vp->default_view.camera);
}

