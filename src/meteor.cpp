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

// This is an ad hoc meteor model
// Could use a simple ablation physics model in the future

#include "meteor.h"
#include "stdlib.h"

Meteor::Meteor(Projector *proj, navigator* nav, tone_reproductor* eye, double v)
{
  //  velocity = 11+(double)rand()/((double)RAND_MAX+1)*v;  // abs range 11-72 km/s
  //TEMP
  velocity=v;

  mag = max_mag = 1;

  // select random trajectory (here radiant is on z axis, z=0 at center of earth)
  // using polar coordinates
  double d = (double)rand()/((double)RAND_MAX+1)*(EARTH_RADIUS+HIGH_ALTITUDE);
  double angle = (double)rand()/((double)RAND_MAX+1)*2*M_PI;

  pos_internal[0] = pos_train[0] = position[0] = d*cos(angle);
  pos_internal[1] = pos_train[1] = position[1] = d*sin(angle);

  // determine life of meteor (h on z axis)
  start_h = sqrt( pow(EARTH_RADIUS+HIGH_ALTITUDE,2) - d*d);

  if( d > EARTH_RADIUS+LOW_ALTITUDE ) {
    end_h = -start_h;
  } else {
    end_h = sqrt( pow(EARTH_RADIUS+LOW_ALTITUDE,2) - d*d);
  }

  /* experiment
  // limit lifetime to 0.5-3.0 sec
  double tmp_h = start_h - velocity * (0.5 + (double)rand()/((double)RAND_MAX+1) * 2.5);
  if( tmp_h > end_h ) {
    end_h = tmp_h;
  }
  */

  pos_train[2] = position[2] = start_h ;

  //  printf("New meteor: %f %f s:%f e:%f v:%f\n", position[0], position[1], start_h, end_h, velocity);

  alive = 1;

  // determine meteor model view matrix (want z in dir of travel of earth)
  // meteor life is so short, no need to recalculate
  double equ_rotation; // rotation needed to align with path of earth
  Vec3d sun_dir = nav->helio_to_earth_equ( Vec3d(0,0,0) );

  Mat4d tmat = Mat4d::xrotation(-23.45f*M_PI/180.f);  // ecliptical tilt
  sun_dir.transfo4d(tmat);  // convert to ecliptical coordinates
  sun_dir.normalize();
  equ_rotation = acos( sun_dir.dot( Vec3d(1,0,0) ) );
  if( sun_dir[1] < 0 ) equ_rotation = 2*M_PI - equ_rotation;

  equ_rotation -= M_PI_2;


  train=0;

  mmat = Mat4d::xrotation(23.45f*M_PI/180.f) * Mat4d::zrotation(equ_rotation) * Mat4d::yrotation(M_PI_2);


  // Determine drawing color given magnitude and eye 
  // (won't be visible during daylight)
  Vec3d RGB = Vec3d(1,1,1);  // *** color varies somewhat depending on velocity
  //float MaxColorValue = MY_MAX(RGB[0],RGB[2]);

  float Mag = (double)rand()/((double)RAND_MAX+1)*6.75f - 3;  // meteor visual magnitude
  mag = (5. + Mag) / 256.0;
  if (mag>250) mag = mag - 256;

  float term1 = expf(-0.92103f*(mag + 12.12331f)) * 108064.73f;

  float cmag=1.f;
  float rmag;

  // Compute the equivalent star luminance for a 5 arc min circle and convert it
  // in function of the eye adaptation
  rmag = eye->adapt_luminance(term1);
  rmag = rmag/powf(proj->get_fov(),0.85f)*50.f;

  // if size of star is too small (blink) we put its size to 1.2 --> no more blink
  // And we compensate the difference of brighteness with cmag
  if (rmag<1.2f) {
    cmag=rmag*rmag/1.44f;
    //if (rmag/star_scale<0.1f || cmag<0.1/star_mag_scale) return;
    //    if (rmag<0.1f || cmag<0.1) alive=0;  // not visible...
  }

  // Global scaling
  //  cmag*=star_mag_scale;

  mag = cmag;  // assumes gray only

}

