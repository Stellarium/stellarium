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

double Navigator::defaultAngleOfView = 1.570796326794896619231321691639f;
double Navigator::distance2selected = .0f;
double Navigator::currentTranslation = .0f;
double Navigator::radiusOfSelected = .0f;
std::string Navigator::nameOfSelected = "";
double Navigator::desiredAngleOfViewSelected = Navigator::defaultAngleOfView;
double Navigator::currentAngleOfViewSelected = Navigator::defaultAngleOfView;
double Navigator::initialAngleOfViewSelected = Navigator::defaultAngleOfView;
int Navigator::flyingSteps = 300;

////////////////////////////////////////////////////////////////////////////////
Navigator::Navigator(Observator* obs, Projector *proj) : flag_tracking(0), flag_lock_equ_pos(0), flag_auto_move(0),
time_speed(JD_SECOND), JDay(0.), position(obs), projector(proj)
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

  speed = 1.0;
  coef = 0.0;
  aimAngleX = 0.0;
  startAngleX = 0.0;
  currentAngleX = 0.0;
  centerFovRatio = 0.0;

  aimAngleZ = 0.0;
  startAngleZ = 0.0;
  currentAngleZ = 0.0;
  centerRotationZ = 0.0;//-80.214; // this is depends on how argus calibration has been performed

  do_stop = false;
}

Navigator::~Navigator()
{}


double Navigator::qube_stoping(double x)
{
	return a_coef*x*x*x + b_coef*x*x + c_coef*x + d_coef;
}


