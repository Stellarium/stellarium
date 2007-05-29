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

/*
NOTE: Here the radiant is always along the ecliptic at the apex of the Earth's way.
In reality, individual meteor streams have varying velocity vectors and therefore radiants
which are generally not at the apex of the Earth's way, such as the Perseids shower.
*/

// Improved realism and efficiency 2004-12

#include <cstdlib>
#include "Meteor.hpp"

Meteor::Meteor(Projector *proj, Navigator* nav, ToneReproducer* eye, double v)
{
  //  velocity = 11+(double)rand()/((double)RAND_MAX+1)*v;  // abs range 11-72 km/s
  velocity=v;

  max_mag = 1;

  // determine meteor model view matrix (want z in dir of travel of earth, z=0 at center of earth)
  // meteor life is so short, no need to recalculate
  double equ_rotation; // rotation needed to align with path of earth
  Vec3d sun_dir = nav->helio_to_earth_equ( Vec3d(0,0,0) );

  Mat4d tmat = Mat4d::xrotation(-23.45f*M_PI/180.f);  // ecliptical tilt
  sun_dir.transfo4d(tmat);  // convert to ecliptical coordinates
  sun_dir.normalize();
  equ_rotation = acos( sun_dir.dot( Vec3d(1,0,0) ) );
  if( sun_dir[1] < 0 ) equ_rotation = 2*M_PI - equ_rotation;

  equ_rotation -= M_PI_2;

  mmat = Mat4d::xrotation(23.45f*M_PI/180.f) * Mat4d::zrotation(equ_rotation) * Mat4d::yrotation(M_PI_2);


  // select random trajectory using polar coordinates in XY plane, centered on observer
  xydistance = (double)rand()/((double)RAND_MAX+1)*(VISIBLE_RADIUS);
  double angle = (double)rand()/((double)RAND_MAX+1)*2*M_PI;

  // find observer position in meteor coordinate system
  obs = nav->local_to_earth_equ(Vec3d(0,0,EARTH_RADIUS));
  obs.transfo4d(mmat.transpose());

  // set meteor start x,y
  pos_internal[0] = pos_train[0] = position[0] = xydistance*cos(angle) +obs[0];
  pos_internal[1] = pos_train[1] = position[1] = xydistance*sin(angle) +obs[1];

  // determine life of meteor (start and end z value based on atmosphere burn altitudes)

  // D is distance from center of earth
  double D = sqrt( position[0]*position[0] + position[1]*position[1] );

  if( D > EARTH_RADIUS+HIGH_ALTITUDE ) {
    // won't be visible
    alive = 0;
    return;
  }

  start_h = sqrt( pow(EARTH_RADIUS+HIGH_ALTITUDE,2) - D*D);

  // determine end of burn point, and nearest point to observer for distance mag calculation
  // mag should be max at nearest point still burning
  if( D > EARTH_RADIUS+LOW_ALTITUDE ) {
    end_h = -start_h;  // earth grazing
    min_dist = xydistance;
  } else {
    end_h = sqrt( pow(EARTH_RADIUS+LOW_ALTITUDE,2) - D*D);
    min_dist = sqrt( xydistance*xydistance + pow( end_h - obs[2], 2) );
  }

  if(min_dist > VISIBLE_RADIUS ) {
    // on average, not visible (although if were zoomed ...)
    alive = 0;
    return;
  }
    
  /* experiment
  // limit lifetime to 0.5-3.0 sec
  double tmp_h = start_h - velocity * (0.5 + (double)rand()/((double)RAND_MAX+1) * 2.5);
  if( tmp_h > end_h ) {
    end_h = tmp_h;
  }
  */

  pos_train[2] = position[2] = start_h;

  //  printf("New meteor: %f %f s:%f e:%f v:%f\n", position[0], position[1], start_h, end_h, velocity);

  alive = 1;
  train=0;

  // Determine drawing color given magnitude and eye 
  // (won't be visible during daylight)

  // *** color varies somewhat based on velocity, plus atmosphere reddening

  // determine intensity
  float Mag1 = (double)rand()/((double)RAND_MAX+1)*6.75f - 3;
  float Mag2 = (double)rand()/((double)RAND_MAX+1)*6.75f - 3;
  float Mag = (Mag1 + Mag2)/2.0f;

  mag = (5. + Mag) / 256.0;
  if (mag>250) mag = mag - 256;

  float term1 = std::exp(-0.92103f*(mag + 12.12331f)) * 108064.73f;

  float cmag=1.f;
  float rmag;

  // Compute the equivalent star luminance for a 5 arc min circle and convert it
  // in function of the eye adaptation
  rmag = eye->adapt_luminance(term1);
  rmag = rmag/powf(proj->getFov(),0.85f)*50.f;

  // if size of star is too small (blink) we put its size to 1.2 --> no more blink
  // And we compensate the difference of brighteness with cmag
  if (rmag<1.2f) {
    cmag=rmag*rmag/1.44f;
  }

  mag = cmag;  // assumes white

  // most visible meteors are under about 180km distant
  // scale max mag down if outside this range 
  float scale = 1;
  if(min_dist!=0) scale = 180*180/(min_dist*min_dist);
  if( scale < 1 ) mag *= scale;

}

