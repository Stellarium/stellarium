/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#include "MovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "InitParser.hpp"
#include "Navigator.hpp"

MovementMgr::MovementMgr(StelCore* acore) : core(acore), flag_lock_equ_pos(false), flagTracking(false), is_mouse_moving_horiz(false), is_mouse_moving_vert(false), 
	flag_auto_move(0), deltaFov(0.), deltaAlt(0.), deltaAz(0.), move_speed(0.00025), flag_auto_zoom(0)
{
	setObjectName("MovementMgr");
	is_dragging = false;
}

MovementMgr::~MovementMgr()
{
}

void MovementMgr::init(const InitParser& conf)
{
	FlagEnableMoveMouse	= conf.get_boolean("navigation","flag_enable_move_mouse",1);
	MouseZoom			= conf.get_int("navigation","mouse_zoom",30);
	FlagEnableZoomKeys	= conf.get_boolean("navigation:flag_enable_zoom_keys");
	FlagEnableMoveKeys  = conf.get_boolean("navigation:flag_enable_move_keys");
	move_speed			= conf.get_double("navigation","move_speed",0.0004);
	zoom_speed			= conf.get_double("navigation","zoom_speed", 0.0004);
	auto_move_duration	= conf.get_double ("navigation","auto_move_duration",1.5);
	FlagManualZoom		= conf.get_boolean("navigation:flag_manual_zoom");
}	
	
bool MovementMgr::handleMouseMoves(Uint16 x, Uint16 y, StelMod mod)
{
	// Turn if the mouse is at the edge of the screen.
	// unless config asks otherwise
	if(FlagEnableMoveMouse)
	{
		if (x == 0)
		{
			turn_left(1);
			is_mouse_moving_horiz = true;
		}
		else if (x == core->getProjection()->getViewportWidth() - 1)
		{
			turn_right(1);
			is_mouse_moving_horiz = true;
		}
		else if (is_mouse_moving_horiz)
		{
			turn_left(0);
			is_mouse_moving_horiz = false;
		}

		if (y == 0)
		{
			turn_up(1);
			is_mouse_moving_vert = true;
		}
		else if (y == core->getProjection()->getViewportHeight() - 1)
		{
			turn_down(1);
			is_mouse_moving_vert = true;
		}
		else if (is_mouse_moving_vert)
		{
			turn_up(0);
			is_mouse_moving_vert = false;
		}
	}
	
	if (is_dragging)
	{
		if (has_dragged ||
		    (std::sqrt((double)((x-previous_x)*(x-previous_x)
								+(y-previous_y)*(y-previous_y)))>4.))
		{
			has_dragged = true;
			setFlagTracking(false);
			dragView(previous_x, previous_y, x, y);
			previous_x = x;
			previous_y = y;
			return true;
		}
	}
	return false;
}


bool MovementMgr::handleKeys(StelKey key, StelMod mod, Uint16 unicode, Uint8 state)
{
	if (state == Stel_KEYDOWN)
	{
		// Direction and zoom deplacements
		if (key==StelKey_LEFT) turn_left(1);
		if (key==StelKey_RIGHT) turn_right(1);
		if (key==StelKey_UP)
		{
			if (mod & StelMod_CTRL) zoom_in(1);
			else turn_up(1);
		}
		if (key==StelKey_DOWN)
		{
			if (mod & StelMod_CTRL) zoom_out(1);
			else turn_down(1);
		}
		if (key==StelKey_PAGEUP) zoom_in(1);
		if (key==StelKey_PAGEDOWN) zoom_out(1);
	}
	else
	{
		// When a deplacement key is released stop mooving
		if (key==StelKey_LEFT) turn_left(0);
		if (key==StelKey_RIGHT) turn_right(0);
		if (mod & StelMod_CTRL)
		{
			if (key==StelKey_UP) zoom_in(0);
			if (key==StelKey_DOWN) zoom_out(0);
		}
		else
		{
			if (key==StelKey_UP) turn_up(0);
			if (key==StelKey_DOWN) turn_down(0);
		}
		if (key==StelKey_PAGEUP) zoom_in(0);
		if (key==StelKey_PAGEDOWN) zoom_out(0);
	}
	if (key==StelKey_LEFT || key==StelKey_RIGHT || key==StelKey_UP || key==StelKey_DOWN || key==StelKey_PAGEUP || key==StelKey_PAGEDOWN)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool MovementMgr::handleMouseClicks(Uint16 x, Uint16 y, Uint8 button, Uint8 state, StelMod mod)
{
	switch (button)
	{
	case Stel_BUTTON_WHEELUP :
		zoomTo(getAimFov()-MouseZoom*getAimFov()/60., 0.2);
		return true;
	case Stel_BUTTON_WHEELDOWN :
		zoomTo(getAimFov()+MouseZoom*getAimFov()/60., 0.2);
		return true;
	case Stel_BUTTON_RIGHT : break;
	case Stel_BUTTON_LEFT :
		if (state==Stel_MOUSEBUTTONDOWN)
		{
			is_dragging = true;
			has_dragged = false;
			previous_x = x;
			previous_y = y;
			return true;
		}
		else
		{
			if (is_dragging)
			{
				is_dragging = false;
				if (has_dragged)
					return true;
				else
					return false;
			}
		}
		break;
	case Stel_BUTTON_MIDDLE :
		if (state==Stel_MOUSEBUTTONUP)
		{
			if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
			{
				moveTo(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getEarthEquatorialPos(core->getNavigation()),auto_move_duration);
				setFlagTracking(true);
			}
		}
		break;
	default: break;
	}
	
	return false;
}

/*************************************************************************
 The selected objects changed, follow it it we were already following another one
*************************************************************************/ 
void MovementMgr::selectedObjectChangeCallBack(StelModuleSelectAction action)
{
	// If an object was selected keep the earth following
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
	{
		if (getFlagTracking())
			setFlagLockEquPos(true);
		setFlagTracking(false);
	}
}

void MovementMgr::turn_right(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = 1;
		setFlagTracking(false);
		setFlagLockEquPos(false);
	}
	else
		deltaAz = 0;
}

