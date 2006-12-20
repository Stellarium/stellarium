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

#include "viewport_distorter.h"
#include "StelApp.h"
#include "spheric_mirror_calculator.h"
#include "InitParser.hpp"
#include "projector.h"

#include "SDL_opengl.h"

class ViewportDistorterDummy : public ViewportDistorter {
private:
  friend class ViewportDistorter;
  string getType(void) const {return "none";}
  void init(const InitParser &conf) {}
  void distort(void) const {}
  bool distortXY(int &x,int &y) {return true;}
};


class ViewportDistorterFisheyeToSphericMirror : public ViewportDistorter {
private:
  friend class ViewportDistorter;
  ViewportDistorterFisheyeToSphericMirror(int screenW,int screenH, Projector* prj);
  ~ViewportDistorterFisheyeToSphericMirror(void);
  string getType(void) const {return "fisheye_to_spheric_mirror";}
  void init(const InitParser &conf);
  void distort(void) const;
  bool distortXY(int &x,int &y);
  const int screenW;
  const int screenH;
  unsigned int mirror_texture;
  int viewport_wh;
  float texture_used;
  struct VertexData {
    float color[4];
    float xy[2];
    double h;
  };
  VertexData *trans_array;
  int trans_width,trans_height;
  SphericMirrorCalculator calc;
};


ViewportDistorterFisheyeToSphericMirror
   ::ViewportDistorterFisheyeToSphericMirror(int screenW,int screenH, Projector* prj)
    :screenW(screenW),screenH(screenH),trans_array(0) {

  if (prj->getProjectionType() == "fisheye") {
    prj->setMaxFov(175.0);
  } else {
    cerr << "ViewportDistorterFisheyeToSphericMirror: "
         << "what are you doing? the projection type should be fisheye."
         << endl;
  }
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
}

void ViewportDistorterFisheyeToSphericMirror::init(const InitParser &conf) {
  calc.init(conf);
  const double gamma = conf.get_double("spheric_mirror","projector_gamma",0.45);
    // init transformation
  trans_width = screenW / 16;
  trans_height = screenH / 16;
  trans_array = new VertexData[(trans_width+1)*(trans_height+1)];
  double max_h = 0;
  for (int j=0;j<=trans_height;j++) {
    for (int i=0;i<=trans_width;i++) {
      VertexData &data(trans_array[(j*(trans_width+1)+i)]);
      Vec3d v,v_x,v_y;
      bool rc = calc.retransform(
                        (i-(trans_width>>1))/(double)trans_height,
                        (j-(trans_height>>1))/(double)trans_height,
                        v,v_x,v_y);
      data.h = rc ? (v_x^v_y).length() : 0.0;
      v[0] = -v[0];
      const double h = v[1];
      v[1] = v[2];
      v[2] = -h;
      const double oneoverh = 1./sqrt(v[0]*v[0]+v[1]*v[1]);
      const double a = 0.5 + atan(v[2]*oneoverh)/M_PI; // range: [0..1]
      double f = a* 180.0/175.0; // MAX_FOV=175.0 for fisheye
      f *= oneoverh;
      double x = (0.5 + v[0] * f);
      double y = (0.5 + v[1] * f);
      if (x < 0.0) {x=0.0;data.h=0;} else if (x > 1.0) {x=1.0;data.h=0;}
      if (y < 0.0) {y=0.0;data.h=0;} else if (y > 1.0) {y=1.0;data.h=0;}
      data.xy[0] = x*texture_used;
      data.xy[1] = y*texture_used;
      if (data.h > max_h) max_h = data.h;
    }
  }
  for (int j=0;j<=trans_height;j++) {
    for (int i=0;i<=trans_width;i++) {
      VertexData &data(trans_array[(j*(trans_width+1)+i)]);
      data.color[0] = data.color[1] = data.color[2] =
        (data.h<=0.0) ? 0.0 : exp(gamma*log(data.h/max_h));
      data.color[3] = 1.0f;
    }
  }
}

