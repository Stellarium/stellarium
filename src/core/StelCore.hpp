/*
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

#ifndef _STELCORE_HPP_
#define _STELCORE_HPP_

#include "StelProjector.hpp"
#include "StelProjectorType.hpp"
#include <QString>
#include <QStringList>

class StelNavigator;
class StelToneReproducer;
class StelSkyDrawer;
class StelGeodesicGrid;
class StelMovementMgr;

//! @class StelCore
//! Main class for Stellarium core processing.
//! This class provides services like management of sky projections,
//! tone conversion, or reference frame conversion. It is used by the
//! various StelModules to update and display themself.
//! There is currently only one StelCore instance in Stellarium, but
//! in the future they may be more, allowing for example to display
//! several independent views of the sky at the same time.
//! @author Fabien Chereau
class StelCore : public QObject
{
	Q_OBJECT
	Q_ENUMS(ProjectionType);

public:
	//! @enum FrameType
	//! Supported reference frame types
	enum FrameType
	{
		FrameAltAz,                   //!< Altazimuthal reference frame centered on observer.
		FrameHeliocentricEcliptic,    //!< Ecliptic reference frame centered on the Sun
		FrameObservercentricEcliptic, //!< Ecliptic reference frame centered on the Observer
		FrameEquinoxEqu,              //!< Equatorial reference frame at the current equinox centered on the observer.
									  //! The north pole follows the precession of the planet on which the observer is located.
		FrameJ2000,                   //!< Equatorial reference frame at the J2000 equinox centered on the observer.
									  //! This is also the ICRS reference frame.
		FrameGalactic                 //! Galactic reference frame centered on observer.
	};

	//! Available projection types. A value of 1000 indicate the default projection
	enum ProjectionType
	{
		ProjectionPerspective,    //!< Perspective projection
		ProjectionEqualArea,      //!< Equal Area projection
		ProjectionStereographic,  //!< Stereograhic projection
		ProjectionFisheye,	      //!< Fisheye projection
		ProjectionHammer,         //!< Hammer-Aitoff projection
		ProjectionCylinder,	      //!< Cylinder projection
		ProjectionMercator,	      //!< Mercator projection
		ProjectionOrthographic	  //!< Orthographic projection
	};

	StelCore();
	virtual ~StelCore();

	//! Init and load all main core components.
	void init();

	//! Update all the objects with respect to the time.
	//! @param deltaTime the time increment in sec.
	void update(double deltaTime);

	//! Handle the resizing of the window
	void windowHasBeenResized(float x, float y, float width, float height);

	//! Update core state before drawing modules.
	void preDraw();

	//! Update core state after drawing modules.
	void postDraw();

	//! Get a new instance of a simple 2d projection. This projection cannot be used to project or unproject but
	//! only for 2d painting
	StelProjectorP getProjection2d() const;

	//! Get a new instance of projector using the current display parameters from Navigation, StelMovementMgr, etc...
	//! If not specified the projection type is the default one set in the core.
	//! This is a smart pointer, you don't need to delete it.
	StelProjectorP getProjection(FrameType frameType, ProjectionType projType=(ProjectionType)1000) const;

	//! Get an instance of projector using the current display parameters from Navigation, StelMovementMgr
	//! and using the given modelview matrix.
	//! If not specified default the projection type is the default one set in the core.
	StelProjectorP getProjection(const Mat4d& modelViewMat, ProjectionType projType=(ProjectionType)1000) const;

	//! Get the current navigation (manages frame transformation) used in the core.
	StelNavigator* getNavigator() {return navigation;}
	//! Get the current navigation (manages frame transformation) used in the core.
	const StelNavigator* getNavigator() const {return navigation;}

	//! Get the current tone reproducer used in the core.
	StelToneReproducer* getToneReproducer() {return toneConverter;}
	//! Get the current tone reproducer used in the core.
	const StelToneReproducer* getToneReproducer() const {return toneConverter;}

	//! Get the current StelSkyDrawer used in the core.
	StelSkyDrawer* getSkyDrawer() {return skyDrawer;}
	//! Get the current StelSkyDrawer used in the core.
	const StelSkyDrawer* getSkyDrawer() const {return skyDrawer;}

	//! Get an instance of StelGeodesicGrid which is garanteed to allow for at least maxLevel levels
	const StelGeodesicGrid* getGeodesicGrid(int maxLevel) const;

	//! Get the instance of movement manager.
	StelMovementMgr* getMovementMgr() {return movementMgr;}
	//! Get the const instance of movement manager.
	const StelMovementMgr* getMovementMgr() const {return movementMgr;}

	//! Set the near and far clipping planes.
	void setClippingPlanes(double znear, double zfar) {currentProjectorParams.zNear=znear;currentProjectorParams.zFar=zfar;}
	//! Get the near and far clipping planes.
	void getClippingPlanes(double* zn, double* zf) const {*zn = currentProjectorParams.zNear; *zf = currentProjectorParams.zFar;}

	//! Get the translated projection name from its TypeKey for the current locale.
	QString projectionTypeKeyToNameI18n(const QString& key) const;

	//! Get the projection TypeKey from its translated name for the current locale.
	QString projectionNameI18nToTypeKey(const QString& nameI18n) const;

	//! Get the current set of parameters to use when creating a new StelProjector.
	StelProjector::StelProjectorParams getCurrentStelProjectorParams() const {return currentProjectorParams;}
	//! Set the set of parameters to use when creating a new StelProjector.
	void setCurrentStelProjectorParams(const StelProjector::StelProjectorParams& newParams) {currentProjectorParams=newParams;}

public slots:
	//! Set the current ProjectionType to use
	void setCurrentProjectionType(ProjectionType type);
	ProjectionType getCurrentProjectionType() const {return currentProjectionType;}

	//! Get the current Mapping used by the Projection
	QString getCurrentProjectionTypeKey(void) const;
	//! Set the current ProjectionType to use from its key
	void setCurrentProjectionTypeKey(QString type);

	//! Get the list of all the available projections
	QStringList getAllProjectionTypeKeys() const;

	//! Set the mask type.
	void setMaskType(StelProjector::StelProjectorMaskType m) {currentProjectorParams.maskType = m; }

	//! Set the flag with decides whether to arrage labels so that
	//! they are aligned with the bottom of a 2d screen, or a 3d dome.
	void setFlagGravityLabels(bool gravity) { currentProjectorParams.gravityLabels = gravity; }
	//! Set the horizontal flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipHorz(bool flip) {currentProjectorParams.flipHorz = flip;}
	//! Set the vertical flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipVert(bool flip) {currentProjectorParams.flipVert = flip;}
	//! Get the state of the horizontal flip.
	//! @return True if flipped horizontally, else false.
	bool getFlipHorz(void) const {return currentProjectorParams.flipHorz;}
	//! Get the state of the vertical flip.
	//! @return True if flipped vertically, else false.
	bool getFlipVert(void) const {return currentProjectorParams.flipVert;}

private:
	StelNavigator* navigation;			// Manage all navigation parameters, coordinate transformations etc..
	StelToneReproducer* toneConverter;		// Tones conversion between stellarium world and display device
	StelSkyDrawer* skyDrawer;
	StelMovementMgr* movementMgr;		// Manage vision movements

	// Manage geodesic grid
	mutable StelGeodesicGrid* geodesicGrid;

	// The currently used projection type
	ProjectionType currentProjectionType;

	// Parameters to use when creating new instances of StelProjector
	StelProjector::StelProjectorParams currentProjectorParams;
};

#endif // _STELCORE_HPP_
