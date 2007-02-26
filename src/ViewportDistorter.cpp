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

#include "GLee.h"

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
  void prepare(void) const {}
  void distort(void) const {}
  bool distortXY(int &x,int &y) const {return true;}
};


class ViewportDistorterFisheyeToSphericMirror : public ViewportDistorter {
private:
  friend class ViewportDistorter;
  ViewportDistorterFisheyeToSphericMirror(int screen_w,int screen_h,
                                          Projector *prj,
                                          const InitParser &conf);
  ~ViewportDistorterFisheyeToSphericMirror(void);
  string getType(void) const {return "fisheye_to_spheric_mirror";}
  void cleanup(void);
  void prepare(void) const;
  void distort(void) const;
  bool distortXY(int &x,int &y) const;
private:
  bool flag_use_ext_framebuffer_object;
  Projector *const prj;
  const int screen_w;
  const int screen_h;
  const double original_max_fov;
  const Vec4i original_viewport;
  const Vec2d original_viewport_center;
  const double original_viewport_fov_diameter;



  int viewport[2],viewport_texture_offset[2];
  int viewport_w,viewport_h;
  float viewport_center[2];
  float viewport_fov_diameter;

  int texture_wh;

  struct TexturePoint {
    float tex_xy[2];
  };
  TexturePoint *texture_point_array;
  int max_x,max_y;
  double step_x,step_y;

  GLuint display_list;
  GLuint mirror_texture;
  GLuint fbo;                    // frame buffer object
  GLuint depth_buffer;            // depth render buffer
};


struct VertexPoint {
  float ver_xy[2];
  float color[4];
  double h;
};