////////////////////////////////////////////////////////////////////////////////
void Navigator::update_vision_vector(int delta_time,const StelObject &selected)
{
  // if no selection or tracking (deselect for example), 
  // reset all control variables
  if ((!selected || !flag_tracking) && fabs(aimAngleX) >= 0.00001)
  {
	do_stop = false; ////

    aimAngleX = 0.0;
    startAngleX = currentAngleX;
    coef = 0.0;

    aimAngleZ = 0.0;
    startAngleZ = currentAngleZ;
  }
  // kornyakov: if selected, start to move object to the right position
  // but this moving is additional to the stellarium work
  else if (selected && flag_tracking && fabs(aimAngleX - centerFovRatio) >= 0.00001)
  {
    aimAngleX = centerFovRatio;
    startAngleX = currentAngleX;
    coef = 0.0;

    aimAngleZ = centerRotationZ;
    startAngleZ = currentAngleZ;
  }

  if (fabs(aimAngleX - currentAngleX) >= 0.00001)
  {
    coef += speed * delta_time; 
	
	if (coef >= 0.99)
    {
      coef = 1.0;
    }
    currentAngleX = coef * (aimAngleX - startAngleX) + startAngleX;
    currentAngleZ = coef * (aimAngleZ - startAngleZ) + startAngleZ;
    this->update_model_view_mat();
  }
  else
  {
    currentAngleX = aimAngleX;
    currentAngleZ = aimAngleZ;
  }

 	if (flag_auto_move)
	{
		double ra_aim, de_aim, ra_start, de_start, ra_now, de_now;

    if (zooming_mode == 1 && selected)
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
			if (fabs(aimAngleX - currentAngleX) <= 0.000515)
			{
				c = 1.0;
			}
			else if(do_stop) ////
			{
				c = qube_stoping(move.coef);
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

				//// could track as moves too, but would need to know if start was actually
				////   a zoomed in view on the object or an extraneous zoom out command
				//   if(move.local_pos) {
				//   move.start=earth_equ_to_local(selected.get_earth_equ_pos(this));
				//   } else {
				//   move.start=selected.get_earth_equ_pos(this);
				//   }
				//   move.start.normalize();
			}
			else
			{
				c =  pow(1.11*(move.coef-.1),3);
			}
		}
		else if(do_stop) ////
		{
			c = qube_stoping(move.coef);
		}
		else c = atanf(smooth * 2. * move.coef - smooth)/atanf(smooth)/2+0.5;

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
		
		if(!do_stop)  // плохо что считает каждый раз .. но думаю пока что можно оставить так .. 
		{
			if (zooming_mode == 1)
			{
				a_coef = 16.*(1 - pow(1.-1.11*(move.coef),3)) - 16 - 4*3.33*pow(1.-1.11*(move.coef),2);
				b_coef = -36.*(1 - pow(1.-1.11*(move.coef),3)) + 36 + 10*3.33*pow(1.-1.11*(move.coef),2);
				c_coef = 24.*(1 - pow(1.-1.11*(move.coef),3)) - 24 - 8*3.33*pow(1.-1.11*(move.coef),2);
				d_coef = -4.*(1 - pow(1.-1.11*(move.coef),3)) + 5 + 2*3.33*pow(1.-1.11*(move.coef),2);
			}
			/*else
			if (zooming_mode == -1)
			{
				a_coef = 16.*pow(1.11*(move.coef-.1),3) - 16 - 4*3.33*pow(1.11*(move.coef-.1),2);
				b_coef = -36.*pow(1.11*(move.coef-.1),3) + 36 + 10*3.33*pow(1.11*(move.coef-.1),2);
				c_coef = 24.*pow(1.11*(move.coef-.1),3) - 24 - 8*3.33*pow(1.11*(move.coef-.1),2);
				d_coef = -4.*pow(1.11*(move.coef-.1),3) + 5 + 2*3.33*pow(1.11*(move.coef-.1),2);				
			}*/
			else if(zooming_mode != -1)
			{
				a_coef = 16.*(atanf(smooth * 2. * move.coef - smooth)/atanf(smooth)/2 + 0.5) - 16 - 16./(atanf(4.)*(1. + pow(8.*move.coef - 4., 2)));
				b_coef = -36.*(atanf(smooth * 2. * move.coef - smooth)/atanf(smooth)/2 + 0.5) + 36 + 40./(atanf(4.)*(1. + pow(8.*move.coef - 4., 2)));
				c_coef = 24.*(atanf(smooth * 2. * move.coef - smooth)/atanf(smooth)/2 + 0.5) - 24 - 32./(atanf(4.)*(1. + pow(8.*move.coef - 4., 2)));
				d_coef = -4.*(atanf(smooth * 2. * move.coef - smooth)/atanf(smooth)/2 + 0.5) + 5 + 8./(atanf(4.)*(1. + pow(8.*move.coef - 4., 2)));
			}
		}

		sphe_to_rect(ra_now, de_now, local_vision);
		equ_vision = local_to_earth_equ(local_vision);
		
		move.coef += move.speed*delta_time;

		do_stop = false; ////

		if (move.coef >= 0.999)
		{
			flag_auto_move = 0;
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
		if((((zooming_mode == 1)&&(move.coef > 0.9)) || ((zooming_mode != 1)&&(move.coef > 0.748))) && (move.coef < 0.999)) do_stop = true; 
															/// одно из лучших значений : 0.748
	}
	else
	{
		if (flag_tracking && selected) // Equatorial vision vector locked on selected object
		{
			equ_vision=selected.get_earth_equ_pos(this);

			// Recalc local vision vector

			local_vision=earth_equ_to_local(equ_vision);

			local_vision[2] += 0.00000000001;
			/*
			local_vision[0] += 0.00000001;
			local_vision[1] += 0.00000001;
			local_vision[2] += 0.0;*/

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

	if (viewing_mode == VIEW_EQUATOR) 
    rect_to_sphe(&azVision, &altVision, equ_vision);
	else 
    rect_to_sphe(&azVision, &altVision, local_vision);

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
// kornyakov:
// Get the new distance to the selected object while we flying to it
double Navigator::GetTranslationValue(void)
{
  float deltaDistance = 0;
  float angleStep = (initialAngleOfViewSelected-desiredAngleOfViewSelected)/flyingSteps;

  if (radiusOfSelected != .0f)
    currentAngleOfViewSelected = atan((distance2selected - currentTranslation)/radiusOfSelected);

  // we do nothing if we don't fly
  if (desiredAngleOfViewSelected != initialAngleOfViewSelected)
  {
    float curr_x = (distance2selected - currentTranslation);
    float new_tangent;
    if (currentAngleOfViewSelected > desiredAngleOfViewSelected)
      new_tangent = tan(currentAngleOfViewSelected - angleStep);
    else
      new_tangent = tan(currentAngleOfViewSelected + angleStep);

    deltaDistance = curr_x - radiusOfSelected * new_tangent; 
    if ((currentAngleOfViewSelected < desiredAngleOfViewSelected)&&(initialAngleOfViewSelected<desiredAngleOfViewSelected))
      deltaDistance *= -1.0;
    //if ((desiredAngleOfViewSelected < 0.9999*currentAngleOfViewSelected)||
    //    (desiredAngleOfViewSelected > 1.0001*currentAngleOfViewSelected))
    //{
    if ((currentTranslation + deltaDistance > 0)&&
        (currentTranslation + deltaDistance < distance2selected))
      currentTranslation += deltaDistance;
    //}
  }

  return currentTranslation;
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

  // rotate screen (rotating screen due calibration camera shift and moving selected objects to the front)
  Mat4d mat_rotation = //kornyakov: this is because of calibration rotation
                       Mat4d::zrotation(currentAngleZ / 180.0 * M_PI) * 
                       //kornyakov: this is for moving objects from zenith to the front
                       Mat4d::xrotation(currentAngleX * projector->get_fov() / 2.0 / 180.0 * M_PI) * 
                       Mat4d::identity();
  // and fly to the selected object
  Vec3d translation(.0, .0, GetTranslationValue());
  Mat4d mat_translation = Mat4d::translation(translation);

  mat_local_to_eye = mat_rotation * mat_local_to_eye;

  mat_earth_equ_to_eye = mat_local_to_eye*mat_earth_equ_to_local;
  mat_j2000_to_eye = mat_earth_equ_to_eye*mat_j2000_to_earth_equ;

  mat_local_to_eye = mat_rotation * mat_translation * mat_rotation.inverse() * mat_local_to_eye;
  mat_helio_to_eye = mat_local_to_eye*mat_helio_to_local;
}

////////////////////////////////////////////////////////////////////////////////
// Return the observer heliocentric position
Vec3d Navigator::get_observer_helio_pos(void) const
{
  Vec3d v(0.,0.,0.);
  Vec3d result;
	// kornyakov: for flying to the selected object
  // we need to use eye_to_helio matrix because we displace
  // along line of view toward selected object
  result =  mat_local_to_helio*v;
  //result = mat_helio_to_eye.inverse()*v;
  return result;
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
	
  speed = move.speed;
  coef = 0.0;
  aimAngleX = centerFovRatio;
  startAngleX = currentAngleX;
  
  if (fabs(aimAngleX - startAngleX) >= 0.00001)
  {
      aimAngleZ = centerRotationZ;
      startAngleZ = currentAngleZ;
  }
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
