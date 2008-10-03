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
 * $Id: light.c,v 1.15 2007/06/20 17:04:08 jeh Exp $
 */
#include <lib3ds/light.h>
#include <lib3ds/chunk.h>
#include <lib3ds/io.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


/*!
 * \defgroup light Lights
 */
/*!


/*!
 * \ingroup light
 */
Lib3dsLight*
lib3ds_light_new(const char *name)
{
  Lib3dsLight *light;

  ASSERT(name);
  ASSERT(strlen(name)<64);
  
  light=(Lib3dsLight*)calloc(sizeof(Lib3dsLight), 1);
  if (!light) {
    return(0);
  }
  strcpy(light->name, name);
  return(light);
}


/*!
 * \ingroup light
 */
void
lib3ds_light_free(Lib3dsLight *light)
{
  memset(light, 0, sizeof(Lib3dsLight));
  free(light);
}


/*!
 * \ingroup light
 */
void
lib3ds_light_dump(Lib3dsLight *light)
{
  ASSERT(light);
  printf("  name:             %s\n", light->name);
  printf("  spot_light:       %s\n", light->spot_light ? "yes" : "no");
  printf("  see_cone:         %s\n", light->see_cone ? "yes" : "no");
  printf("  color:            (%f, %f, %f)\n", 
    light->color[0], light->color[1], light->color[2]);
  printf("  position          (%f, %f, %f)\n", 
    light->position[0], light->position[1], light->position[2]);
  printf("  spot              (%f, %f, %f)\n", 
    light->spot[0], light->spot[1], light->spot[2]);
  printf("  roll:             %f\n", light->roll);
  printf("  off:              %s\n", light->off ? "yes" : "no");
  printf("  outer_range:      %f\n", light->outer_range);
  printf("  inner_range:      %f\n", light->inner_range);
  printf("  multiplier:       %f\n", light->multiplier);
  printf("  attenuation:      %f\n", light->attenuation);
  printf("  rectangular_spot: %s\n", light->rectangular_spot ? "yes" : "no");
  printf("  shadowed:         %s\n", light->shadowed ? "yes" : "no");
  printf("  shadow_bias:      %f\n", light->shadow_bias);
  printf("  shadow_filter:    %f\n", light->shadow_filter);
  printf("  shadow_size:      %d\n", light->shadow_size);
  printf("  spot_aspect:      %f\n", light->spot_aspect);
  printf("  use_projector:    %s\n", light->use_projector ? "yes" : "no");
  printf("  projector:        %s\n", light->projector);
  printf("  spot_overshoot:   %d\n", (int)light->spot_overshoot);
  printf("  ray_shadows:      %s\n", light->ray_shadows ? "yes" : "no");
  printf("  ray_bias:         %f\n", light->ray_bias);
  printf("  hot_spot:         %f\n", light->hot_spot);
  printf("  fall_off:         %f\n", light->fall_off);
  printf("\n");
}


/*!
 * \ingroup light
 */
