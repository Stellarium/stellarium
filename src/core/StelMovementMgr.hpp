/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
 * Copyright (C) 2015 Georg Zotti (offset view adaptations, Up vector fix for zenithal views)
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

#ifndef STELMOVEMENTMGR_HPP
#define STELMOVEMENTMGR_HPP

#include "StelModule.hpp"
#include "StelProjector.hpp"
#include "StelObjectType.hpp"
#include <QTimeLine>
#include <QTimer>
#include <QCursor>
#include <QEasingCurve>

//! @class Smoother
//! Compute smooth animation for a given float value.
//! Used to smooth out the fov animations.
class Smoother
{
public:
	double getValue() const;
	double getAim() const { return aim; }
	void setTarget(double start, double aim, double duration);
	void update(double dt);
	bool finished() const;

private:
	QEasingCurve easingCurve;
	double start;
	double aim;
	double duration;
	double progress;
};

//! @class StelMovementMgr
//! Manages the head movements and zoom operations.
class StelMovementMgr : public StelModule
{
	Q_OBJECT
	Q_ENUMS(MountMode)
	Q_ENUMS(ZoomingMode)
	Q_PROPERTY(bool equatorialMount
		   READ getEquatorialMount
		   WRITE setEquatorialMount
		   NOTIFY equatorialMountChanged)
	Q_PROPERTY(bool tracking
		   READ getFlagTracking
		   WRITE setFlagTracking
		   NOTIFY flagTrackingChanged)
	Q_PROPERTY(bool flagIndicationMountMode
		   READ getFlagIndicationMountMode
		   WRITE setFlagIndicationMountMode
		   NOTIFY flagIndicationMountModeChanged)
	//The targets of viewport offset animation
	Q_PROPERTY(double viewportHorizontalOffsetTarget
		   READ getViewportHorizontalOffsetTarget
		   WRITE setViewportHorizontalOffsetTarget
		   NOTIFY viewportHorizontalOffsetTargetChanged)
	Q_PROPERTY(double viewportVerticalOffsetTarget
		   READ getViewportVerticalOffsetTarget
		   WRITE setViewportVerticalOffsetTarget
		   NOTIFY viewportVerticalOffsetTargetChanged)
	Q_PROPERTY(bool flagAutoZoomOutResetsDirection
		   READ getFlagAutoZoomOutResetsDirection
		   WRITE setFlagAutoZoomOutResetsDirection
		   NOTIFY flagAutoZoomOutResetsDirectionChanged)
	Q_PROPERTY(bool flagEnableMouseNavigation
		   READ getFlagEnableMouseNavigation
		   WRITE setFlagEnableMouseNavigation
		   NOTIFY flagEnableMouseNavigationChanged)
	Q_PROPERTY(bool flagEnableMoveKeys
		   READ getFlagEnableMoveKeys
		   WRITE setFlagEnableMoveKeys
		   NOTIFY flagEnableMoveKeysChanged)
	Q_PROPERTY(bool flagEnableZoomKeys
		   READ getFlagEnableZoomKeys
		   WRITE setFlagEnableZoomKeys
		   NOTIFY flagEnableZoomKeysChanged)
public:
	//! Possible mount modes defining the reference frame in which head movements occur.
	//! MountGalactic and MountSupergalactic is currently only available via scripting API: core.clear("galactic") and core.clear("supergalactic")
	// TODO: add others: MountEcliptical, MountEq2000, MountEcliptical2000 and implement proper variants.
	enum MountMode { MountAltAzimuthal, MountEquinoxEquatorial, MountGalactic, MountSupergalactic};

	//! Named constants for zoom operations.
	enum ZoomingMode { ZoomOut=-1, ZoomNone=0, ZoomIn=1};

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

	//! Update time-dependent things (triggers a time dragging record if required)
	virtual void update(double)
	{
		if (dragTimeMode)
			addTimeDragPoint(QCursor::pos().x(), QCursor::pos().y());
	}
	//! Implement required draw function.  Does nothing.
	virtual void draw(StelCore*) {;}
	//! Handle keyboard events.
	virtual void handleKeys(QKeyEvent* event);
	//! Handle mouse movement events.
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	//! Handle mouse wheel events.
	virtual void handleMouseWheel(class QWheelEvent* event);
	//! Handle mouse click events.
	virtual void handleMouseClicks(class QMouseEvent* event);
	// allow some keypress interaction by plugins.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	//! Handle pinch gesture.
	virtual bool handlePinch(qreal scale, bool started);

