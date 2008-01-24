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

#ifndef MOVEMENTMGR_H_
#define MOVEMENTMGR_H_

#include "StelModule.hpp"
#include "Projector.hpp"

//! @class MovementMgr
//! Manages the movement and zoomer operations.
class MovementMgr : public StelModule
{
	Q_OBJECT

public:
	MovementMgr(StelCore* core);
	virtual ~MovementMgr();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initializes the object based on the application settings
	//! Includes:
	//! - Enabling/disabling the movement keys
	//! - Enabling/disabling the zoom keys
	//! - Enabling/disabling the mouse zoom
	//! - Enabling/disabling the mouse movement
	//! - Sets the zoom and movement speeds
	//! - Sets the auto-zoom duration and mode.
	virtual void init();
	
	//! Update time-dependent things (does nothing).
	virtual void update(double deltaTime) {;}
	//! Implement required draw function.  Does nothing.
	virtual double draw(StelCore* core) {return 0.;}
	//! Handle keyboard events.
	virtual bool handleKeys(StelKey key, StelMod mod, Uint16 unicode, Uint8 state);
	//! Handle mouse movement events.
	virtual bool handleMouseMoves(Uint16 x, Uint16 y, StelMod mod);
	//! Handle mouse click events.
	virtual bool handleMouseClicks(Uint16 x, Uint16 y, Uint8 button, Uint8 state, StelMod mod);
	//! Called then the selected object changes.
	virtual void selectedObjectChangeCallBack(StelModuleSelectAction action=StelModule::REPLACE_SELECTION);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to MovementMgr
	
	//! Increment/decrement smoothly the vision field and position.
	void updateMotion(double deltaTime);
	
	// These are hopefully temporary.
	bool getHasDragged() const {return has_dragged;}
	void stopDragging() {is_dragging=false; has_dragged=false;}
	
	
	//! Get the zoom speed
	// TODO: what are the units?
	double getZoomSpeed() {return zoom_speed;}
	
public slots:
	//! Set whether auto zoom can go further than normal.
	void setFlagManualAutoZoom(bool b) {FlagManualZoom = b;}
	//! Get whether auto zoom can go further than normal.
	bool getFlagManualAutoZoom(void) {return FlagManualZoom;}
	
	//! Set object tracking on/off.
	void setFlagTracking(bool b);
	//! Get current object tracking status.
	bool getFlagTracking(void) const {return flagTracking;}

	//! Set whether sky position is to be locked.
	void setFlagLockEquPos(bool b) {flag_lock_equ_pos=b;}
	//! Get whether sky position is locked.
	bool getFlagLockEquPos(void) const {return flag_lock_equ_pos;}
	
	//! Move view in alt/az (or equatorial if in that mode) coordinates.
	void panView(double deltaAz, double deltaAlt);

	//! Set automove duration in seconds
	//! @param f the number of seconds it takes for an auto-move operation to complete.
	void setAutoMoveDuration(float f) {auto_move_duration = f;}
	//! Get automove duration in seconds
	//! @return the number of seconds it takes for an auto-move operation to complete.
	float getAutoMoveDuration(void) const {return auto_move_duration;}

	//! Move the view to a specified position.
	//! Uses equatorial or local coordinate depending on _local_pos value.
	//! @param aim The position to move to expressed as a vector.
	//! @param moveDuration The time it takes for the move to complete.
	//! @param localPos If false, use equatorial position, else use local.
	//! @param zooming ???
	void moveTo(const Vec3d& aim, float moveDuration = 1., bool localPos = false, int zooming = 0);

	//! Change the zoom level.
	//! @param aimFov The desired field of view in degrees.
	//! @param moveDuration The time that the operation should take to complete.
	void zoomTo(double aimFov, float moveDuration = 1.);

	//! Go and zoom to the selected object. A later call to autoZoomOut will come back to the previous zoom level.
	void autoZoomIn(float move_duration = 1.f, bool allow_manual_zoom = 1);
	//! Unzoom to the previous position.
	void autoZoomOut(float move_duration = 1.f, bool full = 0);

	//! If currently zooming, return the target FOV, otherwise return current FOV in degree.
	double getAimFov(void) const;

private:
	// Viewing direction function : 1 move, 0 stop.
	void turn_right(int);
	void turn_left(int);
	void turn_up(int);
	void turn_down(int);
	void zoom_in(int);
	void zoom_out(int);
	void change_fov(double deltaFov);
	   
	void updateVisionVector(double deltaTime); 
	void update_auto_zoom(double deltaTime); // Update auto_zoom if activated
	
	//! Make the first screen position correspond to the second (useful for mouse dragging)
	void dragView(int x1, int y1, int x2, int y2);
	
	StelCore* core;	// The core on which the movement are applied

	bool flag_lock_equ_pos;			// Define if the equatorial position is locked

	bool flagTracking;			// Define if the selected object is followed
	
	// Flags for mouse movements
	bool is_mouse_moving_horiz;
	bool is_mouse_moving_vert;

	int FlagEnableMoveMouse;  // allow mouse at edge of screen to move view
	int MouseZoom;
	
	int FlagEnableZoomKeys;
	int FlagEnableMoveKeys;

	// Automove
	// Struct used to store data for auto mov
	typedef struct
	{
		Vec3d start;
		Vec3d aim;
		float speed;
		float coef;
		bool local_pos;			// Define if the position are in equatorial or altazimutal
	} auto_move;
	
	auto_move move;				// Current auto movement
	int flag_auto_move;			// Define if automove is on or off
	int zooming_mode;			// 0 : undefined, 1 zooming, -1 unzooming

	double deltaFov,deltaAlt,deltaAz;	// View movement
	double move_speed, zoom_speed;		// Speed of movement and zooming

	int FlagManualZoom;			// Define whether auto zoom can go further
	float auto_move_duration;		// Duration of movement for the auto move to a selected objectin seconds
	
	// Mouse control options
	bool is_dragging, has_dragged;
	int previous_x, previous_y;
	
	// Struct used to store data for auto zoom
	typedef struct
	{
		double start;
		double aim;
		float speed;
		float coef;
	} auto_zoom;
	
	// Automove
	auto_zoom zoom_move;		// Current auto movement
	bool flag_auto_zoom;		// Define if autozoom is on or off
};

#endif /*MOVEMENTMGR_H_*/
