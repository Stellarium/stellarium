/*
 * Stellarium
 * This file Copyright (C) 2004 Robert Spearman
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

#ifndef _METEOR_H_
#define _METEOR_H_

#include "projector.h"
#include "navigator.h"
#include "tone_reproductor.h"

// all in km - altitudes make up meteor range
#define EARTH_RADIUS 6369.f
#define HIGH_ALTITUDE 115.f
#define LOW_ALTITUDE 70.f
#define VISIBLE_RADIUS 457.8f

class Meteor
{

 public:
  Meteor( Projector *proj, Navigator *nav, tone_reproductor* eye, double v);
  virtual ~Meteor();
  bool update(int delta_time);  // update position
  bool draw(Projector *proj, Navigator* nav);		// Draw the meteor
  bool is_alive(void);          // see if burned out yet

 private:
  Mat4d mmat; // tranformation matrix to align radiant with earth direction of travel
  Vec3d obs;  // observer position in meteor coord. system
  Vec3d position;  // equatorial coordinate position
  Vec3d pos_internal;  // middle of train
  Vec3d pos_train;  // end of train
  bool train;      // point or train visible?
  double start_h;  // start height above center of earth
  double end_h;    // end height
  double velocity; // km/s
  bool alive;      // is it still visible?
  float mag;	   // Apparent magnitude at head, 0-1
  float max_mag;  // 0-1
  float abs_mag;  // absolute magnitude
  float vis_mag;  // visual magnitude at observer
  double xydistance; // distance in XY plane (orthogonal to meteor path) from observer to meteor
  double init_dist;  // initial distance from observer
  double min_dist;  // nearest point to observer along path
  double dist_multiplier;  // scale magnitude due to changes in distance 
};


#endif // _METEOR_H