	///////////////////////////////////////////////////////////////////////////
	// Methods specific to StelMovementMgr

	//! Increment/decrement smoothly the vision field and position. Called in StelCore.update().
	void updateMotion(double deltaTime);

	//! Get the zoom speed
	// TODO: what are the units?
	double getZoomSpeed() const {return static_cast<double>(keyZoomSpeed);}

	//! Return the current up view vector in J2000 coordinates.
	Vec3d getViewUpVectorJ2000() const;
	// You can set an upVector in J2000 coordinates which is translated to current mount mode. Important when viewing into the pole of the current mount mode coordinates.
	void setViewUpVectorJ2000(const Vec3d& up);
	// Set vector directly. This is set in the current mountmode, but will be translated to J2000 internally
	// We need this only when viewing to the poles of current coordinate system where the view vector would else be parallel to the up vector.
	void setViewUpVector(const Vec3d& up);

	void setMovementSpeedFactor(float s) {movementsSpeedFactor=s;}
	float getMovementSpeedFactor() const {return movementsSpeedFactor;}

	void setDragTriggerDistance(float d) {dragTriggerDistance=d;}

	Vec3d j2000ToMountFrame(const Vec3d& v) const;
	Vec3d mountFrameToJ2000(const Vec3d& v) const;

	void moveToObject(const StelObjectP& target, float moveDuration = 1., ZoomingMode zooming = ZoomNone);

public slots:
	// UNUSED!
	//! Toggle current mount mode between equatorial and altazimuthal
	void toggleMountMode() {if (getMountMode()==MountAltAzimuthal) setMountMode(MountEquinoxEquatorial); else setMountMode(MountAltAzimuthal);}
	//! Define whether we should use equatorial mount or altazimuthal
	void setEquatorialMount(bool b);

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
	void panView(const double deltaAz, const double deltaAlt);

	//! Set automove duration in seconds
	//! @param f the number of seconds it takes for an auto-move operation to complete.
	void setAutoMoveDuration(float f) {autoMoveDuration = f;}
	//! Get automove duration in seconds
	//! @return the number of seconds it takes for an auto-move operation to complete.
	float getAutoMoveDuration(void) const {return autoMoveDuration;}

	//! Set whether auto zoom out will reset the viewing direction to the inital value
	void setFlagAutoZoomOutResetsDirection(bool b) {if (flagAutoZoomOutResetsDirection != b) { flagAutoZoomOutResetsDirection = b; emit flagAutoZoomOutResetsDirectionChanged(b);}}
	//! Get whether auto zoom out will reset the viewing direction to the inital value
	bool getFlagAutoZoomOutResetsDirection(void) const {return flagAutoZoomOutResetsDirection;}

	//! Get whether keys can control zoom
	bool getFlagEnableZoomKeys() const {return flagEnableZoomKeys;}
	//! Set whether keys can control zoom
	void setFlagEnableZoomKeys(bool b) {flagEnableZoomKeys=b; emit flagEnableZoomKeysChanged(b);}

	//! Get whether keys can control movement
	bool getFlagEnableMoveKeys() const {return flagEnableMoveKeys;}
	//! Set whether keys can control movement
	void setFlagEnableMoveKeys(bool b) {flagEnableMoveKeys=b; emit flagEnableMoveKeysChanged(b); }

	//! Get whether being at the edge of the screen activates movement
	bool getFlagEnableMoveAtScreenEdge() const {return flagEnableMoveAtScreenEdge;}
	//! Set whether being at the edge of the screen activates movement
	void setFlagEnableMoveAtScreenEdge(bool b) {flagEnableMoveAtScreenEdge=b;}

	//! Get whether mouse can control movement
	bool getFlagEnableMouseNavigation() const {return flagEnableMouseNavigation;}
	//! Set whether mouse can control movement
	void setFlagEnableMouseNavigation(bool b) {flagEnableMouseNavigation=b; emit flagEnableMouseNavigationChanged(b); }

	//! Get the state of flag for indication of mount mode
	bool getFlagIndicationMountMode() const {return flagIndicationMountMode;}
	//! Set the state of flag for indication of mount mode
	void setFlagIndicationMountMode(bool b) { flagIndicationMountMode=b; emit flagIndicationMountModeChanged(b); }