void MovementMgr::turn_left(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = -1;
		setFlagTracking(false);
		setFlagLockEquPos(false);

	}
	else
		deltaAz = 0;
}

void MovementMgr::turn_up(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = 1;
		setFlagTracking(false);
		setFlagLockEquPos(false);
	}
	else
		deltaAlt = 0;
}

void MovementMgr::turn_down(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = -1;
		setFlagTracking(false);
		setFlagLockEquPos(false);
	}
	else
		deltaAlt = 0;
}


void MovementMgr::zoom_in(int s)
{
	if (FlagEnableZoomKeys)
		deltaFov = -1*(s!=0);
}

void MovementMgr::zoom_out(int s)
{
	if (FlagEnableZoomKeys)
		deltaFov = (s!=0);
}


// Increment/decrement smoothly the vision field and position
void MovementMgr::updateMotion(double deltaTime)
{
	Projector* proj = core->getProjection();
	
	updateVisionVector(deltaTime);
	
	// the more it is zoomed, the lower the moving speed is (in angle)
	double depl=move_speed*deltaTime*1000*proj->getFov();
//	double deplzoom=zoom_speed*deltaTime*1000*proj->getFov();
	double deplzoom=zoom_speed*deltaTime*1000*proj->getMapping().
	                  deltaZoom(proj->getFov()*(M_PI/360.0))*(360.0/M_PI);

	if (deltaAz<0)
	{
		deltaAz = -depl/30;
		if (deltaAz<-0.2)
			deltaAz = -0.2;
	}
	else
	{
		if (deltaAz>0)
		{
			deltaAz = (depl/30);
			if (deltaAz>0.2)
				deltaAz = 0.2;
		}
	}
	if (deltaAlt<0)
	{
		deltaAlt = -depl/30;
		if (deltaAlt<-0.2)
			deltaAlt = -0.2;
	}
	else
	{
		if (deltaAlt>0)
		{
			deltaAlt = depl/30;
			if (deltaAlt>0.2)
				deltaAlt = 0.2;
		}
	}

	if (deltaFov<0)
	{
		deltaFov = -deplzoom*5;
		if (deltaFov<-0.15*proj->getFov())
			deltaFov = -0.15*proj->getFov();
	}
	else
	{
		if (deltaFov>0)
		{
			deltaFov = deplzoom*5;
			if (deltaFov>20)
				deltaFov = 20;
		}
	}

	if(deltaFov != 0 )
	{
		change_fov(deltaFov);
		std::ostringstream oss;
		oss << "zoom delta_fov " << deltaFov;
	}

	if(deltaAz != 0 || deltaAlt != 0)
	{
		panView(deltaAz, deltaAlt);
		std::ostringstream oss;
		oss << "look delta_az " << deltaAz << " delta_alt " << deltaAlt;
	}
	else
	{
		// must perform call anyway, but don't record!
		panView(deltaAz, deltaAlt);
	}
	
	update_auto_zoom(deltaTime);
}