Meteor::~Meteor()
{   
}

// returns true if alive
bool Meteor::update(double delta_time)
{
  if(!alive) return(0);

  if( position[2] < end_h ) {
    // burning has stopped so magnitude fades out
    // assume linear fade out

    mag -= max_mag * delta_time/500.0f;
    if( mag < 0 ) alive=0;  // no longer visible

  }

  // *** would need time direction multiplier to allow reverse time replay
  position[2] = position[2] - velocity/1000.0f*delta_time;

  // train doesn't extend beyond start of burn
  if( position[2] + velocity*0.5f > start_h ) {
    pos_train[2] = start_h ;
  } else {
    pos_train[2] -= velocity*delta_time/1000.0f;
  }

  //printf("meteor position: %f delta_t %d\n", position[2], delta_time);

  // determine visual magnitude based on distance to observer
  double dist = sqrt( xydistance*xydistance + pow( position[2]-obs[2], 2) );

  if( dist == 0 ) dist = .01;  // just to be cautious (meteor hits observer!)

  dist_multiplier = min_dist*min_dist / (dist*dist);

  return(alive);
}


// returns true if visible
// Assumes that we are in local frame
bool Meteor::draw(Projector *proj, const Navigator* nav)
{
	if(!alive) return(0);

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

	int t1 = proj->projectCheck(spos/1216, start);  // 1216 is to scale down under 1 for desktop version
	int t2 = proj->projectCheck(epos/1216, end);

	// don't draw if not visible (but may come into view)
	if( t1 + t2 == 0 ) return 1;

	//  printf("[%f %f %f] (%d, %d) (%d, %d)\n", position[0], position[1], position[2], (int)start[0], (int)start[1], (int)end[0], (int)end[1]);

	if( train ) {
		// connect this point with last drawn point

		double tmag = mag*dist_multiplier;

		// compute an intermediate point so can curve slightly along projection distortions
		Vec3d intpos;
		Vec3d posi = pos_internal; 
		posi[2] = position[2] + (pos_train[2] - position[2])/2;
		posi.transfo4d(mmat);
		posi = nav->earth_equ_to_local( posi );
		posi[2] -= EARTH_RADIUS;
		proj->project(posi/1216, intpos);

		// draw dark to light
		glBegin(GL_LINE_STRIP);
		glColor4f(0,0,0,0);
		glVertex3f(end[0],end[1],0);
		glColor4f(1,1,1,tmag/2);
		glVertex3f(intpos[0],intpos[1],0);
		glColor4f(1,1,1,tmag);
		glVertex3f(start[0],start[1],0);
		glEnd();
	} else {
		glPointSize(1); 
		proj->drawPoint2d(start[0],start[1]);
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

	train = 1;

	return(1);
}

bool Meteor::is_alive(void)
{
  return(alive);
}