	//! Move the view to a specified J2000 position.
	//! @param aim The position to move to expressed as a vector.
	//! @param aimUp Up vector. Can be usually (0/0/1) but may have to be exact for looking into the zenith/pole
	//! @param moveDuration The time it takes for the move to complete.
	//! @param zooming you want to zoom in, out or not (just center).
	//! @code
	//! // You can use the following code most of the times to find a valid aimUp vector:
	//! StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	//! mvmgr->moveToJ2000(pos, mvmgr->mountFrameToJ2000(Vec3d(0., 0., 1.)), mvmgr->getAutoMoveDuration());
	//! @endcode
	//! @note core::moveToRaDecJ2000 provides a simpler signature for the same function.
	//! @note Objects of class Vec3d are 3-dimensional vectors in a rectangular coordinate system. For
	//!       J2000 positions, the x-axis points to 0h,0°, the y-axis to 6h,0° and the z-axis points to the
	//!       celestial pole. You may use a constructor defining three components (x,y,z) or the
	//!       format with just two angles, e.g., Vec3d("0h","0d").
	void moveToJ2000(const Vec3d& aim, const Vec3d &aimUp, float moveDuration = 1., ZoomingMode zooming = ZoomNone);

	//! Move the view to a specified AltAzimuthal position.
	//! @param aim The position to move to expressed as a vector in AltAz frame.
	//! @param aimUp Up vector in AltAz coordinates. Can be usually (0/0/1) but may have to be exact for looking into the zenith/pole
	//! @param moveDuration The time it takes for the move to complete.
	//! @param zooming you want to zoom in, out or not (just center).
	//! @code
	//! // You can use the following code most of the times to find a valid aimUp vector:
	//! StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	//! mvmgr->moveToAltAzi(pos, Vec3d(0., 0., 1.), mvmgr->getAutoMoveDuration());
	//! @endcode
	//! @note core::moveToAltAzi provides a simpler signature for the same function.
	//! @note Objects of class Vec3d are 3-dimensional vectors in a right-handed (!) rectangular coordinate system.
	//!       For positions in the horizontal coordinate system, the axes point south, east and to the
	//!       zenith, irrespective of the setting of the "Azimuth from south" option in the "Tools" tab of the
	//!       "Configuration" window. You may use a constructor defining three components (x,y,z) or the
	//!       format with just two angles, e.g., Vec3d("0d","0d") points south, Vec3d("90d","0d") points east,
	//!       with azimuth angles running counter-clockwise, i.e., against the usual orientation.
	//! @note Panic function made March 2016. It turned out that using moveToJ2000 for alt-az-based moves behaves odd for long moves during fast timelapse: end vector is linked to the sky!
	//! As of March 2016: This call does nothing when mount frame is not AltAzi!
	void moveToAltAzi(const Vec3d& aim, const Vec3d &aimUp, float moveDuration = 1.f, ZoomingMode zooming = ZoomNone);

	//! Change the zoom level.
	//! @param aimFov The desired field of view in degrees.
	//! @param zoomDuration The time that the operation should take to complete. [seconds]
	void zoomTo(double aimFov, float zoomDuration = 1.f);
	//! Get the current Field Of View in degrees
	double getCurrentFov() const {return currentFov;}

	//! Return the initial default FOV in degree.
	double getInitFov() const {return initFov;}
	//! Set the initial Field Of View in degree.
	void setInitFov(double fov);

	//! Return the initial viewing direction in altazimuthal coordinates.
	//! See StelMovementMgr::moveToAltAzi for an explanation of the return value.
	const Vec3d getInitViewingDirection() const {return initViewPos;}
	//! Sets the initial direction of view to the current altitude and azimuth.
	//! Note: Updates the configuration file.
	void setInitViewDirectionToCurrent();

	//! Return the current viewing direction in the equatorial J2000 frame.
	//! See StelMovementMgr::moveToJ2000 for an explanation of the return value.
	Vec3d getViewDirectionJ2000() const {return viewDirectionJ2000;}
	//! Set the current viewing direction in the equatorial J2000 frame.
	void setViewDirectionJ2000(const Vec3d& v);

	//! Set the maximum field of View in degrees.
	void setMaxFov(double max);
	//! Get the maximum field of View in degrees.
	double getMaxFov(void) const {return maxFov;}

	//! Get the minimum field of View in degrees.
	double getMinFov(void) const {return minFov;}

