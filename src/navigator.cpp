/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chéreau
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

#include "navigator.h"
#include "stellarium.h"
#include "stel_utility.h"
#include "stel_object.h"
#include "stellastro.h"
#include "solarsystem.h"


////////////////////////////////////////////////////////////////////////////////
navigator::navigator(Observator* obs) : flag_traking(0), flag_lock_equ_pos(0), flag_auto_move(0),
						time_speed(JD_SECOND), JDay(0.), position(obs)
{
	if (!position)
	{
		printf("ERROR : Can't create a Navigator without a valid Observator\n");
		exit(-1);
	}
	local_vision=Vec3d(1.,0.,0.);
	equ_vision=Vec3d(1.,0.,0.);
	prec_equ_vision=Vec3d(1.,0.,0.);  // not correct yet...
	viewing_mode = VIEW_HORIZON;  // default

	precession = 0;

}

navigator::~navigator()
{
}

////////////////////////////////////////////////////////////////////////////////
void navigator::update_vision_vector(int delta_time, stel_object* selected)
{
  if (flag_auto_move)
    {
      double ra_aim, de_aim, ra_start, de_start, ra_now, de_now;

      if( zooming_mode == 1 ) {
	// if zooming in, object may be moving so be sure to zoom to latest position
	move.aim=selected->get_earth_equ_pos(this);
	move.aim.normalize();
	move.aim*=2.;
      }

      // Use a smooth function
      float smooth = 4.f;
      double c;

      if (zooming_mode == 1) {
	if( move.coef > .9 ) {
	  c = 1;
	} else {
	  c = 1 - pow(1.-1.11*(move.coef),3);
	}
      }
      else if(zooming_mode == -1) {
	if( move.coef < 0.1 ) { 
	  // keep in view at first as zoom out
	  c = 0;

	  /* could track as moves too, but would need to know if start was actually
	     a zoomed in view on the object or an extraneous zoom out command
	     if(move.local_pos) {
	     move.start=earth_equ_to_local(selected->get_earth_equ_pos(this));
	     } else {
	     move.start=selected->get_earth_equ_pos(this);
	     }
	     move.start.normalize();
	  */

	}else {
	  c =  pow(1.11*(move.coef-.1),3);		  
	}
      }
      else c = atanf(smooth * 2.*move.coef-smooth)/atanf(smooth)/2+0.5;


      if (move.local_pos)
	{
	  rect_to_sphe(&ra_aim, &de_aim, move.aim);
	  rect_to_sphe(&ra_start, &de_start, move.start);
	}
      else
	{
	  rect_to_sphe(&ra_aim, &de_aim, earth_equ_to_local(move.aim));
	  rect_to_sphe(&ra_start, &de_start, earth_equ_to_local(move.start));
	}

      /*  Was causing changes in direction while zooming, and seems unneccessary

      // Trick to choose the good moving direction and never travel on a distance > PI
      float delta = ra_start;
      ra_start -= delta;		// ra_start = 0
      ra_aim -= delta;

      if (ra_aim > M_PI) ra_aim = -2.*M_PI + ra_aim;
      if (ra_aim < -M_PI) ra_aim = 2.*M_PI + ra_aim;

      ra_now = ra_aim*c + ra_start*(1. - c);
      de_now = de_aim*c + de_start*(1. - c);

      ra_now += delta;
      */

      de_now = de_aim*c + de_start*(1. - c);
      ra_now = ra_aim*c + ra_start*(1. - c);

      sphe_to_rect(ra_now, de_now, local_vision);
      equ_vision = local_to_earth_equ(local_vision);

      move.coef+=move.speed*delta_time;
      if (move.coef>=1.)
	{
	  flag_auto_move=0;
	  if (move.local_pos)
	    {
	      local_vision=move.aim;
	      equ_vision=local_to_earth_equ(local_vision);
	    }
	  else
	    {
	      equ_vision=move.aim;
	      local_vision=earth_equ_to_local(equ_vision);
	    }
	}
    }
  else
    {
      if (flag_traking && selected) // Equatorial vision vector locked on selected object
	{
	  equ_vision=selected->get_earth_equ_pos(this);
	  // Recalc local vision vector
			
	  local_vision=earth_equ_to_local(equ_vision);
			
	}
      else
	{
	  if (flag_lock_equ_pos) // Equatorial vision vector locked
	    {
	      // Recalc local vision vector
	      local_vision=earth_equ_to_local(equ_vision);
	    }
	  else // Local vision vector locked
	    {
	      // Recalc equatorial vision vector
	      equ_vision=local_to_earth_equ(local_vision);
	    }
	}
    }
    
  prec_equ_vision = mat_earth_equ_to_prec_earth_equ*equ_vision;

    
}

