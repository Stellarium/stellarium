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

Image::Image( string filename, string name, IMAGE_POSITIONING pos_type) {
  flag_alpha = flag_scale = flag_location = flag_rotation = 0;
  image_pos_type = pos_type;
  image_alpha = 0;  // begin not visible
  image_rotation = 0;
  image_xpos = image_ypos = 0; // centered is default
  image_scale = 1;
  image_name = name;

  // load image using alpha channel in image, otherwise no transparency
  // other than through set_alpha method -- could allow alpha load option from command 

  image_tex = new s_texture(1, filename, TEX_LOAD_TYPE_PNG_ALPHA);  // what if it doesn't load?
  //  image_tex = new s_texture(1, filename, TEX_LOAD_TYPE_PNG_BLEND3);  // black = transparent

  int img_w, img_h;
  image_tex->getDimensions(img_w, img_h);

  //  cout << "script image: (" << filename << ") " << img_w << " " << img_h << endl;

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


void Image::set_location( float xpos, bool deltax, float ypos, bool deltay, float duration) {

	// xpos and ypos are interpreted when drawing based on image position type

	flag_location = 1;

	start_xpos = image_xpos;
	start_ypos = image_ypos;

	// only move if changing value
	if(deltax) end_xpos = xpos;
	else end_xpos = image_xpos;

	if(deltay) end_ypos = ypos;
	else end_ypos = image_ypos;

	coef_location = 1.0f/(1000.f*duration);
	mult_location = 0;

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


	  // this transition is parabolic for better visual results
	  if(start_scale > end_scale) {
		  image_scale = start_scale + (1 - (1-mult_scale)*(1-mult_scale))*(end_scale-start_scale);
	  } else image_scale = start_scale + mult_scale*mult_scale*(end_scale-start_scale);
  }

  if(flag_rotation) {
    mult_rotation += coef_rotation*delta_time;

    if( mult_rotation >= 1) {
      mult_rotation = 1;
      flag_rotation = 0;
    }

    image_rotation = start_rotation + mult_rotation*(end_rotation-start_rotation);
  }


  if(flag_location) {
    mult_location += coef_location*delta_time;

    if( mult_location >= 1) {
      mult_location = 1;
      flag_location = 0;
    }

    image_xpos = start_xpos + mult_location*(end_xpos-start_xpos);
    image_ypos = start_ypos + mult_location*(end_ypos-start_ypos);

  }


  return 1;

}

void Image::draw(int screenw, int screenh, const navigator * nav, Projector * prj) {

  if(image_ratio < 0 || image_alpha == 0) return;

  //  printf("draw image %s alpha %f\n", image_name.c_str(), image_alpha);

  int vieww = prj->viewW();
  int viewh = prj->viewH();  

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  glColor4f(1.0,1.0,1.0,image_alpha);

  glBindTexture(GL_TEXTURE_2D, image_tex->getID());
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  float cx = screenw/2.f;
  float cy = screenh/2.f;

  // calculations to keep image proportions when scale up to fit view
  float prj_ratio = (float)vieww/viewh;   

  float xbase, ybase;
  if(image_ratio > prj_ratio) {
    xbase = vieww/2;
    ybase = xbase/image_ratio;
  } else {
    ybase = viewh/2;
    xbase = ybase*image_ratio;
  }

  float w = image_scale*xbase;
  float h = image_scale*ybase;


  if(image_pos_type == POS_VIEWPORT ) {

	  // at x or y = 1, image is centered on projection edge
	  // centered in viewport at 0,0

	  prj->set_orthographic_projection();	// set 2D coordinate

	  glTranslatef(cx+image_xpos*vieww/2,cy+image_ypos*viewh/2,0);  // rotate around center of image...
	  glRotatef(image_rotation,0,0,-1);

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
	  
	  prj->reset_perspective_projection();

  } else if(image_pos_type == POS_HORIZONTAL ) {

	  // alt az coords
	  prj->reset_perspective_projection();

	  nav->switch_to_local();
	  Mat4d mat = nav->get_local_to_eye_mat();

	  Vec3d gridpt;
	  
	  //	  printf("%f %f\n", image_xpos, image_ypos);

	  // altitude = xpos, azimuth = ypos (0 at North), image top towards zenith when rotation = 0 
	  Vec3d imagev = Mat4d::zrotation(-1*(image_ypos-90)*M_PI/180.) 
		  * Mat4d::xrotation(image_xpos*M_PI/180.) * Vec3d(0,1,0); 

	  Vec3d ortho1 = Mat4d::zrotation((-1*(image_ypos-90))*M_PI/180.) * Vec3d(1,0,0);
	  Vec3d ortho2 = imagev^ortho1;

	  int grid_size = int(image_scale/5.);  // divisions per row, column
	  if(grid_size < 5) grid_size = 5;

	  for (int i=0; i<grid_size; i++) {

		  glBegin(GL_QUAD_STRIP);

		  for (int j=0; j<=grid_size; j++) {

			  for(int k=0; k<=1; k++) {

				  // TODO: separate x, y scales?
				  if(image_ratio<1) {
					  // image height is maximum angular dimension
					  gridpt = Mat4d::rotation( imagev, (image_rotation+180)*M_PI/180.) *
						  Mat4d::rotation( ortho1, image_scale*(j-grid_size/2.)/(float)grid_size*M_PI/180.) *
						  Mat4d::rotation( ortho2, image_scale/image_ratio*(i+k-grid_size/2.)/(float)grid_size*M_PI/180.) *
						  imagev;

				  } else {
					  // image width is maximum angular dimension
					  gridpt = Mat4d::rotation( imagev, (image_rotation+180)*M_PI/180.) *
						  Mat4d::rotation( ortho1, image_scale/image_ratio*(j-grid_size/2.)/(float)grid_size*M_PI/180.) *
						  Mat4d::rotation( ortho2, image_scale*(i+k-grid_size/2.)/(float)grid_size*M_PI/180.) *
						  imagev;
				  }
				  glTexCoord2f((i+k)/(float)grid_size,j/(float)grid_size);
				  prj->sVertex3( gridpt[0], gridpt[1], gridpt[2], mat);
			  }
		  }

		  glEnd();  
	  }
	 


  } else {
	  // earth equatorial positioning
	  printf("Earth equatorial script image positioning not implemented yet\n");

  }
  

}



