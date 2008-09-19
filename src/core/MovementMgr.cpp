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
#include "Navigator.hpp"
#include "StelUtils.hpp"

#include <QString>
#include <QTextStream>
#include <QSettings>
#include <QKeyEvent>
#include <QDebug>

MovementMgr::MovementMgr(StelCore* acore) : core(acore), 
	flagLockEquPos(false),
	flagTracking(false),
	isMouseMovingHoriz(false),
	isMouseMovingVert(false),
	flagEnableMouseNavigation(true),
	keyMoveSpeed(0.00025),
	flagAutoMove(0),
	deltaFov(0.),
	deltaAlt(0.),
	deltaAz(0.),
	flagAutoZoom(0),
	flagAutoZoomOutResetsDirection(0)
{
	setObjectName("MovementMgr");
	isDragging = false;
}

MovementMgr::~MovementMgr()
{
}

void MovementMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	flagEnableMoveAtScreenEdge = conf->value("navigation/flag_enable_move_at_screen_edge",false).toBool();
	mouseZoomSpeed = conf->value("navigation/mouse_zoom",30).toInt();
	flagEnableZoomKeys = conf->value("navigation/flag_enable_zoom_keys").toBool();
	flagEnableMoveKeys = conf->value("navigation/flag_enable_move_keys").toBool();
	keyMoveSpeed = conf->value("navigation/move_speed",0.0004).toDouble();
	keyZoomSpeed = conf->value("navigation/zoom_speed", 0.0004).toDouble();
	autoMoveDuration = conf->value ("navigation/auto_move_duration",1.5).toDouble();
	flagManualZoom = conf->value("navigation/flag_manual_zoom").toBool();
	flagAutoZoomOutResetsDirection = conf->value("navigation/auto_zoom_out_resets_direction", true).toBool();
	flagEnableMouseNavigation = conf->value("navigation/flag_enable_mouse_navigation",true).toBool();
}	
	
bool MovementMgr::handleMouseMoves(int x, int y, Qt::MouseButtons b)
{
	// Turn if the mouse is at the edge of the screen unless config asks otherwise
	if (flagEnableMoveAtScreenEdge)
	{
		if (x <= 1)
		{
			turnLeft(1);
			isMouseMovingHoriz = true;
		}
		else if (x >= core->getProjection()->getViewportWidth() - 2)
		{
			turnRight(1);
			isMouseMovingHoriz = true;
		}
		else if (isMouseMovingHoriz)
		{
			turnLeft(0);
			isMouseMovingHoriz = false;
		}

		if (y <= 1)
		{
			turnUp(1);
			isMouseMovingVert = true;
		}
		else if (y >= core->getProjection()->getViewportHeight() - 2)
		{
			turnDown(1);
			isMouseMovingVert = true;
		}
		else if (isMouseMovingVert)
		{
			turnUp(0);
			isMouseMovingVert = false;
		}
	}
	
	if (isDragging && flagEnableMouseNavigation)
	{
		if (hasDragged || (std::sqrt((double)((x-previousX)*(x-previousX) +(y-previousY)*(y-previousY)))>4.))
		{
			hasDragged = true;
			setFlagTracking(false);
			dragView(previousX, previousY, x, y);
			previousX = x;
			previousY = y;
			return true;
		}
	}
	return false;
}


void MovementMgr::handleKeys(QKeyEvent* event)
{
	if (event->type() == QEvent::KeyPress)
	{
		// Direction and zoom deplacements
		switch (event->key())
		{
			case Qt::Key_Left:
				turnLeft(true); break;
			case Qt::Key_Right:
				turnRight(true); break;
			case Qt::Key_Up:
				if (event->modifiers().testFlag(Qt::ControlModifier)) zoomIn(true);
				else turnUp(true);
				break;
			case Qt::Key_Down:
				if (event->modifiers().testFlag(Qt::ControlModifier)) zoomOut(true);
				else turnDown(true);
				break;
			case Qt::Key_PageUp:
				zoomIn(true); break;
			case Qt::Key_PageDown:
				zoomOut(true); break;
			default:
				return;
		}
	}
	else
	{
		// When a deplacement key is released stop mooving
		switch (event->key())
		{
			case Qt::Key_Left:
				turnLeft(false); break;
			case Qt::Key_Right:
				turnRight(false); break;
			case Qt::Key_Up:
				zoomIn(false);
				turnUp(false);
				break;
			case Qt::Key_Down:
				zoomOut(false);
				turnDown(false);
				break;
			case Qt::Key_PageUp:
				zoomIn(false); break;
			case Qt::Key_PageDown:
				zoomOut(false); break;
			default:
				return;
		}
	}
	event->accept();
}