////////////////////////////////////////////////////////////////////////////////
void navigator::set_local_vision(const Vec3d& _pos)
{
	local_vision = _pos;
	equ_vision=local_to_earth_equ(local_vision);
	prec_equ_vision = mat_earth_equ_to_prec_earth_equ*equ_vision;
}

////////////////////////////////////////////////////////////////////////////////
void navigator::update_move(double deltaAz, double deltaAlt)
{
	double azVision, altVision;

	if( viewing_mode == VIEW_EQUATOR) rect_to_sphe(&azVision,&altVision,equ_vision);
	else rect_to_sphe(&azVision,&altVision,local_vision);

    // if we are moving in the Azimuthal angle (left/right)
    if (deltaAz) azVision-=deltaAz;
    if (deltaAlt)
    {
		if (altVision+deltaAlt <= M_PI_2 && altVision+deltaAlt >= -M_PI_2) altVision+=deltaAlt;
		if (altVision+deltaAlt > M_PI_2) altVision = M_PI_2 - 0.000001;		// Prevent bug
		if (altVision+deltaAlt < -M_PI_2) altVision = -M_PI_2 + 0.000001;	// Prevent bug
    }

    // recalc all the position variables
	if (deltaAz || deltaAlt)
	{
    	if( viewing_mode == VIEW_EQUATOR)
		{
			sphe_to_rect(azVision, altVision, equ_vision);
			local_vision=earth_equ_to_local(equ_vision);
		}
		else
		{
			sphe_to_rect(azVision, altVision, local_vision);
			// Calc the equatorial coordinate of the direction of vision wich was in Altazimuthal coordinate
			equ_vision=local_to_earth_equ(local_vision);
			prec_equ_vision = mat_earth_equ_to_prec_earth_equ*equ_vision;
		}
	}

	// Update the final modelview matrices
	update_model_view_mat();

}

////////////////////////////////////////////////////////////////////////////////
// Increment time
void navigator::update_time(int delta_time)
{
	JDay+=time_speed*(double)delta_time/1000.;

	// Fix time limits to -100000 to +100000 to prevent bugs
	if (JDay>38245309.499988) JDay = 38245309.499988;
	if (JDay<-34803211.500012) JDay = -34803211.500012;

	// precession since epoch
	// using annual rate of 50.27 arcseconds
	precession = 0.000243716*(JDay-J2000)/365.25;

	//precession += 0.01;  // for quick testing
}