Meteor::~Meteor()
{   
}

// returns true if alive
bool Meteor::update(int delta_time)
{

  if(!alive) return(0);

  if( position[2] < end_h ) {
    // burning has stopped so magnitude fades out
    // here assume linear fade out

    mag -= max_mag * (double)delta_time/500.0f;
    if( mag < 0 ) alive=0;  // no longer visible
  }

  // would need time direction multiplier to allow reverse time replay
  position[2] = position[2] - velocity/1000.0f*(double)delta_time;


  // train doesn't extend beyond start of burn
  if( position[2] + velocity*0.5f > start_h ) {
    pos_train[2] = start_h ;
  } else {
    pos_train[2] -= velocity*(double)delta_time/1000.0f;
  }

  //printf("meteor position: %f delta_t %d\n", position[2], delta_time);

  return(alive);
}


// returns true if visible
bool Meteor::draw(Projector *proj, navigator* nav)
{

  if(!alive) return(alive);

  Vec3d start, end;

  Vec3d spos = position;
  Vec3d epos = pos_train;

  // convert to equ
  spos.transfo4d(mmat);
  epos.transfo4d(mmat);

  // convert to local and correct for earth radius [since equ and local coordinates in stellarium use same 0 point!] 
  spos = nav->earth_equ_to_local( spos );
  epos = nav->earth_equ_to_local( epos );
  spos[2] -= EARTH_RADIUS;
  epos[2] -= EARTH_RADIUS;

  int t1 = proj->project_local_check(spos/1216, start);  // 1216 is to scale down under 1 for desktop version
  int t2 = proj->project_local_check(epos/1216, end);

  // don't draw if not visible (but may come into view)
  if( t1 + t2 == 0 ) return 1;

  //  printf("[%f %f %f] (%d, %d) (%d, %d)\n", position[0], position[1], position[2], (int)start[0], (int)start[1], (int)end[0], (int)end[1]);

  glEnable(GL_BLEND); 
  glDisable(GL_TEXTURE_2D);  // much dimmer without this

  if( train ) {
    // connect this point with last drawn point

    // compute an intermediate point so can curve slightly along projection distortions
    Vec3d intpos;
    Vec3d posi = pos_internal; 
    posi[2] = position[2] + (pos_train[2] - position[2])/2;
    posi.transfo4d(mmat);
    posi = nav->earth_equ_to_local( posi );
    posi[2] -= EARTH_RADIUS;
    proj->project_local(posi/1216, intpos);

    // draw dark to light
    glBegin(GL_LINE_STRIP);
    glColor3f(0,0,0);
    glVertex3f(end[0],end[1],0);
    glColor3f(mag/2,mag/2,mag/2);
    glVertex3f(intpos[0],intpos[1],0);
    glColor3f(mag,mag,mag);
    glVertex3f(start[0],start[1],0);
    glEnd();
  } else {
    glPointSize(1);
    glBegin(GL_POINTS);
    glVertex3f(start[0],start[1],0);
    glEnd();
  }

  /*  
  // TEMP - show radiant
  Vec3d radiant = Vec3d(0,0,0.5f);
  radiant.transfo4d(mmat);
  if( projection->project_earth_equ(radiant, start) ) {
    glColor3f(1,0,1);
    glBegin(GL_LINES);
    glVertex3f(start[0]-10,start[1],0);
    glVertex3f(start[0]+10,start[1],0);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(start[0],start[1]-10,0);
    glVertex3f(start[0],start[1]+10,0);
    glEnd();
  }
  */

  glEnable(GL_TEXTURE_2D);

  train = 1;

  return(1);
}

bool Meteor::is_alive(void)
{
  return(alive);
}
