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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELMOVEMENTMGR_HPP_
#define _STELMOVEMENTMGR_HPP_

#include "StelModule.hpp"
#include "StelProjector.hpp"
#include "StelObjectType.hpp"

//! @class StelMovementMgr
//! Manages the head movements and zoom operations.
class StelMovementMgr : public StelModule
{
	Q_OBJECT

public:

	//! Possible mount modes defining the reference frame in which head movements occur.
	enum MountMode { MountAltAzimuthal, MountEquinoxEquatorial, MountGalactic};

	StelMovementMgr(StelCore* core);
	virtual ~StelMovementMgr();

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
	virtual void update(double) {;}
	//! Implement required draw function.  Does nothing.
	virtual void draw(StelCore*, class StelRenderer*) {;}
	//! Handle keyboard events.
	virtual void handleKeys(QKeyEvent* event);
	//! Handle mouse movement events.
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	//! Handle mouse wheel events.
	virtual void handleMouseWheel(class QWheelEvent* event);
	//! Handle mouse click events.
	virtual void handleMouseClicks(class QMouseEvent* event);

	///////////////////////////////////////////////////////////////////////////
	// Methods specific to StelMovementMgr

	//! Increment/decrement smoothly the vision field and position.
	void updateMotion(double deltaTime);

	// These are hopefully temporary.
	bool getHasDragged() const {return hasDragged;}

	//! Get the zoom speed
	// TODO: what are the units?
	double getZoomSpeed() {return keyZoomSpeed;}

	//! Return the current up view vector.
	Vec3d getViewUpVectorJ2000() const;
	void setViewUpVectorJ2000(const Vec3d& up);

	void setMovementSpeedFactor(float s) {movementsSpeedFactor=s;}
	float getMovementSpeedFactor() const {return movementsSpeedFactor;}

	void setDragTriggerDistance(float d) {dragTriggerDistance=d;}

public slots:
	//! Toggle current mount mode between equatorial and altazimuthal
	void toggleMountMode() {if (getMountMode()==MountAltAzimuthal) setMountMode(MountEquinoxEquatorial); else setMountMode(MountAltAzimuthal);}
	//! Define whether we should use equatorial mount or altazimuthal
	void setEquatorialMount(bool b) {setMountMode(b ? MountEquinoxEquatorial : MountAltAzimuthal);}

	//! Set object tracking on/off and go to selected object
	void setFlagTracking(bool b=true);
	//! Get current object tracking status.
	bool getFlagTracking(void) const {return flagTracking;}

	//! Set whether sky position is to be locked.
	void setFlagLockEquPos(bool b);
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

	//! Move the view to a specified J2000 position.
	//! @param aim The position to move to expressed as a vector.
	//! @param moveDuration The time it takes for the move to complete.
	//! @param zooming ???
	void moveToJ2000(const Vec3d& aim, float moveDuration = 1., int zooming = 0);
	void moveToObject(const StelObjectP& target, float moveDuration = 1., int zooming = 0);

	//! Change the zoom level.
	//! @param aimFov The desired field of view in degrees.
	//! @param moveDuration The time that the operation should take to complete.
	void zoomTo(double aimFov, float moveDuration = 1.);
	//! Get the current Field Of View in degrees
	double getCurrentFov() const {return currentFov;}

	//! Return the initial default FOV in degree.
	double getInitFov() const {return initFov;}
	//! Set the initial Field Of View in degree.
	void setInitFov(double fov) {initFov=fov;}

	//! Return the inital viewing direction in altazimuthal coordinates
	const Vec3d& getInitViewingDirection() {return initViewPos;}
	//! Sets the initial direction of view to the current altitude and azimuth.
	//! Note: Updates the configuration file.
	void setInitViewDirectionToCurrent();

	//! Return the current viewing direction in equatorial J2000 frame.
	Vec3d getViewDirectionJ2000() const {return viewDirectionJ2000;}
	void setViewDirectionJ2000(const Vec3d& v);

