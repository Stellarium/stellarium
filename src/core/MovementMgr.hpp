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

#ifndef _MOVEMENTMGR_HPP_
#define _MOVEMENTMGR_HPP_

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
	virtual void draw(StelCore* core) {;}
	//! Handle keyboard events.
	virtual void handleKeys(QKeyEvent* event);
	//! Handle mouse movement events.
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	//! Handle mouse wheel events.
	virtual void handleMouseWheel(class QWheelEvent* event);
	//! Handle mouse click events.
	virtual void handleMouseClicks(class QMouseEvent* event);
	//! Called then the selected object changes.
	virtual void selectedObjectChangeCallBack(StelModuleSelectAction action=StelModule::ReplaceSelection);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to MovementMgr
	
	//! Increment/decrement smoothly the vision field and position.
	void updateMotion(double deltaTime);
	
	// These are hopefully temporary.
	bool getHasDragged() const {return hasDragged;}
	
	//! Get the zoom speed
	// TODO: what are the units?
	double getZoomSpeed() {return keyZoomSpeed;}
	
public slots:
	//! Set object tracking on/off and go to selected object
	void setFlagTracking(bool b=true);
	//! Get current object tracking status.
	bool getFlagTracking(void) const {return flagTracking;}

	//! Set whether sky position is to be locked.
	void setFlagLockEquPos(bool b) {flagLockEquPos=b;}
	//! Get whether sky position is locked.
	bool getFlagLockEquPos(void) const {return flagLockEquPos;}
	
	//! Move view in alt/az (or equatorial if in that mode) coordinates.
	//! Changes to viewing direction are instantaneous.
	//! @param deltaAz change in azimuth angle in radians
	//! @param deltaAlt change in altitude angle in radians
	void panView(double deltaAz, double deltaAlt);

	//! Set automove duration in seconds
	//! @param f the number of seconds it takes for an auto-move operation to complete.
	void setAutoMoveDuration(float f) {autoMoveDuration = f;}
	//! Get automove duration in seconds
	//! @return the number of seconds it takes for an auto-move operation to complete.
	float getAutoMoveDuration(void) const {return autoMoveDuration;}

	//! Set whether auto zoom out will reset the viewing direction to the inital value
	void setFlagAutoZoomOutResetsDirection(bool b) {flagAutoZoomOutResetsDirection = b;}
	//! Get whether auto zoom out will reset the viewing direction to the inital value
	bool getFlagAutoZoomOutResetsDirection(void) {return flagAutoZoomOutResetsDirection;}
	
	//! Get whether keys can control zoom
	bool getFlagEnableZoomKeys() const {return flagEnableZoomKeys;}
	//! Set whether keys can control zoom
	void setFlagEnableZoomKeys(bool b) {flagEnableZoomKeys=b;}
	
	//! Get whether keys can control move
	bool getFlagEnableMoveKeys() const {return flagEnableMoveKeys;}
	//! Set whether keys can control movement
	void setFlagEnableMoveKeys(bool b) {flagEnableMoveKeys=b;}
	
	//! Get whether being at the edge of the screen activates movement
	bool getFlagEnableMoveAtScreenEdge() const {return flagEnableMoveAtScreenEdge;}
	//! Set whether being at the edge of the screen activates movement
	void setFlagEnableMoveAtScreenEdge(bool b) {flagEnableMoveAtScreenEdge=b;}
	
	//! Get whether mouse can control movement
	bool getFlagEnableMouseNavigation() const {return flagEnableMouseNavigation;}
	//! Set whether mouse can control movement
	void setFlagEnableMouseNavigation(bool b) {flagEnableMouseNavigation=b;}
	
	//! Move the view to a specified position.
	//! Uses equatorial or local coordinate depending on _localPos value.
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
	void autoZoomIn(float moveDuration = 1.f, bool allowManualZoom = 1);
	//! Unzoom to the previous position.
	void autoZoomOut(float moveDuration = 1.f, bool full = 0);

	//! If currently zooming, return the target FOV, otherwise return current FOV in degree.
	double getAimFov(void) const;

	//! Viewing direction function : true move, false stop.
	void turnRight(bool);
	void turnLeft(bool);
	void turnUp(bool);
	void turnDown(bool);
	void zoomIn(bool);
	void zoomOut(bool);
	
private:
	
	void changeFov(double deltaFov);
	   
	void updateVisionVector(double deltaTime); 
	void updateAutoZoom(double deltaTime); // Update autoZoom if activated
	
	//! Make the first screen position correspond to the second (useful for mouse dragging)
	void dragView(int x1, int y1, int x2, int y2);
	
	StelCore* core;          // The core on which the movement are applied

	bool flagLockEquPos;     // Define if the equatorial position is locked

	bool flagTracking;       // Define if the selected object is followed
	
	// Flags for mouse movements
	bool isMouseMovingHoriz;
	bool isMouseMovingVert;

	bool flagEnableMoveAtScreenEdge; // allow mouse at edge of screen to move view
	bool flagEnableMouseNavigation;
	float mouseZoomSpeed;
	
	bool flagEnableZoomKeys;
	bool flagEnableMoveKeys;
	float keyMoveSpeed;			// Speed of keys movement
	float keyZoomSpeed;			// Speed of keys zoom
	
	// Automove
	// Struct used to store data for auto mov
	typedef struct
	{
		Vec3d start;
		Vec3d aim;
		float speed;
		float coef;
		bool localPos;  // Define if the position are in equatorial or altazimuthal
	} autoMove;
	
	autoMove move;          // Current auto movement
	int flagAutoMove;       // Define if automove is on or off
	int zoomingMode;        // 0 : undefined, 1 zooming, -1 unzooming

	double deltaFov,deltaAlt,deltaAz; // View movement

	bool flagManualZoom;     // Define whether auto zoom can go further
	float autoMoveDuration; // Duration of movement for the auto move to a selected objectin seconds
	
	// Mouse control options
	bool isDragging, hasDragged;
	int previousX, previousY;
	
	// Struct used to store data for auto zoom
	typedef struct
	{
		double start;
		double aim;
		float speed;
		float coef;
	} autoZoom;
	
	// Automove
	autoZoom zoomMove; // Current auto movement
	bool flagAutoZoom; // Define if autozoom is on or off
	bool flagAutoZoomOutResetsDirection;
};

#endif // _MOVEMENTMGR_HPP_

