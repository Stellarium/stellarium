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

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <string>
#include "s_texture.h"

class Image
{

 public:
  Image(string filename, string name);
  virtual ~Image();
  //  int drop(string image_name);
  void set_alpha(float alpha, float duration);
  void set_scale(float scale, float duration);
  void set_rotation(float rotation, float duration);
  bool update(int delta_time);  // update properties
  void draw(int screenw, int screenh);
  string get_name() { return image_name; };

 private:
  s_texture * image_tex;
  string image_name;
  float image_x, image_y, image_scale, image_alpha, image_rotation;
  float image_ratio;

  bool flag_alpha, flag_scale, flag_rotation;
  float coef_alpha, coef_scale, coef_rotation;
  float mult_alpha, mult_scale, mult_rotation;
  float start_alpha, start_scale, start_rotation;
  float end_alpha, end_scale, end_rotation;

};


#endif // _IMAGE_H
