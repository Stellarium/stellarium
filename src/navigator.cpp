/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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
#include "solarsystem.h"


////////////////////////////////////////////////////////////////////////////////
Navigator::Navigator(Observator* obs) : flag_traking(0), flag_lock_equ_pos(0), flag_auto_move(0),
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

}

Navigator::~Navigator()
{}

////////////////////////////////////////////////////////////////////////////////
void Navigator::update_vision_vector(int delta_time,const StelObject &selected)
{
	if (flag_auto_move)
	{
		double ra_aim, de_aim, ra_start, de_start, ra_now, de_now;

		if( zooming_mode == 1 && selected)
		{
			// if zooming in, object may be moving so be sure to zoom to latest position
			move.aim = selected.get_earth_equ_pos(this);
			move.aim.normalize();
			move.aim*=2.;
		}

		// Use a smooth function
		float smooth = 4.f;
		double c;

		if (zooming_mode == 1)
		{
			if( move.coef > .9 )
			{
				c = 1;
			}
			else
			{
				c = 1 - pow(1.-1.11*(move.coef),3);
			}
		}
		else if(zooming_mode == -1)
		{
			if( move.coef < 0.1 )
			{
				// keep in view at first as zoom out
				c = 0;

				/* could track as moves too, but would need to know if start was actually
				   a zoomed in view on the object or an extraneous zoom out command
				   if(move.local_pos) {
				   move.start=earth_equ_to_local(selected.get_earth_equ_pos(this));
				   } else {
				   move.start=selected.get_earth_equ_pos(this);
				   }
				   move.start.normalize();
				*/

			}
			else
			{
				c =  pow(1.11*(move.coef-.1),3);
			}
		}
		else c = atanf(smooth * 2.*move.coef-smooth)/atanf(smooth)/2+0.5;


		if (move.local_pos)
		{
			StelUtils::rect_to_sphe(&ra_aim, &de_aim, move.aim);
			StelUtils::rect_to_sphe(&ra_start, &de_start, move.start);
		}
		else
		{
			StelUtils::rect_to_sphe(&ra_aim, &de_aim, earth_equ_to_local(move.aim));
			StelUtils::rect_to_sphe(&ra_start, &de_start, earth_equ_to_local(move.start));
		}
		
		// Trick to choose the good moving direction and never travel on a distance > PI
		if (ra_aim-ra_start > M_PI)
		{
			ra_aim -= 2.*M_PI;
		}
		else if (ra_aim-ra_start < -M_PI)
		{
			ra_aim += 2.*M_PI;
		}
		
		de_now = de_aim*c + de_start*(1. - c);
		ra_now = ra_aim*c + ra_start*(1. - c);
		
		StelUtils::sphe_to_rect(ra_now, de_now, local_vision);
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
			equ_vision=selected.get_earth_equ_pos(this);
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

	prec_equ_vision = mat_earth_equ_to_j2000*equ_vision;


}

////////////////////////////////////////////////////////////////////////////////
void Navigator::set_local_vision(const Vec3d& _pos)
{
	local_vision = _pos;
	equ_vision=local_to_earth_equ(local_vision);
	prec_equ_vision = mat_earth_equ_to_j2000*equ_vision;
}

////////////////////////////////////////////////////////////////////////////////
void Navigator::update_move(double deltaAz, double deltaAlt)
{
	double azVision, altVision;

	if( viewing_mode == VIEW_EQUATOR) StelUtils::rect_to_sphe(&azVision,&altVision,equ_vision);
	else StelUtils::rect_to_sphe(&azVision,&altVision,local_vision);

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
			StelUtils::sphe_to_rect(azVision, altVision, equ_vision);
			local_vision=earth_equ_to_local(equ_vision);
		}
		else
		{
			StelUtils::sphe_to_rect(azVision, altVision, local_vision);
			// Calc the equatorial coordinate of the direction of vision wich was in Altazimuthal coordinate
			equ_vision=local_to_earth_equ(local_vision);
			prec_equ_vision = mat_earth_equ_to_j2000*equ_vision;
		}
	}

	// Update the final modelview matrices
	update_model_view_mat();

}