ViewportDistorterFisheyeToSphericMirror
   ::ViewportDistorterFisheyeToSphericMirror(int screen_w,int screen_h,
                                             Projector *prj,
                                             const InitParser &conf)
    :prj(prj),screen_w(screen_w),screen_h(screen_h),
     original_max_fov(prj->getMapping().maxFov),
     original_viewport(prj->getViewport()),
     original_viewport_center(prj->getViewportCenter()),
     original_viewport_fov_diameter(prj->getViewportFovDiameter()),
     texture_point_array(0) {
  SphericMirrorCalculator calc;
  calc.init(conf);

  double distorter_max_fov
    = conf.get_double("spheric_mirror","distorter_max_fov",175.0);
  if (distorter_max_fov > 240.0) distorter_max_fov = 240.0;
  else if (distorter_max_fov < 120.0) distorter_max_fov = 120.0;
  if (distorter_max_fov > prj->getMapping().maxFov)
    distorter_max_fov = prj->getMapping().maxFov;

  flag_use_ext_framebuffer_object = GLEE_EXT_framebuffer_object;
  if (flag_use_ext_framebuffer_object) {
    flag_use_ext_framebuffer_object
      = conf.get_boolean("spheric_mirror",
                         "flag_use_ext_framebuffer_object",true);
  }
  cout << "ViewportDistorterFisheyeToSphericMirror::"
          "ViewportDistorterFisheyeToSphericMirror: "
          "flag_use_ext_framebuffer_object: "
       << flag_use_ext_framebuffer_object << endl;

  double texture_triangle_base_length
    = conf.get_double("spheric_mirror","texture_triangle_base_length",16.0);  
  if (texture_triangle_base_length > 256.0)
    texture_triangle_base_length = 256.0;
  else if (texture_triangle_base_length < 2.0)
    texture_triangle_base_length = 2.0;


    // initialize viewport parameters and texture size:
  viewport_w = conf.get_int("spheric_mirror","viewport_width",
                            original_viewport[2]);
  if (viewport_w <= 0) {
    viewport_w = original_viewport[2];
  } else if (!flag_use_ext_framebuffer_object && viewport_w > screen_w) {
    viewport_w = screen_w;
  }
  viewport_h = conf.get_int("spheric_mirror","viewport_height",
                            original_viewport[3]);
  if (viewport_h <= 0) {
    viewport_h = original_viewport[3];
  } else if (!flag_use_ext_framebuffer_object && viewport_h > screen_h) {
    viewport_h = screen_h;
  }
  viewport_center[0] = conf.get_double("spheric_mirror","viewport_center_x",
                                       0.5*viewport_w);
  viewport_center[1] = conf.get_double("spheric_mirror","viewport_center_y",
                                       0.5*viewport_h);
  viewport_fov_diameter
	= conf.get_double("spheric_mirror","viewport_fov_diameter",
                      MY_MIN(viewport_w,viewport_h));

  texture_wh = 1;
  while (texture_wh < viewport_w || texture_wh < viewport_h)
    texture_wh <<= 1;
  viewport_texture_offset[0] =  (texture_wh-viewport_w)>>1;
  viewport_texture_offset[1] =  (texture_wh-viewport_h)>>1;

  if (flag_use_ext_framebuffer_object) {
    viewport[0] = viewport_texture_offset[0];
    viewport[1] = viewport_texture_offset[1];
  } else {
    viewport[0] = (screen_w-viewport_w) >> 1;
    viewport[1] = (screen_h-viewport_h) >> 1;
  }
//cout << "texture_wh: " << texture_wh << endl;
//cout << "viewport_fov_diameter: " << viewport_fov_diameter << endl;
//cout << "screen: " << screen_w << ", " << screen_h << endl;
//cout << "viewport: " << viewport[0] << ", " << viewport[1] << ", "
//     << viewport_w << ", " << viewport_h << endl;
//cout << "viewport_texture_offset: "
//     << viewport_texture_offset[0] << ", "
//     << viewport_texture_offset[1] << endl;

  prj->setViewport(viewport[0],viewport[1],
                   viewport_w,viewport_h,
                   viewport_center[0],viewport_center[1],
                   viewport_fov_diameter);



    // initialize mirror_texture:
  glGenTextures(1, &mirror_texture);
  glBindTexture(GL_TEXTURE_2D, mirror_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP /*_TO_EDGE*/);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP /*_TO_EDGE*/);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  if (flag_use_ext_framebuffer_object) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 texture_wh,texture_wh, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
      // Create the render buffer for depth    
    glGenRenderbuffersEXT(1, &depth_buffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_buffer);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                             texture_wh,texture_wh);
      // Setup our FBO
    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

      // Attach the texture to the FBO so we can render to it
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, mirror_texture, 0);
      // Attach the depth render buffer to the FBO as it's depth attachment
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, depth_buffer);
    const GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
        cout << "could not initialize GL_FRAMEBUFFER_EXT" << endl;
        assert(0);
    }
      // clear the texture
    glClear(GL_COLOR_BUFFER_BIT);
      // Unbind the FBO
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  } else {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    const int size = 3*texture_wh*texture_wh;
    unsigned char *pixel_data = new unsigned char[size];
    assert(pixel_data);
    memset(pixel_data,0,size);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 texture_wh, texture_wh, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 pixel_data);
    delete pixel_data;
  }

  prj->setMaxFov(distorter_max_fov);
  
    // init transformation
  double gamma
    = conf.get_double("spheric_mirror","projector_gamma",0.45);
  if (gamma < 0.0) gamma = 0.0;
  const float view_scaling_factor
    = viewport_fov_diameter
    * prj->getMapping().fovToViewScalingFactor(distorter_max_fov);
  max_x = (int)trunc(0.5 + screen_w/texture_triangle_base_length);
  step_x = screen_w / (double)(max_x-0.5);
  max_y = (int)trunc(screen_h/(texture_triangle_base_length*0.5*sqrt(3.0)));
  step_y = screen_h/ (double)max_y;