// Go and zoom to the selected object.
void MovementMgr::autoZoomIn(float move_duration, bool allow_manual_zoom)
{
	Projector* proj = core->getProjection();
	
	if (!StelApp::getInstance().getStelObjectMgr().getWasSelected())
		return;
		
	float manual_move_duration;

	if (!getFlagTracking())
	{
		setFlagTracking(true);
		moveTo(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getEarthEquatorialPos(core->getNavigation()), move_duration, false, 1);
		manual_move_duration = move_duration;
	}
	else
	{
		// faster zoom in manual zoom mode once object is centered
		manual_move_duration = move_duration*.66f;
	}

	if( allow_manual_zoom && FlagManualZoom )
	{
		// if manual zoom mode, user can zoom in incrementally
		float newfov = proj->getFov()*0.5f;
		zoomTo(newfov, manual_move_duration);
	}
	else
	{
		float satfov = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->get_satellites_fov(core->getNavigation());

		if (satfov>0.0 && proj->getFov()*0.9>satfov)
			zoomTo(satfov, move_duration);
		else
		{
			float closefov = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getCloseViewFov(core->getNavigation());
			if (proj->getFov()>closefov)
				zoomTo(closefov, move_duration);
		}
	}
}


// Unzoom and go to the init position
void MovementMgr::autoZoomOut(float move_duration, bool full)
{
	Navigator* nav = core->getNavigation();
	Projector* proj = core->getProjection();
	
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected() && !full)
	{
		// If the selected object has satellites, unzoom to satellites view
		// unless specified otherwise
		float satfov = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->get_satellites_fov(core->getNavigation());

		if (satfov>0.0 && proj->getFov()<=satfov*0.9)
		{
			zoomTo(satfov, move_duration);
			return;
		}

		// If the selected object is part of a Planet subsystem (other than sun),
		// unzoom to subsystem view
		satfov = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->get_parent_satellites_fov((core->getNavigation()));
		if (satfov>0.0 && proj->getFov()<=satfov*0.9)
		{
			zoomTo(satfov, move_duration);
			return;
		}
	}

	zoomTo(proj->getInitFov(), move_duration);
	moveTo(nav->getinitViewPos(), move_duration, true, -1);
	setFlagTracking(false);
	setFlagLockEquPos(false);
}