////////////////////////////////////////////////////////////////////////////////
// Increment time
void Navigator::update_time(int delta_time)
{
	JDay+=time_speed*(double)delta_time/1000.;

	// Fix time limits to -100000 to +100000 to prevent bugs
	if (JDay>38245309.499988) JDay = 38245309.499988;
	if (JDay<-34803211.500012) JDay = -34803211.500012;
}

////////////////////////////////////////////////////////////////////////////////
// The non optimized (more clear version is available on the CVS : before date 25/07/2003)

  // see vsop87.doc:

const Mat4d mat_j2000_to_vsop87(
              Mat4d::xrotation(-23.4392803055555555556*(M_PI/180)) *
              Mat4d::zrotation(0.0000275*(M_PI/180)));

const Mat4d mat_vsop87_to_j2000(mat_j2000_to_vsop87.transpose());


void Navigator::update_transform_matrices(void)
{
	mat_local_to_earth_equ = position->getRotLocalToEquatorial(JDay);
	mat_earth_equ_to_local = mat_local_to_earth_equ.transpose();

	mat_earth_equ_to_j2000 = mat_vsop87_to_j2000
                           * position->getRotEquatorialToVsop87();
	mat_j2000_to_earth_equ = mat_earth_equ_to_j2000.transpose();

	mat_helio_to_earth_equ =
	    mat_j2000_to_earth_equ *
        mat_vsop87_to_j2000 *
	    Mat4d::translation(-position->getCenterVsop87Pos());


	// These two next have to take into account the position of the observer on the earth
	Mat4d tmp =
	    mat_j2000_to_vsop87 *
	    mat_earth_equ_to_j2000 *
        mat_local_to_earth_equ;

	mat_local_to_helio =  Mat4d::translation(position->getCenterVsop87Pos()) *
	                      tmp *
	                      Mat4d::translation(Vec3d(0.,0., position->getDistanceFromCenter()));

	mat_helio_to_local =  Mat4d::translation(Vec3d(0.,0.,-position->getDistanceFromCenter())) *
	                      tmp.transpose() *
	                      Mat4d::translation(-position->getCenterVsop87Pos());
}

////////////////////////////////////////////////////////////////////////////////
// Update the modelview matrices
void Navigator::update_model_view_mat(void)
{

	Vec3d f;

	if( viewing_mode == VIEW_EQUATOR)
	{
		// view will use equatorial coordinates, so that north is always up
		f = equ_vision;
	}
	else
	{
		// view will correct for horizon (always down)
		f = local_vision;
	}


	f.normalize();
	Vec3d s(f[1],-f[0],0.);


	if( viewing_mode == VIEW_EQUATOR)
	{
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

//johannes
//    mat_local_to_eye =  Mat4d::zrotation(0.5*M_PI) * mat_local_to_eye;

	mat_earth_equ_to_eye = mat_local_to_eye*mat_earth_equ_to_local;
	mat_helio_to_eye = mat_local_to_eye*mat_helio_to_local;
	mat_j2000_to_eye = mat_earth_equ_to_eye*mat_j2000_to_earth_equ;
}

////////////////////////////////////////////////////////////////////////////////
// Return the observer heliocentric position
Vec3d Navigator::get_observer_helio_pos(void) const
{
	Vec3d v(0.,0.,0.);
	return mat_local_to_helio*v;
}

////////////////////////////////////////////////////////////////////////////////
// Move to the given equatorial position
void Navigator::move_to(const Vec3d& _aim, float move_duration, bool _local_pos, int zooming)
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
void Navigator::set_viewing_mode(VIEWING_MODE_TYPE view_mode)
{
	viewing_mode = view_mode;

	// TODO: include some nice smoothing function trigger here to rotate between
	// the two modes

}

void Navigator::switch_viewing_mode(void)
{
	if (viewing_mode==VIEW_HORIZON) set_viewing_mode(VIEW_EQUATOR);
	else set_viewing_mode(VIEW_HORIZON);
}