	//! Go and zoom to the selected object. A later call to autoZoomOut will come back to the previous zoom level.
	void autoZoomIn(float moveDuration = 1.f, bool allowManualZoom = 1);
	//! Unzoom to the previous position.
	void autoZoomOut(float moveDuration = 1.f, bool full = 0);

	//! Deselect the selected object
	void deselection(void);

	//! If currently zooming, return the target FOV, otherwise return current FOV in degree.
	double getAimFov(void) const;

	//! With true, starts turning the direction of view to the right, with an unspecified speed, according to the
	//! current mount mode (i.e., increasing azimuth, decreasing rectascension). Turning stops only
	//! due to a call to turnRight with false (or to turnLeft with any value) ; it does not stop when the script
	//! is terminated.
	//! @param s - true move, false stop
	//! @code
	//! // You can use the following code to turn the direction of the view
	//! // "a little" to the right, by an un predictable amount.
	//! StelMovementMgr.turnRight(true);
	//! core.wait(0.42);
	//! StelMovementMgr.turnRight(false);
	//! @endcode
	//! @note Use StelMovementMgr.panView for precise control of view movements.
	void turnRight(bool s);

	//! With true, starts turning the direction of view to the left, with an unspecified speed, and according to the
	//! current mount mode (i.e., decreasing azimuth, increasing rectascension). Turning stops only
	//! due to a call to turnLeft with false (or to turnRight with any value); it does not stop when the script
	//! is terminated.
	//! @param s - true move, false stop
	//! @code
	//! // You can use the following code to turn the direction of the view
	//! // "a little" to the left, by an unpredictable amount.
	//! StelMovementMgr.turnLeft(true);
	//! core.wait(0.42);
	//! StelMovementMgr.turnLeft(false);
	//! @endcode
	//! @note Use StelMovementMgr.panView for precise control of view movements.
	void turnLeft(bool s);

	//! With true, starts moving the direction of the view up, with an unspecified speed, and according to the
	//! current mount mode (i.e., towards the zenith or the celestial north pole). Movement halts when the
	//! goal is reached, but the command remains active until turnUp is called with false, or turnDown with
	//! any value. While this command is active, other movement commands or mouse or keyboard operations will be
	//! countermanded by the still pending turnUp command.
	//! @param s - true move, false stop
	//! @note Use StelMovementMgr.panView for precise control of view movements.
	void turnUp(bool s);
	
	//! With true, starts moving the direction of the view down, with an unspecified speed, and according to the
	//! current mount mode (i.e., towards the nadir or the celestial south pole). Movement halts when the
	//! goal is reached, but the command remains active until turnDown is called with false, or turnUp with
	//! any value. While this command is active, other movement commands or mouse or keyboard operations will be
	//! countermanded by the still pending turnDown command.
	//! @param s - true move, false stop
	//! @note Use StelMovementMgr.panView for precise control of view movements.
	void turnDown(bool s);
	
	void moveSlow(bool b) {flagMoveSlow=b;}

	//! With true, starts zooming in, with an unspecified ratio of degrees per second, either until zooming
	//! is stopped with a zoomIn call with false (or a zoomOut call). Zooming pauses when the field of view limit
	//! (5 arc seconds) is reached, but the command remains active until zoomIn is called with false, or zoomOut
	//! with any value. While this command is active, other movement commands or mouse or keyboard operations
	//! will be countermanded by the still pending zoomIn command.
	//! @param s - true zoom, false stop
	void zoomIn(bool s);

	//! With true, starts zooming out, with an unspecified ratio of degrees per second, either until zooming
	//! is stopped with a zoomIn call with false (or a zoomOut call). Zooming pauses when the field of view limit
	//! (235 degrees) is reached, but the command remains active until zoomOut is called with false, or zoomIn
	//! with any value. While this command is active, other movement commands or mouse or keyboard operations
	//! will be countermanded by the still pending zoomOut command.
	//! @param s - true zoom, false stop
	void zoomOut(bool s);

