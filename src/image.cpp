/*
 * Stellarium
 * This file Copyright (C) 2005 Robert Spearman
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

// manage an image for display from scripts

#include <iostream>
#include "image.h"

Image::Image( string filename, string name) {
  flag_alpha = flag_scale = 0;
  image_alpha = 0;  // begin not visible
  image_rotation = 0;
  image_x = image_y = .5; // centered is default
  image_scale = 1; // full size
  image_name = name;

  // load image using alpha channel in image, otherwise no transparency
  // other than through set_alpha method -- could allow alpha load option from command 

  // TODO - needs to take path outside texture dir
  image_tex = new s_texture(filename, TEX_LOAD_TYPE_PNG_ALPHA);  // what if it doesn't load?

  int img_w, img_h;
  image_tex->getDimensions(img_w, img_h);

  cout << "script image: " << img_w << " " << img_h << endl;

  if(img_h == 0) image_ratio = -1;  // no image loaded
  else image_ratio = (float)img_w/img_h;
  
}

Image::~Image() {
  if(image_tex) delete image_tex;
}

void Image::set_alpha(float alpha, float duration) {

  flag_alpha = 1;

  start_alpha = image_alpha;
  end_alpha = alpha;

  coef_alpha = 1.0f/(1000.f*duration);
  mult_alpha = 0;

}
 

void Image::set_scale(float scale, float duration) {

  flag_scale = 1;

  start_scale = image_scale;
  end_scale = scale;

  coef_scale = 1.0f/(1000.f*duration);
  mult_scale = 0;

}

void Image::set_rotation(float rotation, float duration) {

  flag_rotation = 1;

  start_rotation = image_rotation;
  end_rotation = rotation;

  coef_rotation = 1.0f/(1000.f*duration);
  mult_rotation = 0;

}


bool Image::update(int delta_time) {

  if(image_ratio < 0) return 0;

  if(flag_alpha) {
    mult_alpha += coef_alpha*delta_time;

    if( mult_alpha >= 1) {
      mult_alpha = 1;
      flag_alpha = 0;
    }

    image_alpha = start_alpha + mult_alpha*(end_alpha-start_alpha);
  }

  if(flag_scale) {
    mult_scale += coef_scale*delta_time;

    if( mult_scale >= 1) {
      mult_scale = 1;
      flag_scale = 0;
    }

    image_scale = start_scale + mult_scale*(end_scale-start_scale);
  }

  if(flag_rotation) {
    mult_rotation += coef_rotation*delta_time;

    if( mult_rotation >= 1) {
      mult_rotation = 1;
      flag_rotation = 0;
    }

    image_rotation = start_rotation + mult_rotation*(end_rotation-start_rotation);
  }

  return 1;

}

void Image::draw(int screenw, int screenh) {

  if(image_ratio < 0) return;
  
  glPushMatrix();

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  glColor4f(1.0,1.0,1.0,image_alpha);

  glBindTexture(GL_TEXTURE_2D, image_tex->getID());

  float cx = screenw/2.f;
  float cy = screenh/2.f;

  // keep image dimensions to original proportions
  float prj_ratio = (float)screenw/screenh;

  float xbase, ybase;
  if(image_ratio > prj_ratio) {
    xbase = cx;
    ybase = xbase/image_ratio;
  } else {
    ybase = cy;
    xbase = ybase*image_ratio;
  }

  float w = image_scale*xbase;
  float h = image_scale*ybase;


  glTranslatef(cx,cy,0);  // rotate around center of image...
  glRotatef(image_rotation,0,0,1);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // why is black transparent?

  glBegin(GL_TRIANGLE_STRIP); {
    glTexCoord2i(1,0);              // Bottom Right
    glVertex3f(w,-h,0);
    glTexCoord2i(0,0);              // Bottom Left
    glVertex3f(-w,-h,0);
    glTexCoord2i(1,1);              // Top Right
    glVertex3f(w,h,0);
    glTexCoord2i(0,1);              // Top Left
    glVertex3f(-w,h,0); }
  glEnd();

  glPopMatrix();

}



