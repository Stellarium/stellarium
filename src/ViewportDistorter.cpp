/*
 * Stellarium
 * Copyright (C) Fabien Chereau
 * Author 2006 Johannes Gajdosik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "ViewportDistorter.hpp"
#include "StelApp.hpp"
#include "SphericMirrorCalculator.hpp"
#include "InitParser.hpp"
#include "Projector.hpp"

#include "SDL_opengl.h"

#include <math.h>

class ViewportDistorterDummy : public ViewportDistorter {
private:
  friend class ViewportDistorter;
  string getType(void) const {return "none";}
  void init(const InitParser&) {}
  void distort(void) const {}
  bool distortXY(int &x,int &y) const {return true;}
};


class ViewportDistorterFisheyeToSphericMirror : public ViewportDistorter {
private:
  friend class ViewportDistorter;
  ViewportDistorterFisheyeToSphericMirror(int screenW,int screenH,
                                          Projector *prj);
  ~ViewportDistorterFisheyeToSphericMirror(void);
  string getType(void) const {return "fisheye_to_spheric_mirror";}
  void init(const InitParser &conf);
  void distort(void) const;
  bool distortXY(int &x,int &y) const;
  Projector *const prj;
  const int screenW;
  const int screenH;
  unsigned int mirror_texture;
  int viewport_wh;
  float texture_used;
  struct TexturePoint {
    float tex_xy[2];
  };
  TexturePoint *texture_point_array;
  int max_x,max_y;
  double step_x,step_y;
  SphericMirrorCalculator calc;
  GLuint display_list;
  double original_max_fov;
};


ViewportDistorterFisheyeToSphericMirror
   ::ViewportDistorterFisheyeToSphericMirror(int screenW,int screenH,
                                             Projector *prj)
    :prj(prj),screenW(screenW),screenH(screenH),texture_point_array(0) {

  original_max_fov = prj->getMaxFov();

//  if (prj->getCurrentProjection() != "fisheye") {
//    cerr << "ViewportDistorterFisheyeToSphericMirror: "
//         << "what are you doing? the projection type should be fisheye."
//         << endl;
////    assert(0);
//  }
  viewport_wh = (screenW < screenH) ? screenW : screenH;
  int texture_wh = 1;
  while (texture_wh < viewport_wh) texture_wh <<= 1;
  texture_used = viewport_wh / (float)texture_wh;
  
  glGenTextures(1, &mirror_texture);
  glBindTexture(GL_TEXTURE_2D, mirror_texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_wh, texture_wh, 0,
               GL_RGB, GL_UNSIGNED_BYTE, 0);

//  calc.setParams(Vec3d(0,-2,15),Vec3d(0,0,20),1,25,0.125);
  display_list = glGenLists(1);
}

struct VertexPoint {
  float ver_xy[2];
  float color[4];
  double h;
};

void ViewportDistorterFisheyeToSphericMirror::init(const InitParser &conf) {
//cout << "ViewportDistorterFisheyeToSphericMirror::init: "
//     << prj->getCurrentProjection() << endl;
  calc.init(conf);
  const double gamma
    = conf.get_double("spheric_mirror","projector_gamma",0.45);
  double distorter_max_fov
    = conf.get_double("spheric_mirror","distorter_max_fov",175.0);
  if (distorter_max_fov > 240.0) distorter_max_fov = 240.0;
  else if (distorter_max_fov < 120.0) distorter_max_fov = 120.0;
  if (distorter_max_fov > prj->getMapping().maxFov)
    distorter_max_fov = prj->getMapping().maxFov;
  prj->setMaxFov(distorter_max_fov);
  const double fact
    = prj->getMapping().fovToViewScalingFactor(distorter_max_fov);
  double texture_triangle_base_length
    = conf.get_double("spheric_mirror","texture_triangle_base_length",16.0);  
  if (texture_triangle_base_length > 256.0)
    texture_triangle_base_length = 256.0;
  else if (texture_triangle_base_length < 2.0)
    texture_triangle_base_length = 2.0;
  
    // init transformation
  max_x = (int)trunc(0.5 + screenW/texture_triangle_base_length);
  step_x = screenW / (double)(max_x-0.5);
  max_y = (int)trunc(screenH/(texture_triangle_base_length*0.5*sqrt(3.0)));
  step_y = screenH/ (double)max_y;
  texture_point_array = new TexturePoint[(max_x+1)*(max_y+1)];
  VertexPoint *vertex_point_array = new VertexPoint[(max_x+1)*(max_x+1)];
  double max_h = 0;
  for (int j=0;j<=max_y;j++) {
    for (int i=0;i<=max_x;i++) {
      VertexPoint &vertex_point(vertex_point_array[(j*(max_x+1)+i)]);
      TexturePoint &texture_point(texture_point_array[(j*(max_x+1)+i)]);
      vertex_point.ver_xy[0] = ((i == 0) ? 0.0 :
				              (i == max_x) ? screenW :
				              (i-0.5*(j&1))*step_x);
      vertex_point.ver_xy[1] = j*step_y;
      Vec3d v,v_x,v_y;
      bool rc = calc.retransform(
                       (vertex_point.ver_xy[0]-0.5*screenW) / screenH,
                       (vertex_point.ver_xy[1]-0.5*screenH) / screenH,
                       v,v_x,v_y);
      double h = v[1];
      v[1] = v[2];
      v[2] = -h;

      rc &= prj->getMapping().forward(v);
      double x = (0.5 + v[0] * fact);
      double y = (0.5 + v[1] * fact);

      vertex_point.h = rc ? (v_x^v_y).length() : 0.0;

      if (x < 0.0) {x=0.0;vertex_point.h=0;}
      else if (x > 1.0) {x=1.0;vertex_point.h=0;}
      if (y < 0.0) {y=0.0;vertex_point.h=0;}
      else if (y > 1.0) {y=1.0;vertex_point.h=0;}
      texture_point.tex_xy[0] = x*texture_used;
      texture_point.tex_xy[1] = y*texture_used;
      if (vertex_point.h > max_h) max_h = vertex_point.h;
    }
  }
  for (int j=0;j<=max_y;j++) {
    for (int i=0;i<=max_x;i++) {
      VertexPoint &vertex_point(vertex_point_array[(j*(max_x+1)+i)]);
      vertex_point.color[0] = vertex_point.color[1] = vertex_point.color[2] =
        (vertex_point.h<=0.0) ? 0.0 : exp(gamma*log(vertex_point.h/max_h));
      vertex_point.color[3] = 1.0f;
    }
  }
  glNewList(display_list,GL_COMPILE);
  glMatrixMode(GL_PROJECTION);        // projection matrix mode
  glPushMatrix();                     // store previous matrix
  glLoadIdentity();
  gluOrtho2D(0,screenW,0,screenH);    // set a 2D orthographic projection
  glMatrixMode(GL_MODELVIEW);         // modelview matrix mode
  glPushMatrix();
  glLoadIdentity();
  for (int j=0;j<max_y;j++) {
    const TexturePoint *t0 = texture_point_array + j*(max_x+1);
    const TexturePoint *t1 = t0;
    const VertexPoint *v0 = vertex_point_array + j*(max_x+1);
    const VertexPoint *v1 = v0;
    if (j&1) {
      t1 += (max_x+1);
      v1 += (max_x+1);
    } else {
      t0 += (max_x+1);
      v0 += (max_x+1);
    }
    glBegin(GL_QUAD_STRIP);
    for (int i=0;i<=max_x;i++,t0++,t1++,v0++,v1++) {
      glColor4fv(v0->color);
      glTexCoord2fv(t0->tex_xy);
      glVertex2fv(v0->ver_xy);
      glColor4fv(v1->color);
      glTexCoord2fv(t1->tex_xy);
      glVertex2fv(v1->ver_xy);
    }
    glEnd();
  }
  glMatrixMode(GL_PROJECTION);        // Restore previous matrix
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glEndList();
  delete[] vertex_point_array;
}

ViewportDistorterFisheyeToSphericMirror
    ::~ViewportDistorterFisheyeToSphericMirror(void) {
  glDeleteLists(display_list,1);
  if (texture_point_array) delete[] texture_point_array;
  glDeleteTextures(1,&mirror_texture);
  prj->setMaxFov(original_max_fov);
}

bool ViewportDistorterFisheyeToSphericMirror::distortXY(int &x,int &y) const {
  const float f = viewport_wh/(float)texture_used;
  y = screenH-1-y;

    // find the triangle and interpolate accordingly:
  float dy = y / step_y;
  const int j = (int)floorf(dy);
  dy -= j;
  if (j&1) {
    float dx = x / step_x + 0.5*(1.0-dy);
    const int i = (int)floorf(dx);
    dx -= i;
    const TexturePoint *const t = texture_point_array + (j*(max_x+1)+i);
    if (dx + dy <= 1.f) {
      if (i == 0) {
        dx -= 0.5*(1.0-dy);
        dx *= 2.f;
      }
      x = (int)(floorf(0.5*(screenW-viewport_wh)
            + f * (t[0].tex_xy[0]
                  + dx * (t[1].tex_xy[0]-t[0].tex_xy[0])
                  + dy * (t[max_x+1].tex_xy[0]-t[0].tex_xy[0]))));
      y = (int)(floorf(0.5*(screenH-viewport_wh)
            + f * (t[0].tex_xy[1]
                   + dx * (t[1].tex_xy[1]-t[0].tex_xy[1])
                   + dy * (t[max_x+1].tex_xy[1]-t[0].tex_xy[1]))));
    } else {
      if (i == max_x-1) {
        dx -= 0.5*(1.0-dy);
        dx *= 2.f;
      }
      x = (int)(floorf(0.5*(screenW-viewport_wh)
            + f * (t[max_x+2].tex_xy[0]
                   + (1.f-dy) * (t[1].tex_xy[0]-t[max_x+2].tex_xy[0])
                   + (1.f-dx) * (t[max_x+1].tex_xy[0]-t[max_x+2].tex_xy[0]))));
      y = (int)(floorf(0.5*(screenH-viewport_wh)
            + f * (t[max_x+2].tex_xy[1]
                   + (1.f-dy) * (t[1].tex_xy[1]-t[max_x+2].tex_xy[1])
                   + (1.f-dx) * (t[max_x+1].tex_xy[1]-t[max_x+2].tex_xy[1]))));
    }
  } else {
    float dx = x / step_x + 0.5*dy;
    const int i = (int)floorf(dx);
    dx -= i;
    const TexturePoint *const t = texture_point_array + (j*(max_x+1)+i);
    if (dx >= dy) {
      if (i == max_x-1) {
        dx -= 0.5*dy;
        dx *= 2.f;
      }
      x = (int)(floorf(0.5*(screenW-viewport_wh)
            + f * (t[1].tex_xy[0]
                   + (1.f-dx) * (t[0].tex_xy[0]-t[1].tex_xy[0])
                   + dy * (t[max_x+2].tex_xy[0]-t[1].tex_xy[0]))));
      y = (int)(floorf(0.5*(screenH-viewport_wh)
            + f * (t[1].tex_xy[1]
                   + (1.f-dx) * (t[0].tex_xy[1]-t[1].tex_xy[1])
                   + dy * (t[max_x+2].tex_xy[1]-t[1].tex_xy[1]))));
    } else {
      if (i == 0) {
        dx -= 0.5*dy;
        dx *= 2.f;
      }
      x = (int)(floorf(0.5*(screenW-viewport_wh)
            + f * (t[max_x+1].tex_xy[0]
                   + (1.f-dy) * (t[0].tex_xy[0]-t[max_x+1].tex_xy[0])
                   + dx * (t[max_x+2].tex_xy[0]-t[max_x+1].tex_xy[0]))));
      y = (int)(floorf(0.5*(screenH-viewport_wh)
            + f * (t[max_x+1].tex_xy[1]
                   + (1.f-dy) * (t[0].tex_xy[1]-t[max_x+1].tex_xy[1])
                   + dx * (t[max_x+2].tex_xy[1]-t[max_x+1].tex_xy[1]))));
    }
  }
  
  y = screenH-1-y;
  return true;
}

void ViewportDistorterFisheyeToSphericMirror::distort(void) const {
  glViewport(0, 0, screenW, screenH);
  glBindTexture(GL_TEXTURE_2D, mirror_texture);
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                      (screenW-viewport_wh)>>1,(screenH-viewport_wh)>>1,
                      viewport_wh,viewport_wh);
  glEnable(GL_TEXTURE_2D);

  float color[4] = {1,1,1,1};
  glColor4fv(color);
  glDisable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, mirror_texture);

  glCallList(display_list);
}


ViewportDistorter *ViewportDistorter::create(const string &type,
                                             int width,int height,
                                             Projector *prj) {
  if (type == "fisheye_to_spheric_mirror") {
    return new ViewportDistorterFisheyeToSphericMirror(width,height,prj);
  }
  return new ViewportDistorterDummy;
}