ViewportDistorterFisheyeToSphericMirror
    ::~ViewportDistorterFisheyeToSphericMirror(void) {
  if (trans_array) delete[] trans_array;
  glDeleteTextures(1,&mirror_texture);
}

bool ViewportDistorterFisheyeToSphericMirror::distortXY(int &x,int &y) {
    // linear interpolation:
  y = screenH-1-y;
  const float dx = (x&15)/16.0f;
  const int i = x >> 4;
  const float dy = (y&15)/16.0f;
  const int j = y >> 4;
  const float f00 = (1.0f-dx)*(1.0f-dy);
  const float f01 = (     dx)*(1.0f-dy);
  const float f10 = (1.0f-dx)*(     dy);
  const float f11 = (     dx)*(     dy);
  const VertexData *const v = trans_array + (j*(trans_width+1)+i);
  x = ((screenW-viewport_wh)>>1)
    + (int)std::floor(0.5f +
                        (v[0].xy[0]*f00
                       + v[1].xy[0]*f01
                       + v[trans_width+1].xy[0]*f10
                       + v[trans_width+2].xy[0]*f11)*viewport_wh/texture_used);
  y = ((screenH-viewport_wh)>>1)
    + (int)std::floor(0.5f +
                        ((v[0].xy[1]*f00
                       + v[1].xy[1]*f01
                       + v[trans_width+1].xy[1]*f10
                       + v[trans_width+2].xy[1]*f11)*viewport_wh/texture_used));
  y = screenH-1-y;
  return true;
}

void ViewportDistorterFisheyeToSphericMirror::distort(void) const {
  glViewport(0, 0, screenW, screenH);
  glMatrixMode(GL_PROJECTION);		// projection matrix mode
  glPushMatrix();						// store previous matrix
  glLoadIdentity();
  gluOrtho2D(0,screenW,0,screenH);			// set a 2D orthographic projection
  glMatrixMode(GL_MODELVIEW);			// modelview matrix mode
  glPushMatrix();
  glLoadIdentity();	


  glBindTexture(GL_TEXTURE_2D, mirror_texture);
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                      (screenW-viewport_wh)>>1,(screenH-viewport_wh)>>1,
                      viewport_wh,viewport_wh);

  glEnable(GL_TEXTURE_2D);

  float color[4] = {1,1,1,1};
  glColor4fv(color);
  glDisable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, mirror_texture);

/*
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0); glVertex3f(screenW,0, 0.0);
  glTexCoord2f(texture_used, 0.0); glVertex3f(0,0, 0.0);
  glTexCoord2f(texture_used, texture_used); glVertex3f(0, screenH, 0.0);
  glTexCoord2f(0.0, texture_used); glVertex3f(screenW, screenH, 0.0);
  glEnd();
*/


  for (int j=0;j<trans_height;j++) {
    glBegin(GL_QUAD_STRIP);
    const VertexData *v0 = trans_array + j*(trans_width+1);
    const VertexData *v1 = v0 + (trans_width+1);
    for (int i=0;i<=trans_width;i++) {
      glColor4fv(v0[i].color);
      glTexCoord2fv(v0[i].xy);
      glVertex3f(i*16, j*16, 0.0);
      glColor4fv(v1[i].color);
      glTexCoord2fv(v1[i].xy);
      glVertex3f(i*16, (j+1)*16, 0.0);
    }
    glEnd();
  }


  glMatrixMode(GL_PROJECTION);		// Restore previous matrix
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();	
}

ViewportDistorter *ViewportDistorter::create(const string &type,
                                             int width,int height, Projector* prj) {
  if (type == "fisheye_to_spheric_mirror") {
    return new ViewportDistorterFisheyeToSphericMirror(width,height, prj);
  }
  return new ViewportDistorterDummy;
}