//cout << "max_x: " << max_x << ", max_y: " << max_y
//     << ", step_x: " << step_x << ", step_y: " << step_y << endl;
  texture_point_array = new TexturePoint[(max_x+1)*(max_y+1)];
  VertexPoint *vertex_point_array = new VertexPoint[(max_x+1)*(max_x+1)];
  double max_h = 0;
  for (int j=0;j<=max_y;j++) {
    for (int i=0;i<=max_x;i++) {
      VertexPoint &vertex_point(vertex_point_array[(j*(max_x+1)+i)]);
      TexturePoint &texture_point(texture_point_array[(j*(max_x+1)+i)]);
      vertex_point.ver_xy[0] = ((i == 0) ? 0.f :
				              (i == max_x) ? screen_w :
				              (i-0.5f*(j&1))*step_x);
      vertex_point.ver_xy[1] = j*step_y;
      Vec3d v,v_x,v_y;
      bool rc = calc.retransform(
                       (vertex_point.ver_xy[0]-0.5f*screen_w) / screen_h,
                       (vertex_point.ver_xy[1]-0.5f*screen_h) / screen_h,
                       v,v_x,v_y);
      double h = v[1];
      v[1] = v[2];
      v[2] = -h;

      rc &= prj->getMapping().forward(v);
      const float x = viewport_center[0] + v[0] * view_scaling_factor;
      const float y = viewport_center[1] + v[1] * view_scaling_factor;
      vertex_point.h = rc ? (v_x^v_y).length() : 0.0;

        // sharp image up to the border of the fisheye image, an the cost of
        // accepting clamping artefacts. You can get rid of the clamping
        // artefacts by specifying a viewport size a little less then
        // (1<<n)*(1<<n), for instance 1022*1022. With a viewport size
        // of 512*512 and viewport_fov_diameter=512 you will get clamping
        // artefacts in the 3 otherwise black hills on the bottom of the image.

//      if (x < 0.f) {x=0.f;vertex_point.h=0;}
//      else if (x > viewport_w) {x=viewport_w;vertex_point.h=0;}
//      if (y < 0.f) {y=0.f;vertex_point.h=0;}
//      else if (y > viewport_h) {y=viewport_h;vertex_point.h=0;}

      texture_point.tex_xy[0] = (viewport_texture_offset[0]+x)/texture_wh;
      texture_point.tex_xy[1] = (viewport_texture_offset[1]+y)/texture_wh;

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

    // initialize the display list
  display_list = glGenLists(1);
  glNewList(display_list,GL_COMPILE);
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
    glBegin(GL_TRIANGLE_STRIP);
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
  glEndList();
  delete[] vertex_point_array;
}



ViewportDistorterFisheyeToSphericMirror::
    ~ViewportDistorterFisheyeToSphericMirror(void) {
  if (texture_point_array) delete[] texture_point_array;
  glDeleteLists(display_list,1);
  if (flag_use_ext_framebuffer_object) {
    glDeleteFramebuffersEXT(1, &fbo);
    glDeleteRenderbuffersEXT(1, &depth_buffer);
  }
  glDeleteTextures(1,&mirror_texture);
  prj->setMaxFov(original_max_fov);
  prj->setViewport(original_viewport[0],original_viewport[1],
                   original_viewport[2],original_viewport[3],
                   original_viewport_center[0],original_viewport_center[1],
                   original_viewport_fov_diameter);
}




bool ViewportDistorterFisheyeToSphericMirror::distortXY(int &x,int &y) const {
  float texture_x,texture_y;

    // find the triangle and interpolate accordingly:
  float dy = y / step_y;
  const int j = (int)floorf(dy);
  dy -= j;
  if (j&1) {
    float dx = x / step_x + 0.5f*(1.f-dy);
    const int i = (int)floorf(dx);
    dx -= i;
    const TexturePoint *const t = texture_point_array + (j*(max_x+1)+i);
    if (dx + dy <= 1.f) {
      if (i == 0) {
        dx -= 0.5f*(1.f-dy);
        dx *= 2.f;
      }
      texture_x = t[0].tex_xy[0]
                + dx * (t[1].tex_xy[0]-t[0].tex_xy[0])
                + dy * (t[max_x+1].tex_xy[0]-t[0].tex_xy[0]);
      texture_y = t[0].tex_xy[1]
                + dx * (t[1].tex_xy[1]-t[0].tex_xy[1])
                + dy * (t[max_x+1].tex_xy[1]-t[0].tex_xy[1]);
    } else {
      if (i == max_x-1) {
        dx -= 0.5f*(1.f-dy);
        dx *= 2.f;
      }
      texture_x = t[max_x+2].tex_xy[0]
                + (1.f-dy) * (t[1].tex_xy[0]-t[max_x+2].tex_xy[0])
                + (1.f-dx) * (t[max_x+1].tex_xy[0]-t[max_x+2].tex_xy[0]);
      texture_y = t[max_x+2].tex_xy[1]
                + (1.f-dy) * (t[1].tex_xy[1]-t[max_x+2].tex_xy[1])
                + (1.f-dx) * (t[max_x+1].tex_xy[1]-t[max_x+2].tex_xy[1]);
    }
  } else {
    float dx = x / step_x + 0.5f*dy;
    const int i = (int)floorf(dx);
    dx -= i;
    const TexturePoint *const t = texture_point_array + (j*(max_x+1)+i);
    if (dx >= dy) {
      if (i == max_x-1) {
        dx -= 0.5f*dy;
        dx *= 2.f;
      }
      texture_x = t[1].tex_xy[0]
                + (1.f-dx) * (t[0].tex_xy[0]-t[1].tex_xy[0])
                + dy * (t[max_x+2].tex_xy[0]-t[1].tex_xy[0]);
      texture_y = t[1].tex_xy[1]
                + (1.f-dx) * (t[0].tex_xy[1]-t[1].tex_xy[1])
                + dy * (t[max_x+2].tex_xy[1]-t[1].tex_xy[1]);
    } else {
      if (i == 0) {
        dx -= 0.5f*dy;
        dx *= 2.f;
      }
      texture_x = t[max_x+1].tex_xy[0]
                + (1.f-dy) * (t[0].tex_xy[0]-t[max_x+1].tex_xy[0])
                + dx * (t[max_x+2].tex_xy[0]-t[max_x+1].tex_xy[0]);
      texture_y = t[max_x+1].tex_xy[1]
                + (1.f-dy) * (t[0].tex_xy[1]-t[max_x+1].tex_xy[1])
                + dx * (t[max_x+2].tex_xy[1]-t[max_x+1].tex_xy[1]);
    }
  }
  
  
  x = (int)floorf(0.5+texture_wh*texture_x)
    - viewport_texture_offset[0] + viewport[0];
  y = (int)floorf(0.5+texture_wh*texture_y)
    - viewport_texture_offset[1] + viewport[1];
//  y = screen_h-1-y;
  return true;
}

void ViewportDistorterFisheyeToSphericMirror::prepare(void) const {
  // First we bind the FBO so we can render to it
  if (flag_use_ext_framebuffer_object) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
  }
}