	//! Look immediately towards East.
	//! @param zero true to center on horizon, false to keep altitude, or when looking to the zenith already, turn eastern horizon to screen bottom.
	void lookEast(bool zero=false);
	//! Look immediately towards West.
	//! @param zero true to center on horizon, false to keep altitude, or when looking to the zenith already, turn western horizon to screen bottom.
	void lookWest(bool zero=false);
	//! Look immediately towards North.
	//! @param zero true to center on horizon, false to keep altitude, or when looking to the zenith already, turn northern horizon to screen bottom.
	void lookNorth(bool zero=false);
	//! Look immediately towards South.
	//! @param zero true to center on horizon, false to keep altitude, or when looking to the zenith already, turn southern horizon to screen bottom.
	void lookSouth(bool zero=false);
	//! Look immediately towards Zenith, turning southern horizon to screen bottom.
	void lookZenith(void);
	//! Look immediately towards Nadir, turning southern horizon to screen top.
	void lookNadir(void);
	//! Look immediately towards North Celestial pole.
	void lookTowardsNCP(void);
	//! Look immediately towards South Celestial pole.
	void lookTowardsSCP(void);

	//! set or start animated move of the viewport offset.
	//! This can be used e.g. in wide cylindrical panorama screens to push the horizon down and see more of the sky.
	//! Also helpful in stereographic projection to push the horizon down and see more of the sky.
	//! @param offsetX new horizontal viewport offset, percent. clamped to [-50...50]. Probably not very useful.
	//! @param offsetY new horizontal viewport offset, percent. clamped to [-50...50]. This is also available in the GUI.
	//! @param duration animation duration, seconds. Optional.
	//! @note Only vertical viewport is really meaningful.
	void moveViewport(double offsetX, double offsetY, const float duration=0.f);

	//! Set current mount type defining the reference frame in which head movements occur.
	void setMountMode(MountMode m);
	//! Get current mount type defining the reference frame in which head movements occur.
	MountMode getMountMode(void) const {return mountMode;}
	bool getEquatorialMount(void) const {return mountMode == MountEquinoxEquatorial;}

	//! Function designed only for scripting context. Put the function into the startup.ssc of your planetarium setup,
	//! this will avoid any unwanted tracking.
	void setInhibitAllAutomoves(bool inhibit) { flagInhibitAllAutomoves=inhibit;}

	//! Returns the targetted value of the viewport offset
	Vec2d getViewportOffsetTarget() const { return targetViewportOffset; }
	double getViewportHorizontalOffsetTarget() const { return targetViewportOffset[0]; }
	double getViewportVerticalOffsetTarget() const { return targetViewportOffset[1]; }

	void setViewportHorizontalOffsetTarget(double f) { moveViewport(f,getViewportVerticalOffsetTarget()); }
	void setViewportVerticalOffsetTarget(double f) { moveViewport(getViewportHorizontalOffsetTarget(),f); }

signals:
	//! Emitted when the tracking property changes
	void flagTrackingChanged(bool b);
	void equatorialMountChanged(bool b);
	void flagIndicationMountModeChanged(bool b);
	void flagAutoZoomOutResetsDirectionChanged(bool b);
	void viewportHorizontalOffsetTargetChanged(double f);
	void viewportVerticalOffsetTargetChanged(double f);
	void flagEnableMouseNavigationChanged(bool b);
	void flagEnableMoveKeysChanged(bool b);
	void flagEnableZoomKeysChanged(bool b);

private slots:
	//! Called when the selected object changes.
	void selectedObjectChange(StelModule::StelModuleSelectAction action);

	//! Connected to the viewportOffsetTimeLine, does the actual viewport shift.
	void handleViewportOffsetMovement(qreal value);

	void setFOVDeg(float fov);
	void bindingFOVActions();

private:
	double currentFov; // The current FOV in degrees
	double initFov;    // The FOV at startup
	double minFov;     // Minimum FOV in degrees
	double maxFov;     // Maximum FOV in degrees
	double deltaFov;   // requested change of FOV (degrees) used during zooming.
	void setFov(double f)
	{
		currentFov=qBound(minFov, f, maxFov);
	}
	// immediately add deltaFov argument to FOV - does not change private var.
	void changeFov(double deltaFov);

	// Move (a bit) to selected/tracked object until move.coef reaches 1, or auto-follow (track) selected object.
	// Does nothing if flagInhibitAllAutomoves=true
	void updateVisionVector(double deltaTime);
	void updateAutoZoom(double deltaTime); // Update autoZoom if activated

	//! Make the first screen position correspond to the second (useful for mouse dragging and also time dragging.)
	void dragView(int x1, int y1, int x2, int y2);