void MovementMgr::setFlagTracking(bool b)
{
	if(!b || !StelApp::getInstance().getStelObjectMgr().getWasSelected())
	{
		flagTracking=false;
	}
	else
	{
		moveTo(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getEarthEquatorialPos(core->getNavigation()), getAutoMoveDuration());
		flagTracking=true;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Move to the given equatorial position
void MovementMgr::moveTo(const Vec3d& _aim, float move_duration, bool _local_pos, int zooming)
{
	Navigator* nav = core->getNavigation();
	zooming_mode = zooming;
	move.aim=_aim;
	move.aim.normalize();
	move.aim*=2.;
	if (_local_pos)
	{
		move.start=nav->getLocalVision();
	}
	else
	{
		move.start=nav->getEquVision();
	}
	move.start.normalize();
	move.speed=1.f/(move_duration*1000);
	move.coef=0.;
	move.local_pos = _local_pos;
	flag_auto_move = true;
}


////////////////////////////////////////////////////////////////////////////////
void MovementMgr::updateVisionVector(double deltaTime)
{
	Navigator* nav = core->getNavigation();
	if (flag_auto_move)
	{
		double ra_aim, de_aim, ra_start, de_start, ra_now, de_now;

		if( zooming_mode == 1 && StelApp::getInstance().getStelObjectMgr().getWasSelected())
		{
			// if zooming in, object may be moving so be sure to zoom to latest position
			move.aim = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getEarthEquatorialPos(core->getNavigation());
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
				   move.start=earth_equ_to_local(selected.getEarthEquatorialPos(this));
				   } else {
				   move.start=selected.getEarthEquatorialPos(this);
				   }
				   move.start.normalize();
				*/

			}
			else
			{
				c =  pow(1.11*(move.coef-.1),3);
			}
		}
		else c = std::atan(smooth * 2.*move.coef-smooth)/std::atan(smooth)/2+0.5;


		if (move.local_pos)
		{
			StelUtils::rect_to_sphe(&ra_aim, &de_aim, move.aim);
			StelUtils::rect_to_sphe(&ra_start, &de_start, move.start);
		}
		else
		{
			StelUtils::rect_to_sphe(&ra_aim, &de_aim, nav->earth_equ_to_local(move.aim));
			StelUtils::rect_to_sphe(&ra_start, &de_start, nav->earth_equ_to_local(move.start));
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
		
		Vec3d tmp;
		StelUtils::sphe_to_rect(ra_now, de_now, tmp);
		nav->setEquVision(nav->local_to_earth_equ(tmp));

		move.coef+=move.speed*deltaTime*1000;
		if (move.coef>=1.)
		{
			flag_auto_move=0;
			if (move.local_pos)
			{
				nav->setLocalVision(move.aim);
			}
			else
			{
				nav->setEquVision(move.aim);
			}
		}
	}
	else
	{
		if (flagTracking && StelApp::getInstance().getStelObjectMgr().getWasSelected()) // Equatorial vision vector locked on selected object
		{
			nav->setEquVision(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getEarthEquatorialPos(core->getNavigation()));
		}
		else
		{
			if (flag_lock_equ_pos) // Equatorial vision vector locked
			{
				// Recalc local vision vector
				nav->setLocalVision(nav->earth_equ_to_local(nav->getEquVision()));
			}
			else // Local vision vector locked
			{
				// Recalc equatorial vision vector
				nav->setEquVision(nav->local_to_earth_equ(nav->getLocalVision()));
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
void MovementMgr::panView(double deltaAz, double deltaAlt)
{
	Navigator* nav = core->getNavigation();
	double azVision, altVision;

	if( nav->getViewingMode() == Navigator::VIEW_EQUATOR) StelUtils::rect_to_sphe(&azVision,&altVision,nav->getEquVision());
	else StelUtils::rect_to_sphe(&azVision,&altVision,nav->getLocalVision());

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
		setFlagTracking(false);
		if( nav->getViewingMode() == Navigator::VIEW_EQUATOR)
		{
			Vec3d tmp;
			StelUtils::sphe_to_rect(azVision, altVision, tmp);
			nav->setLocalVision(nav->earth_equ_to_local(tmp));
		}
		else
		{
			Vec3d tmp;
			StelUtils::sphe_to_rect(azVision, altVision, tmp);
			// Calc the equatorial coordinate of the direction of vision wich was in Altazimuthal coordinate
			nav->setEquVision(nav->local_to_earth_equ(tmp));
		}
	}

	// Update the final modelview matrices
	nav->updateModelViewMat();
}


//! Make the first screen position correspond to the second (useful for mouse dragging)
void MovementMgr::dragView(int x1, int y1, int x2, int y2)
{
	Navigator* nav = core->getNavigation();
	Projector* proj = core->getProjection();
	
	Vec3d tempvec1, tempvec2;
	double az1, alt1, az2, alt2;
	if (nav->getViewingMode()==Navigator::VIEW_HORIZON)
		proj->setCurrentFrame(Projector::FRAME_LOCAL);
	else
		proj->setCurrentFrame(Projector::FRAME_EARTH_EQU);
		
//johannes: StelApp already gives appropriate x/y coordinates
//	proj->unProject(x2,proj->getViewportHeight()-y2, tempvec2);
//	proj->unProject(x1,proj->getViewportHeight()-y1, tempvec1);
	proj->unProject(x2,y2, tempvec2);
	proj->unProject(x1,y1, tempvec1);
	StelUtils::rect_to_sphe(&az1, &alt1, tempvec1);
	StelUtils::rect_to_sphe(&az2, &alt2, tempvec2);
	panView(az2-az1, alt1-alt2);
	setFlagTracking(false);
	setFlagLockEquPos(false);
}


// Update auto_zoom if activated
void MovementMgr::update_auto_zoom(double deltaTime)
{
	if (flag_auto_zoom)
	{
		Projector* proj = core->getProjection();
		
		// Use a smooth function
		double c;

		if( zoom_move.start > zoom_move.aim )
		{
			// slow down as approach final view
			c = 1 - (1-zoom_move.coef)*(1-zoom_move.coef)*(1-zoom_move.coef);
		}
		else
		{
			// speed up as leave zoom target
			c = (zoom_move.coef)*(zoom_move.coef)*(zoom_move.coef);
		}

		proj->setFov(zoom_move.start + (zoom_move.aim - zoom_move.start) * c);
		zoom_move.coef+=zoom_move.speed*deltaTime*1000;
		if (zoom_move.coef>=1.)
		{
			flag_auto_zoom = 0;
			proj->setFov(zoom_move.aim);
		}
	}
}

// Zoom to the given field of view
void MovementMgr::zoomTo(double aim_fov, float move_duration)
{
	zoom_move.aim=aim_fov;
    zoom_move.start=core->getProjection()->getFov();
    zoom_move.speed=1.f/(move_duration*1000);
    zoom_move.coef=0.;
    flag_auto_zoom = true;
}

void MovementMgr::change_fov(double deltaFov)
{
	// if we are zooming in or out
	if (deltaFov) core->getProjection()->setFov(core->getProjection()->getFov()+deltaFov);
}

double MovementMgr::getAimFov(void) const
{
	return (flag_auto_zoom ? zoom_move.aim : core->getProjection()->getFov());
}