void ViewportDistorterFisheyeToSphericMirror::distort(void) const {
  // set rendering back to default frame buffer
  if (flag_use_ext_framebuffer_object) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  }
  glViewport(0, 0, screen_w, screen_h);
  glMatrixMode(GL_PROJECTION);        // projection matrix mode
  glPushMatrix();                     // store previous matrix
  glLoadIdentity();
  gluOrtho2D(0,screen_w,0,screen_h);    // set a 2D orthographic projection
  glMatrixMode(GL_MODELVIEW);         // modelview matrix mode
  glPushMatrix();
  glLoadIdentity();


  glBindTexture(GL_TEXTURE_2D, mirror_texture);
  if (!flag_use_ext_framebuffer_object) {
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                        viewport_texture_offset[0],
                        viewport_texture_offset[1],
                        viewport[0],
                        viewport[1],
                        viewport_w,viewport_h);
  }
  glEnable(GL_TEXTURE_2D);

  float color[4] = {1,1,1,1};
  glColor4fv(color);
  glDisable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, mirror_texture);

  glCallList(display_list);

//glBegin(GL_QUADS);
//glTexCoord2f(0.0f, 1.0f); glVertex2i(0,screen_h);
//glTexCoord2f(1.0f, 1.0f); glVertex2i(screen_w,screen_h);
//glTexCoord2f(1.0f, 0.0f); glVertex2i(screen_w,0);
//glTexCoord2f(0.0f, 0.0f); glVertex2i(0,0);
//glEnd();

  glMatrixMode(GL_PROJECTION);        // Restore previous matrix
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glViewport(viewport[0],viewport[1],viewport_w,viewport_h);
}


ViewportDistorter *ViewportDistorter::create(const string &type,
                                             int width,int height,
                                             Projector *prj,
                                             const InitParser &conf) {
  if (type == "fisheye_to_spheric_mirror") {
    return new ViewportDistorterFisheyeToSphericMirror(width,height,prj,conf);
  }
  return new ViewportDistorterDummy;
}

