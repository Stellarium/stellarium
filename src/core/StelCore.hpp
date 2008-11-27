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

class Navigator;
class ToneReproducer;
class SkyDrawer;
class GeodesicGrid;
class MovementMgr;

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
	Q_OBJECT;
	Q_ENUMS(ProjectionType);

public:	
	//! @enum FrameType
	//! Supported reference frame types
	enum FrameType
	{
		FrameAltAz,      //!< Altazimuthal reference frame centered on observer.
  		FrameHelio,      //!< Heliocentric reference frame centered on the Sun
		FrameEquinoxEqu, //!< Equatorial reference frame at the current equinox centered on the observer. The north pole follow the precession of the planet on which the observer is located.
  		FrameJ2000       //!< Equatorial reference frame at the J2000 equinox centered on the observer. This is also the ICRS reference frame.
	};
	
	//! Available projection types. A value of 1000 indicate the default projection
	enum ProjectionType
	{
		ProjectionPerspective,    //!< Perspective projection
		ProjectionEqualArea,      //!< Equal Area projection
		ProjectionStereographic,  //!< Stereograhic projection
		ProjectionFisheye,	      //!< Fisheye projection
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
	void windowHasBeenResized(int width,int height);
	
	//! Update core state before drawing modules.
	void preDraw();

	//! Update core state after drawing modules.
	void postDraw();
		
	//! Get a new instance of a simple 2d projection. This projection cannot be used to project or unproject but
	//! only for 2d painting
	const StelProjectorP getProjection2d() const;
			
	//! Get a new instance of projector using the current display parameters from Navigation, MovementMgr, etc..
	//! If not specified default the projection type is the default one set in the core.
	//! This is a smart pointer, you don't need to delete it.
	const StelProjectorP getProjection(FrameType frameType, ProjectionType projType=(ProjectionType)1000) const;
	//! Get an instance of projector using the current display parameters from Navigation, MovementMgr
	//! and using the given modelview matrix.
	//! If not specified default the projection type is the default one set in the core.
	const StelProjectorP getProjection(const Mat4d& modelViewMat, ProjectionType projType=(ProjectionType)1000) const;
			
	//! Get the current navigation (manages frame transformation) used in the core.
	Navigator* getNavigation() {return navigation;}
	//! Get the current navigation (manages frame transformation) used in the core.
	const Navigator* getNavigation() const {return navigation;}

	//! Get the current tone reproducer used in the core.
	ToneReproducer* getToneReproducer() {return toneConverter;}
	//! Get the current tone reproducer used in the core.
	const ToneReproducer* getToneReproducer() const {return toneConverter;}

	//! Get the current SkyDrawer used in the core.
	SkyDrawer* getSkyDrawer() {return skyDrawer;}
	//! Get the current SkyDrawer used in the core.
	const SkyDrawer* getSkyDrawer() const {return skyDrawer;}

	//! Get an instance of GeodesicGrid which is garanteed to allow for at least maxLevel levels
	const GeodesicGrid* getGeodesicGrid(int maxLevel) const;
	
	//! Get the instance of movement manager.
	MovementMgr* getMovementMgr() {return movementMgr;}
	//! Get the const instance of movement manager.
	const MovementMgr* getMovementMgr() const {return movementMgr;}
	
	//! Set the near and far clipping planes.
	void setClippingPlanes(double znear, double zfar) {currentStelProjectorParams.zNear=znear;currentStelProjectorParams.zFar=zfar;} 
	//! Get the near and far clipping planes.
	void getClippingPlanes(double* zn, double* zf) const {*zn = currentStelProjectorParams.zNear; *zf = currentStelProjectorParams.zFar;}
	
	//! Get the translated projection name from its TypeKey for the current locale.
	QString projectionTypeKeyToNameI18n(const QString& key) const;
	
	//! Get the projection TypeKey from its translated name for the current locale.
	QString projectionNameI18nToTypeKey(const QString& nameI18n) const;
	
	//! Get the current set of parameters to use when creating a new StelProjector.
	StelProjector::StelProjectorParams getCurrentStelProjectorParams() const {return currentStelProjectorParams;}
	//! Set the set of parameters to use when creating a new StelProjector.
	void setCurrentStelProjectorParams(const StelProjector::StelProjectorParams& newParams) {currentStelProjectorParams=newParams;}
	
public slots:
	//! Set the current ProjectionType to use
	void setCurrentProjectionType(ProjectionType type) {currentProjectionType=type;}
	
	//! Get the current Mapping used by the Projection
	QString getCurrentProjectionTypeKey(void) const;
	//! Set the current ProjectionType to use from its key
	void setCurrentProjectionTypeKey(QString type);
	
	//! Get the list of all the available projections
	QStringList getAllProjectionTypeKeys() const;
				
	//! Set the mask type.
	void setMaskType(StelProjector::StelProjectorMaskType m) {currentStelProjectorParams.maskType = m; }
	
	//! Set the flag with decides whether to arrage labels so that
	//! they are aligned with the bottom of a 2d screen, or a 3d dome.
	void setFlagGravityLabels(bool gravity) { currentStelProjectorParams.gravityLabels = gravity; }	
	//! Set the horizontal flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipHorz(bool flip) {currentStelProjectorParams.flipHorz = flip;}
	//! Set the vertical flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipVert(bool flip) {currentStelProjectorParams.flipVert = flip;}
	//! Get the state of the horizontal flip.
	//! @return True if flipped horizontally, else false.
	bool getFlipHorz(void) const {return currentStelProjectorParams.flipHorz;}
	//! Get the state of the vertical flip.
	//! @return True if flipped vertically, else false.
	bool getFlipVert(void) const {return currentStelProjectorParams.flipVert;}
	
private:
	Navigator* navigation;			// Manage all navigation parameters, coordinate transformations etc..
	ToneReproducer* toneConverter;		// Tones conversion between stellarium world and display device
	SkyDrawer* skyDrawer;
	MovementMgr* movementMgr;		// Manage vision movements
	
	// Manage geodesic grid
	mutable GeodesicGrid* geodesicGrid;
	
	// The currently used projection type
	ProjectionType currentProjectionType;
	
	// Parameters to use when creating new instances of StelProjector
	StelProjector::StelProjectorParams currentStelProjectorParams;
};

#endif // _STELCORE_HPP_