	//! Set the maximum field of View in degrees.
	void setMaxFov(double max);
	//! Get the maximum field of View in degrees.
	double getMaxFov(void) const {return maxFov;}

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
	void moveSlow(bool b) {flagMoveSlow=b;}
	void zoomIn(bool);
	void zoomOut(bool);

	//! Set current mount type defining the reference frame in which head movements occur.
	void setMountMode(MountMode m);
	//! Get current mount type defining the reference frame in which head movements occur.
	MountMode getMountMode(void) const {return mountMode;}

	void setDragTimeMode(bool b) {dragTimeMode=b;}
	bool getDragTimeMode() const {return dragTimeMode;}

	void setFov(double f)
	{
		currentFov = f;
		if (f>maxFov)
			currentFov = maxFov;
		if (f<minFov)
			currentFov = minFov;
	}

private slots:
	//! Called when the selected object changes.
	void selectedObjectChange(StelModule::StelModuleSelectAction action);
	
private:
	Vec3d j2000ToMountFrame(const Vec3d& v) const;
	Vec3d mountFrameToJ2000(const Vec3d& v) const;

	double currentFov; // The current FOV in degree
	double initFov;    // The FOV at startup
	double minFov;     // Minimum FOV in degree
	double maxFov;     // Maximum FOV in degree

	void changeFov(double deltaFov);

	void updateVisionVector(double deltaTime);
	void updateAutoZoom(double deltaTime); // Update autoZoom if activated

	//! Make the first screen position correspond to the second (useful for mouse dragging)
	void dragView(int x1, int y1, int x2, int y2);

	StelCore* core;          // The core on which the movement are applied
	class StelObjectMgr* objectMgr;
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
	float keyMoveSpeed;              // Speed of keys movement
	float keyZoomSpeed;              // Speed of keys zoom
	bool flagMoveSlow;

	// Speed factor for real life time movements, used for fast forward when playing scripts.
	float movementsSpeedFactor;

	//! @internal
	//! Store data for auto-move
	struct AutoMove
	{
		Vec3d start;
		Vec3d aim;
		float speed;
		float coef;
		// If not null, move to the object.
		StelObjectP targetObject;
	};

	AutoMove move;          // Current auto movement
	bool flagAutoMove;       // Define if automove is on or off
	int zoomingMode;        // 0 : undefined, 1 zooming, -1 unzooming

	double deltaFov,deltaAlt,deltaAz; // View movement

	bool flagManualZoom;     // Define whether auto zoom can go further
	float autoMoveDuration; // Duration of movement for the auto move to a selected objectin seconds

	// Mouse control options
	bool isDragging, hasDragged;
	int previousX, previousY;

	// Contains the last N real time / JD times pairs associated with the last N mouse move events
	struct DragHistoryEntry
	{
		double runTime;
		double jd;
		int x;
		int y;
	};

	QList<DragHistoryEntry> timeDragHistory;
	void addTimeDragPoint(int x, int y);
	float beforeTimeDragTimeRate;

	// Time mouse control
	bool dragTimeMode;

	//! @internal
	//! Store data for auto-zoom.
	struct AutoZoom
	{
		double start;
		double aim;
		float speed;
		float coef;
	};

	// Automove
	AutoZoom zoomMove; // Current auto movement
	bool flagAutoZoom; // Define if autozoom is on or off
	bool flagAutoZoomOutResetsDirection;

	// defines if view corrects for horizon, or uses equatorial coordinates
	MountMode mountMode;

	Vec3d initViewPos;        // Default viewing direction

	// Viewing direction in equatorial J2000 coordinates
	Vec3d viewDirectionJ2000;
	// Viewing direction in the mount reference frame.
	Vec3d viewDirectionMountFrame;

	Vec3d upVectorMountFrame;

	float dragTriggerDistance;
};

#endif // _STELMOVEMENTMGR_HPP_