////////////////////////////////////////////////////////////////////////////////
// The non optimized (more clear version is available on the CVS : before date 25/07/2003)
void navigator::update_transform_matrices(Vec3d earth_ecliptic_pos)
{

        double lat = position->get_latitude();  
	// TODO: Figure out how to keep continuity in sky as reach poles  
	// otherwise sky jumps in rotation when reach poles

	// This is a kludge
	if( lat > 89 )  lat = 89.5;
	if( lat < -89 ) lat = -89.5;

	// axis of precession
	Vec3d axis = Vec3d(0,-0.3977702,0.9174851);

	mat_local_to_earth_equ = 
	  Mat4d::zrotation((get_apparent_sidereal_time(JDay)+position->get_longitude())*M_PI/180.) *
	  Mat4d::yrotation((90.-lat)*M_PI/180.);

	mat_earth_equ_to_local = mat_local_to_earth_equ.transpose();


	// for precession
	mat_earth_equ_to_prec_earth_equ = Mat4d::rotation(axis, precession);
	mat_prec_earth_equ_to_earth_equ = mat_earth_equ_to_prec_earth_equ.transpose();

	mat_helio_to_earth_equ = 	  
	  Mat4d::rotation( axis, -precession) *
	  Mat4d::xrotation(23.438855*M_PI/180.) *
	  Mat4d::translation(-earth_ecliptic_pos);
	

	// These two next have to take into account the position of the observer on the earth
	Mat4d tmp = 	  
	  Mat4d::xrotation(-23.438855*M_PI/180.) *
	  Mat4d::rotation( axis, precession) *
	  Mat4d::zrotation((position->get_longitude()+get_mean_sidereal_time(JDay))*M_PI/180.) *
	  Mat4d::yrotation((90.-lat)*M_PI/180.);


	mat_local_to_helio = 	Mat4d::translation(earth_ecliptic_pos) *
							tmp *
							Mat4d::translation(Vec3d(0.,0., 6378.1/AU+(double)position->get_altitude()/AU/1000));

	mat_helio_to_local = 	Mat4d::translation(Vec3d(0.,0.,-6378.1/AU-(double)position->get_altitude()/AU/1000)) *
							tmp.transpose() *
							Mat4d::translation(-earth_ecliptic_pos);

}

////////////////////////////////////////////////////////////////////////////////
// Update the modelview matrices
void navigator::update_model_view_mat(void)
{

  Vec3d f;

  if( viewing_mode == VIEW_EQUATOR) {
    // view will use equatorial coordinates, so that north is always up
    f = equ_vision;
  } else {
    // view will correct for horizon (always down)
    f = local_vision;
  }


  f.normalize();
  Vec3d s(f[1],-f[0],0.);
  

  if( viewing_mode == VIEW_EQUATOR) {
    // convert everything back to local coord
    f = local_vision;
    f.normalize();
    s = earth_equ_to_local( s );
  }

  Vec3d u(s^f);
  s.normalize();
  u.normalize();

  mat_local_to_eye.set(s[0],u[0],-f[0],0.,
		       s[1],u[1],-f[1],0.,
		       s[2],u[2],-f[2],0.,
		       0.,0.,0.,1.);

  mat_earth_equ_to_eye = mat_local_to_eye*mat_earth_equ_to_local;
  mat_helio_to_eye = mat_local_to_eye*mat_helio_to_local;
  mat_prec_earth_equ_to_eye = mat_local_to_eye*mat_earth_equ_to_local*mat_prec_earth_equ_to_earth_equ;
}

////////////////////////////////////////////////////////////////////////////////
// Return the observer heliocentric position
Vec3d navigator::get_observer_helio_pos(void) const
{
	Vec3d v(0.,0.,0.);
	return mat_local_to_helio*v;
}

////////////////////////////////////////////////////////////////////////////////
// Move to the given equatorial position
void navigator::move_to(const Vec3d& _aim, float move_duration, bool _local_pos, int zooming)
{
	zooming_mode = zooming;
	move.aim=_aim;
    move.aim.normalize();
    move.aim*=2.;
	if (_local_pos)
	{
		move.start=local_vision;
	}
	else
	{
		move.start=equ_vision;
	}
    move.start.normalize();
    move.speed=1.f/(move_duration*1000);
    move.coef=0.;
	move.local_pos = _local_pos;
    flag_auto_move = true;
}


////////////////////////////////////////////////////////////////////////////////
// Set type of viewing mode (align with horizon or equatorial coordinates)
void navigator::set_viewing_mode(VIEWING_MODE_TYPE view_mode)
{
  viewing_mode = view_mode;

  // TODO: include some nice smoothing function trigger here to rotate between
  // the two modes 

}

void navigator::switch_viewing_mode(void)
{
	if (viewing_mode==VIEW_HORIZON) set_viewing_mode(VIEW_EQUATOR);
	else set_viewing_mode(VIEW_HORIZON);
}