//! Handle mouse wheel events.
void MovementMgr::handleMouseWheel(QWheelEvent* event)
{
	if (flagEnableMouseNavigation==false)
		return;
	int numDegrees = event->delta() / 8;
	int numSteps = numDegrees / 15;
	zoomTo(getAimFov()-mouseZoomSpeed*numSteps*getAimFov()/60., 0.2);
	event->accept();
}

void MovementMgr::handleMouseClicks(QMouseEvent* event)
{
	switch (event->button())
	{
		case Qt::RightButton : break;
		case Qt::LeftButton :
			if (event->type()==QEvent::MouseButtonPress)
			{
				isDragging = true;
				hasDragged = false;
				previousX = event->x();
				previousY = event->y();
				event->accept();
				return;
			}
			else
			{
				if (isDragging)
				{
					isDragging = false;
					if (hasDragged)
					{
						event->accept();
						return;
					}
					else
						return;
				}
			}
			break;
		case Qt::MidButton :
			if (event->type()==QEvent::MouseButtonRelease)
			{
				if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
				{
					moveTo(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getObsEquatorialPos(core->getNavigation()),autoMoveDuration);
					setFlagTracking(true);
				}
			}
			break;
		default: break;
	}
	return;
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

void MovementMgr::turnRight(bool s)
{
	if (s && flagEnableMoveKeys)
	{
		deltaAz = 1;
		setFlagTracking(false);
		setFlagLockEquPos(false);
	}
	else
		deltaAz = 0;
}

void MovementMgr::turnLeft(bool s)
{
	if (s && flagEnableMoveKeys)
	{
		deltaAz = -1;
		setFlagTracking(false);
		setFlagLockEquPos(false);
	}
	else
		deltaAz = 0;
}

void MovementMgr::turnUp(bool s)
{
	if (s && flagEnableMoveKeys)
	{
		deltaAlt = 1;
		setFlagTracking(false);
		setFlagLockEquPos(false);
	}
	else
		deltaAlt = 0;
}

void MovementMgr::turnDown(bool s)
{
	if (s && flagEnableMoveKeys)
	{
		deltaAlt = -1;
		setFlagTracking(false);
		setFlagLockEquPos(false);
	}
	else
		deltaAlt = 0;
}


void MovementMgr::zoomIn(bool s)
{
	if (flagEnableZoomKeys)
		deltaFov = -1*(s!=0);
}

void MovementMgr::zoomOut(bool s)
{
	if (flagEnableZoomKeys)
		deltaFov = (s!=0);
}


// Increment/decrement smoothly the vision field and position
void MovementMgr::updateMotion(double deltaTime)
{
	Projector* proj = core->getProjection();
	
	updateVisionVector(deltaTime);
	
	// the more it is zoomed, the lower the moving speed is (in angle)
	double depl=keyMoveSpeed*deltaTime*1000*proj->getFov();
	double deplzoom=keyZoomSpeed*deltaTime*1000*proj->getCurrentMapping().deltaZoom(proj->getFov()*(M_PI/360.0))*(360.0/M_PI);

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

	if (deltaFov != 0 )
	{
		changeFov(deltaFov);
	}
	panView(deltaAz, deltaAlt);
	updateAutoZoom(deltaTime);
}


// Go and zoom to the selected object.
void MovementMgr::autoZoomIn(float moveDuration, bool allowManualZoom)
{
	Projector* proj = core->getProjection();
	
	if (!StelApp::getInstance().getStelObjectMgr().getWasSelected())
		return;
		
	float manualMoveDuration;

	if (!getFlagTracking())
	{
		setFlagTracking(true);
		moveTo(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getObsEquatorialPos(core->getNavigation()), moveDuration, false, 1);
		manualMoveDuration = moveDuration;
	}
	else
	{
		// faster zoom in manual zoom mode once object is centered
		manualMoveDuration = moveDuration*.66f;
	}

	if( allowManualZoom && flagManualZoom )
	{
		// if manual zoom mode, user can zoom in incrementally
		float newfov = proj->getFov()*0.5f;
		zoomTo(newfov, manualMoveDuration);
	}
	else
	{
		float satfov = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getSatellitesFov(core->getNavigation());

		if (satfov>0.0 && proj->getFov()*0.9>satfov)
			zoomTo(satfov, moveDuration);
		else
		{
			float closefov = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getCloseViewFov(core->getNavigation());
			if (proj->getFov()>closefov)
				zoomTo(closefov, moveDuration);
		}
	}
}


// Unzoom and go to the init position
void MovementMgr::autoZoomOut(float moveDuration, bool full)
{
	Navigator* nav = core->getNavigation();
	Projector* proj = core->getProjection();
	
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected() && !full)
	{
		// If the selected object has satellites, unzoom to satellites view
		// unless specified otherwise
		float satfov = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getSatellitesFov(core->getNavigation());

		if (satfov>0.0 && proj->getFov()<=satfov*0.9)
		{
			zoomTo(satfov, moveDuration);
			return;
		}

		// If the selected object is part of a Planet subsystem (other than sun),
		// unzoom to subsystem view
		satfov = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getParentSatellitesFov((core->getNavigation()));
		if (satfov>0.0 && proj->getFov()<=satfov*0.9)
		{
			zoomTo(satfov, moveDuration);
			return;
		}
	}

	zoomTo(proj->getInitFov(), moveDuration);
	if (flagAutoZoomOutResetsDirection) 
		moveTo(nav->getInitViewingDirection(), moveDuration, true, -1);
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
		moveTo(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getObsEquatorialPos(core->getNavigation()), getAutoMoveDuration());
		flagTracking=true;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Move to the given equatorial position
void MovementMgr::moveTo(const Vec3d& _aim, float moveDuration, bool _localPos, int zooming)
{
	Navigator* nav = core->getNavigation();
	zoomingMode = zooming;
	move.aim=_aim;
	move.aim.normalize();
	move.aim*=2.;
	if (_localPos)
	{
		move.start=nav->getLocalVision();
	}
	else
	{
		move.start=nav->getEquVision();
	}
	move.start.normalize();
	move.speed=1.f/(moveDuration*1000);
	move.coef=0.;
	move.localPos = _localPos;
	flagAutoMove = true;
}


////////////////////////////////////////////////////////////////////////////////
void MovementMgr::updateVisionVector(double deltaTime)
{
	Navigator* nav = core->getNavigation();
	if (flagAutoMove)
	{
		double ra_aim, de_aim, ra_start, de_start, ra_now, de_now;

		if( zoomingMode == 1 && StelApp::getInstance().getStelObjectMgr().getWasSelected())
		{
			// if zooming in, object may be moving so be sure to zoom to latest position
			move.aim = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getObsEquatorialPos(core->getNavigation());
			move.aim.normalize();
			move.aim*=2.;
		}

		// Use a smooth function
		float smooth = 4.f;
		double c;

		if (zoomingMode == 1)
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
		else if(zoomingMode == -1)
		{
			if( move.coef < 0.1 )
			{
				// keep in view at first as zoom out
				c = 0;

				/* could track as moves too, but would need to know if start was actually
				   a zoomed in view on the object or an extraneous zoom out command
				   if(move.localPos) {
				   move.start=earthEquToLocal(selected.getObsEquatorialPos(this));
				   } else {
				   move.start=selected.getObsEquatorialPos(this);
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


		if (move.localPos)
		{
			StelUtils::rectToSphe(&ra_aim, &de_aim, move.aim);
			StelUtils::rectToSphe(&ra_start, &de_start, move.start);
		}
		else
		{
			StelUtils::rectToSphe(&ra_aim, &de_aim, nav->earthEquToLocal(move.aim));
			StelUtils::rectToSphe(&ra_start, &de_start, nav->earthEquToLocal(move.start));
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
		StelUtils::spheToRect(ra_now, de_now, tmp);
		nav->setEquVision(nav->localToEarthEqu(tmp));

		move.coef+=move.speed*deltaTime*1000;
		if (move.coef>=1.)
		{
			flagAutoMove=0;
			if (move.localPos)
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
			nav->setEquVision(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getObsEquatorialPos(core->getNavigation()));
		}
		else
		{
			if (flagLockEquPos) // Equatorial vision vector locked
			{
				// Recalc local vision vector
				nav->setLocalVision(nav->earthEquToLocal(nav->getEquVision()));
			}
			else // Local vision vector locked
			{
				// Recalc equatorial vision vector
				nav->setEquVision(nav->localToEarthEqu(nav->getLocalVision()));
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
void MovementMgr::panView(double deltaAz, double deltaAlt)
{
	Navigator* nav = core->getNavigation();
	double azVision, altVision;

	if( nav->getViewingMode() == Navigator::ViewEquator) StelUtils::rectToSphe(&azVision,&altVision,nav->getEquVision());
	else StelUtils::rectToSphe(&azVision,&altVision,nav->getLocalVision());

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
		if( nav->getViewingMode() == Navigator::ViewEquator)
		{
			Vec3d tmp;
			StelUtils::spheToRect(azVision, altVision, tmp);
			nav->setLocalVision(nav->earthEquToLocal(tmp));
		}
		else
		{
			Vec3d tmp;
			StelUtils::spheToRect(azVision, altVision, tmp);
			// Calc the equatorial coordinate of the direction of vision wich was in Altazimuthal coordinate
			nav->setEquVision(nav->localToEarthEqu(tmp));
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
	if (nav->getViewingMode()==Navigator::ViewHorizon)
		proj->setCurrentFrame(Projector::FrameLocal);
	else
		proj->setCurrentFrame(Projector::FrameEarthEqu);
		
//johannes: StelApp already gives appropriate x/y coordinates
//	proj->unProject(x2,proj->getViewportHeight()-y2, tempvec2);
//	proj->unProject(x1,proj->getViewportHeight()-y1, tempvec1);
	proj->unProject(x2,y2, tempvec2);
	proj->unProject(x1,y1, tempvec1);
	StelUtils::rectToSphe(&az1, &alt1, tempvec1);
	StelUtils::rectToSphe(&az2, &alt2, tempvec2);
	panView(az2-az1, alt1-alt2);
	setFlagTracking(false);
	setFlagLockEquPos(false);
}


// Update autoZoom if activated
void MovementMgr::updateAutoZoom(double deltaTime)
{
	if (flagAutoZoom)
	{
		Projector* proj = core->getProjection();
		
		// Use a smooth function
		double c;

		if( zoomMove.start > zoomMove.aim )
		{
			// slow down as approach final view
			c = 1 - (1-zoomMove.coef)*(1-zoomMove.coef)*(1-zoomMove.coef);
		}
		else
		{
			// speed up as leave zoom target
			c = (zoomMove.coef)*(zoomMove.coef)*(zoomMove.coef);
		}

		proj->setFov(zoomMove.start + (zoomMove.aim - zoomMove.start) * c);
		zoomMove.coef+=zoomMove.speed*deltaTime*1000;
		if (zoomMove.coef>=1.)
		{
			flagAutoZoom = 0;
			proj->setFov(zoomMove.aim);
		}
	}
}

// Zoom to the given field of view
void MovementMgr::zoomTo(double aim_fov, float moveDuration)
{
	zoomMove.aim=aim_fov;
    zoomMove.start=core->getProjection()->getFov();
    zoomMove.speed=1.f/(moveDuration*1000);
    zoomMove.coef=0.;
    flagAutoZoom = true;
}

void MovementMgr::changeFov(double deltaFov)
{
	// if we are zooming in or out
	if (deltaFov) core->getProjection()->setFov(core->getProjection()->getFov()+deltaFov);
}

double MovementMgr::getAimFov(void) const
{
	return (flagAutoZoom ? zoomMove.aim : core->getProjection()->getFov());
}
