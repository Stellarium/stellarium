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

#include "Navigator.hpp"
#include "stellarium.h"
#include "StelUtils.hpp"
#include "StelObject.hpp"
#include "SolarSystem.hpp"


////////////////////////////////////////////////////////////////////////////////
Navigator::Navigator(Observer* obs) : time_speed(JD_SECOND), JDay(0.), position(obs)
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

void Navigator::init(const InitParser& conf, LoadingBar& lb)
{
	setTimeNow();
	setLocalVision(Vec3f(1,1e-05,0.2));
	// Compute transform matrices between coordinates systems
	updateTransformMatrices();
	updateModelViewMat();
	string tmpstr = conf.get_str("navigation:viewing_mode");
	if (tmpstr=="equator")
		setViewingMode(Navigator::VIEW_EQUATOR);
	else
	{
		if (tmpstr=="horizon")
			setViewingMode(Navigator::VIEW_HORIZON);
		else
		{
			cerr << "ERROR : Unknown viewing mode type : " << tmpstr << endl;
			assert(0);
		}
	}
	initViewPos = StelUtils::str_to_vec3f(conf.get_str("navigation:init_view_pos"));
	setLocalVision(initViewPos);
	
	// Navigation section
	PresetSkyTime 		= conf.get_double ("navigation","preset_sky_time",2451545.);
	StartupTimeMode 	= conf.get_str("navigation:startup_time_mode");	// Can be "now" or "preset"
	if (StartupTimeMode=="preset" || StartupTimeMode=="Preset")
		setJDay(PresetSkyTime - get_GMT_shift_from_system(PresetSkyTime,0) * JD_HOUR);
	else setTimeNow();
}

//! Set stellarium time to current real world time
void Navigator::setTimeNow()
{
	setJDay(get_julian_from_sys());
}

//! Get whether the current stellarium time is the real world time
bool Navigator::getIsTimeNow(void) const
{
	// cache last time to prevent to much slow system call
	static double lastJD = getJDay();
	static bool previousResult = (fabs(getJDay()-get_julian_from_sys())<JD_SECOND);
	if (fabs(lastJD-getJDay())>JD_SECOND/4)
	{
		lastJD = getJDay();
		previousResult = (fabs(getJDay()-get_julian_from_sys())<JD_SECOND);
	}
	return previousResult;
}


////////////////////////////////////////////////////////////////////////////////
void Navigator::setLocalVision(const Vec3d& _pos)
{
	local_vision = _pos;
	equ_vision=local_to_earth_equ(local_vision);
	prec_equ_vision = mat_earth_equ_to_j2000*equ_vision;
}

////////////////////////////////////////////////////////////////////////////////
void Navigator::setEquVision(const Vec3d& _pos)
{
	equ_vision = _pos;
	prec_equ_vision = mat_earth_equ_to_j2000*equ_vision;
	local_vision = earth_equ_to_local(equ_vision);
}

////////////////////////////////////////////////////////////////////////////////
void Navigator::setPrecEquVision(const Vec3d& _pos)
{
	prec_equ_vision = _pos;
	equ_vision = mat_j2000_to_earth_equ*prec_equ_vision;
	local_vision = earth_equ_to_local(equ_vision);
}

////////////////////////////////////////////////////////////////////////////////
// Increment time
void Navigator::updateTime(int delta_time)
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


void Navigator::updateTransformMatrices(void)
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
void Navigator::updateModelViewMat(void)
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
Vec3d Navigator::getObserverHelioPos(void) const
{
	Vec3d v(0.,0.,0.);
	return mat_local_to_helio*v;
}



////////////////////////////////////////////////////////////////////////////////
// Set type of viewing mode (align with horizon or equatorial coordinates)
void Navigator::setViewingMode(ViewingModeType view_mode)
{
	viewing_mode = view_mode;

	// TODO: include some nice smoothing function trigger here to rotate between
	// the two modes

}