	StelCore* core;          // The core on which the movement are applied
	QSettings* conf;
	class StelObjectMgr* objectMgr;
	bool flagLockEquPos;     // Define if the equatorial position is locked
	bool flagTracking;       // Define if the selected object is followed
	bool flagInhibitAllAutomoves; // Required for special installations: If true, there is no automatic centering etc.

	// Flags for mouse movements
	bool isMouseMovingHoriz;
	bool isMouseMovingVert;

	bool flagEnableMoveAtScreenEdge; // allow mouse at edge of screen to move view
	bool flagEnableMouseNavigation;
	double mouseZoomSpeed;

	bool flagEnableZoomKeys;
	bool flagEnableMoveKeys;
	double keyMoveSpeed;              // Speed of keys movement
	double keyZoomSpeed;              // Speed of keys zoom
	bool flagMoveSlow;

	// Speed factor for real life time movements, used for fast forward when playing scripts.
	float movementsSpeedFactor;

	//! @internal
	//! Store data for auto-move
	struct AutoMove
	{
		Vec3d start;
		Vec3d aim;
		Vec3d startUp; // The Up vector at start time
		Vec3d aimUp;   // The Up vector at end time of move
		float speed;   // set to 1/duration[ms] during automove setup.
		float coef;    // Set to 0 at begin of an automove, runs up to 1.
		// If not null, move to the object instead of the aim.
		StelObjectP targetObject;
		MountMode mountMode; // In which frame we shall move. This may be different from the frame the display is set to!
		// The start and aim vectors are given in those coordinates, and are interpolated in the respective reference frames,
		// then the view vector is derived from the current coef.
		// AzAlt moves should be set to AltAz mode, else they will move towards the RA/Dec at begin of move which may have moved.
		// It is an error to move in J2000 or Eq frame with fast timelapse!
		// This is a March 2016 GZ hack. TODO: This class should be thought over a bit.
	};

	AutoMove move;          // Current auto movement. 2016-03: During setup, decide upon the frame for motion!
	bool flagAutoMove;       // Define if automove is on or off
	ZoomingMode zoomingMode;

	double deltaAlt,deltaAz; // View movement

	bool flagManualZoom;     // Define whether auto zoom can go further
	float autoMoveDuration; // Duration of movement for the auto move to a selected object in seconds

	// Mouse control options
	bool isDragging, hasDragged;
	int previousX, previousY;

	// Contains the last N real time / JD times pairs associated with the last N mouse move events at screen coordinates x/y
	struct DragHistoryEntry
	{
		double runTime;
		double jd;
		int x;
		int y;
	};
	QList<DragHistoryEntry> timeDragHistory; // list of max 3 entries.
	void addTimeDragPoint(int x, int y);
	double beforeTimeDragTimeRate;

	// Time mouse control
	bool dragTimeMode; // Internal flag, true during mouse time motion. This is set true when mouse is moving with ctrl pressed. Set false when releasing ctrl.

	// Internal state for smooth zoom animation.
	Smoother zoomMove;

	bool flagAutoZoom; // Define if autozoom is on or off
	bool flagAutoZoomOutResetsDirection;

	// defines if view corrects for horizon, or uses equatorial coordinates
	MountMode mountMode;

	Vec3d initViewPos;        // Default viewing direction
	Vec3d initViewUp;         // original up vector. Usually 0/0/1, but maybe something else in rare setups (e.g. Planetarium dome upwards fisheye projection).

	// Viewing direction in equatorial J2000 coordinates
	Vec3d viewDirectionJ2000;
	// Viewing direction in the mount reference frame.
	Vec3d viewDirectionMountFrame;

	// Up vector (in OpenGL terms) in the mount reference frame.
	// This can usually be just 0/0/1, but must be set to something useful when viewDirectionMountFrame is parallel, i.e. looks into a pole.
	Vec3d upVectorMountFrame;

	// TODO: Docfix?
	float dragTriggerDistance;

	// Viewport shifting. This animates a property belonging to StelCore. But the shift itself is likely best placed here.
	QTimeLine *viewportOffsetTimeline;
	// Those two are used during viewport offset animation transitions. Both are set by moveViewport(), and irrelevant after the transition.
	Vec2d oldViewportOffset;
	Vec2d targetViewportOffset;

	bool flagIndicationMountMode; // state of mount mode

	//! @name Screen message infrastructure
	//@{
	int lastMessageID;
	//@}
};

#endif // STELMOVEMENTMGR_HPP

