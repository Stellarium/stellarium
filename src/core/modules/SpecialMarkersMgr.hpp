/*
 * Stellarium
 * Copyright (C) 2020 Alexander Wolf
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

#ifndef SPECIALMARKERSMGR_HPP
#define SPECIALMARKERSMGR_HPP

#include "VecMath.hpp"
#include "StelModule.hpp"

class SpecialSkyMarker;

class SpecialMarkersMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool fovCenterMarkerDisplayed	READ getFlagFOVCenterMarker		WRITE setFlagFOVCenterMarker		NOTIFY fovCenterMarkerDisplayedChanged)
	Q_PROPERTY(Vec3f fovCenterMarkerColor		READ getColorFOVCenterMarker	WRITE setColorFOVCenterMarker	NOTIFY fovCenterMarkerColorChanged)

	Q_PROPERTY(bool fovCircularMarkerDisplayed	READ getFlagFOVCircularMarker	WRITE setFlagFOVCircularMarker	NOTIFY fovCircularMarkerDisplayedChanged)
	Q_PROPERTY(double fovCircularMarkerSize	READ getFOVCircularMarkerSize	WRITE setFOVCircularMarkerSize	NOTIFY fovCircularMarkerSizeChanged)
	Q_PROPERTY(Vec3f fovCircularMarkerColor		READ getColorFOVCircularMarker	WRITE setColorFOVCircularMarker	NOTIFY fovCircularMarkerColorChanged)

	Q_PROPERTY(bool fovRectangularMarkerDisplayed	READ getFlagFOVRectangularMarker		WRITE setFlagFOVRectangularMarker		NOTIFY fovRectangularMarkerDisplayedChanged)
	Q_PROPERTY(double fovRectangularMarkerWidth	READ getFOVRectangularMarkerWidth	WRITE setFOVRectangularMarkerWidth	NOTIFY fovRectangularMarkerWidthChanged)
	Q_PROPERTY(double fovRectangularMarkerHeight	READ getFOVRectangularMarkerHeight	WRITE setFOVRectangularMarkerHeight	NOTIFY fovRectangularMarkerHeightChanged)
	Q_PROPERTY(double fovRectangularMarkerRotationAngle	READ getFOVRectangularMarkerRotationAngle	WRITE setFOVRectangularMarkerRotationAngle	NOTIFY fovRectangularMarkerRotationAngleChanged)
	Q_PROPERTY(Vec3f fovRectangularMarkerColor		READ getColorFOVRectangularMarker	WRITE setColorFOVRectangularMarker	NOTIFY fovRectangularMarkerColorChanged)

public:
	SpecialMarkersMgr();
	virtual ~SpecialMarkersMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the GridLinesMgr. This process checks the values in the
	//! application settings, and according to the settings there
	//! sets the visibility of the Equatorial Grids, Ecliptical Grids, Azimuthal Grid, Meridian Line,
	//! Equator Line and Ecliptic Lines.
	virtual void init();

	//! Get the module ID, returns "SpecialMarkersMgr".
	virtual QString getModuleID() const {return "SpecialMarkersMgr";}

	//! Draw the grids and great circle lines.
	//! Draws the Equatorial Grids, Ecliptical Grids, Azimuthal Grid, Meridian Line, Equator Line,
	//! Ecliptic Lines, Precession Circles, Conjunction-Opposition Line, east-west vertical and colures according to the
	//! various flags which control their visibility.
	virtual void draw(StelCore* core);

	//! Update time-dependent features.
	//! Used to control fading when turning on and off the grid lines and great circles.
	virtual void update(double deltaTime);

	//! Used to determine the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////////////////
	// Setter and getters
public slots:
	//! Setter for displaying the FOV center marker
	void setFlagFOVCenterMarker(const bool displayed);
	//! Accessor for displaying FOV center marker.
	bool getFlagFOVCenterMarker() const;
	//! Get the current color of the FOV center marker.
	Vec3f getColorFOVCenterMarker() const;
	//! Set the color of the FOV center marker.
	//! @param newColor The color of FOV center marker.
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorFOVCenterMarker(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorFOVCenterMarker(const Vec3f& newColor);

	//! Setter for displaying the circular FOV marker
	void setFlagFOVCircularMarker(const bool displayed);
	//! Accessor for displaying circular FOV marker.
	bool getFlagFOVCircularMarker() const;
	//! Setter for size of circular FOV marker
	void setFOVCircularMarkerSize(const double size);
	//! Accessor for get size of circular FOV marker.
	double getFOVCircularMarkerSize() const;
	//! Get the current color of the circular FOV marker.
	Vec3f getColorFOVCircularMarker() const;
	//! Set the color of the circular FOV marker.
	//! @param newColor The color of circular FOV marker.
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorFOVCircularMarker(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorFOVCircularMarker(const Vec3f& newColor);

	//! Setter for displaying the rectangular FOV marker
	void setFlagFOVRectangularMarker(const bool displayed);
	//! Accessor for displaying rectangular FOV marker.
	bool getFlagFOVRectangularMarker() const;
	//! Setter for width of rectangular FOV marker
	void setFOVRectangularMarkerWidth(const double size);
	//! Accessor for get width of rectangular FOV marker.
	double getFOVRectangularMarkerWidth() const;
	//! Setter for height of rectangular FOV marker
	void setFOVRectangularMarkerHeight(const double size);
	//! Accessor for get height of rectangular FOV marker.
	double getFOVRectangularMarkerHeight() const;
	//! Setter for rotation angle of rectangular FOV marker
	void setFOVRectangularMarkerRotationAngle(const double pa);
	//! Accessor for get rotation angle of rectangular FOV marker.
	double getFOVRectangularMarkerRotationAngle() const;
	//! Get the current color of the rectangular FOV marker.
	Vec3f getColorFOVRectangularMarker() const;
	//! Set the color of the rectangular FOV marker.
	//! @param newColor The color of rectangular FOV marker.
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorFOVRectangularMarker(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorFOVRectangularMarker(const Vec3f& newColor);

signals:
	void fovCenterMarkerDisplayedChanged(const bool displayed) const;
	void fovCenterMarkerColorChanged(const Vec3f & newColor) const;
	void fovCircularMarkerDisplayedChanged(const bool displayed) const;
	void fovCircularMarkerSizeChanged(const double size) const;
	void fovCircularMarkerColorChanged(const Vec3f & newColor) const;
	void fovRectangularMarkerDisplayedChanged(const bool displayed) const;
	void fovRectangularMarkerWidthChanged(const double size) const;
	void fovRectangularMarkerHeightChanged(const double size) const;
	void fovRectangularMarkerRotationAngleChanged(const double size) const;
	void fovRectangularMarkerColorChanged(const Vec3f & newColor) const;

private:
	SpecialSkyMarker * fovCenterMarker;
	SpecialSkyMarker * fovCircularMarker;
	SpecialSkyMarker * fovRectangularMarker;
};

#endif // SPECIALMARKERSMGR_HPP