static Lib3dsBool
spotlight_read(Lib3dsLight *light, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;
  int i;

  if (!lib3ds_chunk_read_start(&c, LIB3DS_DL_SPOTLIGHT, io)) {
    return(LIB3DS_FALSE);
  }
  light->spot_light=LIB3DS_TRUE;
  for (i=0; i<3; ++i) {
    light->spot[i]=lib3ds_io_read_float(io);
  }
  light->hot_spot = lib3ds_io_read_float(io);
  light->fall_off = lib3ds_io_read_float(io);
  lib3ds_chunk_read_tell(&c, io);
  
  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_DL_SPOT_ROLL:
        {
          light->roll=lib3ds_io_read_float(io);
        }
        break;
      case LIB3DS_DL_SHADOWED:
        {
          light->shadowed=LIB3DS_TRUE;
        }
        break;
      case LIB3DS_DL_LOCAL_SHADOW2:
        {
          light->shadow_bias=lib3ds_io_read_float(io);
          light->shadow_filter=lib3ds_io_read_float(io);
          light->shadow_size=lib3ds_io_read_intw(io);
        }
        break;
      case LIB3DS_DL_SEE_CONE:
        {
          light->see_cone=LIB3DS_TRUE;
        }
        break;
      case LIB3DS_DL_SPOT_RECTANGULAR:
        {
          light->rectangular_spot=LIB3DS_TRUE;
        }
        break;
      case LIB3DS_DL_SPOT_ASPECT:
        {
          light->spot_aspect=lib3ds_io_read_float(io);
        }
        break;
      case LIB3DS_DL_SPOT_PROJECTOR:
        {
          light->use_projector=LIB3DS_TRUE;
          if (!lib3ds_io_read_string(io, light->projector, 64)) {
            return(LIB3DS_FALSE);
          }
        }
      case LIB3DS_DL_SPOT_OVERSHOOT:
        {
          light->spot_overshoot=LIB3DS_TRUE;
        }
        break;
      case LIB3DS_DL_RAY_BIAS:
        {
          light->ray_bias=lib3ds_io_read_float(io);
        }
        break;
      case LIB3DS_DL_RAYSHAD:
        {
          light->ray_shadows=LIB3DS_TRUE;
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
 * \ingroup light
 */
Lib3dsBool
lib3ds_light_read(Lib3dsLight *light, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;

  if (!lib3ds_chunk_read_start(&c, LIB3DS_N_DIRECT_LIGHT, io)) {
    return(LIB3DS_FALSE);
  }
  {
    int i;
    for (i=0; i<3; ++i) {
      light->position[i]=lib3ds_io_read_float(io);
    }
  }
  lib3ds_chunk_read_tell(&c, io);
  
  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_COLOR_F:
        {
          int i;
          for (i=0; i<3; ++i) {
            light->color[i]=lib3ds_io_read_float(io);
          }
        }
        break;
      case LIB3DS_DL_OFF:
        {
          light->off=LIB3DS_TRUE;
        }
        break;
      case LIB3DS_DL_OUTER_RANGE:
        {
          light->outer_range=lib3ds_io_read_float(io);
        }
        break;
      case LIB3DS_DL_INNER_RANGE:
        {
          light->inner_range=lib3ds_io_read_float(io);
        }
        break;
      case LIB3DS_DL_MULTIPLIER:
        {
          light->multiplier=lib3ds_io_read_float(io);
        }
        break;
      case LIB3DS_DL_EXCLUDE:
        {
          /* FIXME: */
          lib3ds_chunk_unknown(chunk);
        }
      case LIB3DS_DL_ATTENUATE:
        {
          light->attenuation=lib3ds_io_read_float(io);
        }
        break;
      case LIB3DS_DL_SPOTLIGHT:
        {
          lib3ds_chunk_read_reset(&c, io);
          if (!spotlight_read(light, io)) {
            return(LIB3DS_FALSE);
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
 * \ingroup light
 */
Lib3dsBool
lib3ds_light_write(Lib3dsLight *light, Lib3dsIo *io)
{
  Lib3dsChunk c;

  c.chunk=LIB3DS_N_DIRECT_LIGHT;
  if (!lib3ds_chunk_write_start(&c,io)) {
    return(LIB3DS_FALSE);
  }
  lib3ds_io_write_vector(io, light->position);
  { /*---- LIB3DS_COLOR_F ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_COLOR_F;
    c.size=18;
    lib3ds_chunk_write(&c, io);
    lib3ds_io_write_rgb(io, light->color);
  }
  if (light->off) { /*---- LIB3DS_DL_OFF ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_DL_OFF;
    c.size=6;
    lib3ds_chunk_write(&c, io);
  }
  { /*---- LIB3DS_DL_OUTER_RANGE ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_DL_OUTER_RANGE;
    c.size=10;
    lib3ds_chunk_write(&c, io);
    lib3ds_io_write_float(io, light->outer_range);
  }
  { /*---- LIB3DS_DL_INNER_RANGE ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_DL_INNER_RANGE;
    c.size=10;
    lib3ds_chunk_write(&c, io);
    lib3ds_io_write_float(io, light->inner_range);
  }
  { /*---- LIB3DS_DL_MULTIPLIER ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_DL_MULTIPLIER;
    c.size=10;
    lib3ds_chunk_write(&c, io);
    lib3ds_io_write_float(io, light->multiplier);
  }
  if (light->attenuation) { /*---- LIB3DS_DL_ATTENUATE ----*/
    Lib3dsChunk c;
    c.chunk=LIB3DS_DL_ATTENUATE;
    c.size=6;
    lib3ds_chunk_write(&c, io);
  }

  if (light->spot_light) {
    Lib3dsChunk c;

    c.chunk=LIB3DS_DL_SPOTLIGHT;
    if (!lib3ds_chunk_write_start(&c,io)) {
      return(LIB3DS_FALSE);
    }
    lib3ds_io_write_vector(io, light->spot);
    lib3ds_io_write_float(io, light->hot_spot);
    lib3ds_io_write_float(io, light->fall_off);
    
    { /*---- LIB3DS_DL_SPOT_ROLL ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_SPOT_ROLL;
      c.size=10;
      lib3ds_chunk_write(&c, io);
      lib3ds_io_write_float(io, light->roll);
    }
    if (light->shadowed) { /*---- LIB3DS_DL_SHADOWED ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_SHADOWED;
      c.size=6;
      lib3ds_chunk_write(&c, io);
    }
    if ((fabs(light->shadow_bias)>LIB3DS_EPSILON) ||
      (fabs(light->shadow_filter)>LIB3DS_EPSILON) ||
      (light->shadow_size!=0)) { /*---- LIB3DS_DL_LOCAL_SHADOW2 ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_LOCAL_SHADOW2;
      c.size=16;
      lib3ds_chunk_write(&c, io);
      lib3ds_io_write_float(io, light->shadow_bias);
      lib3ds_io_write_float(io, light->shadow_filter);
      lib3ds_io_write_intw(io, light->shadow_size);
    }
    if (light->see_cone) { /*---- LIB3DS_DL_SEE_CONE ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_SEE_CONE;
      c.size=6;
      lib3ds_chunk_write(&c, io);
    }
    if (light->rectangular_spot) { /*---- LIB3DS_DL_SPOT_RECTANGULAR ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_SPOT_RECTANGULAR;
      c.size=6;
      lib3ds_chunk_write(&c, io);
    }
    if (fabs(light->spot_aspect)>LIB3DS_EPSILON) { /*---- LIB3DS_DL_SPOT_ASPECT ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_SPOT_ASPECT;
      c.size=10;
      lib3ds_chunk_write(&c, io);
      lib3ds_io_write_float(io, light->spot_aspect);
    }
    if (light->use_projector) { /*---- LIB3DS_DL_SPOT_PROJECTOR ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_SPOT_PROJECTOR;
      c.size=10;
      lib3ds_chunk_write(&c, io);
      lib3ds_io_write_string(io, light->projector);
    }
    if (light->spot_overshoot) { /*---- LIB3DS_DL_SPOT_OVERSHOOT ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_SPOT_OVERSHOOT;
      c.size=6;
      lib3ds_chunk_write(&c, io);
    }
    if (fabs(light->ray_bias)>LIB3DS_EPSILON) { /*---- LIB3DS_DL_RAY_BIAS ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_RAY_BIAS;
      c.size=10;
      lib3ds_chunk_write(&c, io);
      lib3ds_io_write_float(io, light->ray_bias);
    }
    if (light->ray_shadows) { /*---- LIB3DS_DL_RAYSHAD ----*/
      Lib3dsChunk c;
      c.chunk=LIB3DS_DL_RAYSHAD;
      c.size=6;
      lib3ds_chunk_write(&c, io);
    }
    if (!lib3ds_chunk_write_end(&c,io)) {
      return(LIB3DS_FALSE);
    }
  }
  if (!lib3ds_chunk_write_end(&c,io)) {
    return(LIB3DS_FALSE);
  }
  return(LIB3DS_TRUE);
}


