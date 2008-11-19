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

#ifndef _PROJECTOR_HPP_
#define _PROJECTOR_HPP_

#include "ProjectorType.hpp"
#include "vecmath.h"
#include "SphereGeometry.hpp"

class SFont;

//! @class Projector
//! Provides functions for drawing operations which are performed with some sort of 
//! "projection" according to the current projection mode.  This projection 
//! distorts the shape of the objects to be drawn and make the necessary calls 
//! to OpenGL to draw the required object. This class overrides a number of openGL 
//! functions to enable non-linear projection, such as fisheye or stereographic 
//! projections. This class also provide drawing primitives that are optimized 
//! according to the projection mode.
class Projector
{
public:
	friend class StelPainter;
	
	//! @enum ProjectorMaskType
	//! Define viewport mask types
	enum ProjectorMaskType
	{
		MaskNone,	//!< Regular - no mask.
		MaskDisk	//!< For disk viewport mode (circular mask to seem like bins/telescope)
	};
	
	//! @struct ProjectorParams
	//! Contains all the param needed to initialize a Projector
	struct ProjectorParams
	{
		Vector4<int> viewportXywh;     //! posX, posY, width, height
		double fov;                    //! FOV in degrees
		bool gravityLabels;            //! the flag to use gravity labels or not
		ProjectorMaskType maskType;    //! The current projector mask
		double zNear, zFar;            //! Near and far clipping planes
		Vec2d viewportCenter;          //! Viewport center in screen pixel
		double viewportFovDiameter;    //! diameter of the FOV disk in pixel
		bool flipHorz, flipVert;       //! Whether to flip in horizontal or vertical directions
	};
	
	///////////////////////////////////////////////////////////////////////////
	// Main constructor
	Projector(const Mat4d& modelViewMat);
	~Projector();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods which must be reimplemented by all instance of Projector
	//! Get an ID matching the projection type to use for reference in config files
	virtual QString getId() const = 0;
	//! Get a human-readable name for this projection type
	virtual QString getNameI18() const = 0;
	//! Get a human-readable short description for this projection type
	virtual QString getDescriptionI18() const {return "No description";}
	//! Get a HTML version of the short description for this projection type
	QString getHtmlSummary() const;
	//! Get the maximum FOV apperture in degree
	virtual double getMaxFov() const = 0;	
	//! Apply the transformation in the forward direction in place.
	//! After transformation v[2] will always contain the length of the original v: sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
	//! regardless of the projection type. This makes it possible to implement depth buffer testing in a way independent of the
	//! projection type. I would like to return the squared length instead of the length because of performance reasons.
	//! But then far away objects are not textured any more, perhaps because of a depth buffer overflow although
	//! the depth test is disabled?
	virtual bool forward(Vec3d &v) const = 0;
	//! Apply the transformation in the backward projection in place.
	virtual bool backward(Vec3d &v) const = 0;
	//! Return the small zoom increment to use at the given FOV for nice movements
	virtual double deltaZoom(double fov) const = 0;

	//! Convert a Field Of View radius value in radians in ViewScalingFactor (used internally)
	virtual double fovToViewScalingFactor(double fov) const = 0;
	//! Convert a ViewScalingFactor value (used internally) in Field Of View radius in radians
	virtual double viewScalingFactorToFov(double vsf) const = 0;
	
public:
	//! Initialise the Projector from a param instance
	void init(const ProjectorParams& param);
	
	//! Get the current state of the flag which decides whether to 
	//! arrage labels so that they are aligned with the bottom of a 2d 
	//! screen, or a 3d dome.
	bool getFlagGravityLabels() const { return gravityLabels; }

	//! Get the lower left corner of the viewport and the width, height.
	const Vector4<int>& getViewport(void) const {return viewportXywh;}

	//! Get the center of the viewport relative to the lower left corner.
	Vec2d getViewportCenter(void) const
	{
		return Vec2d(viewportCenter[0]-viewportXywh[0],viewportCenter[1]-viewportXywh[1]);
	}

	//! Get the diameter of the FOV disk in pixels
	double getViewportFovDiameter(void) const {return viewportFovDiameter;}
	
	//! Get the horizontal viewport offset in pixels.
	int getViewportPosX(void) const {return viewportXywh[0];}
	//! Get the vertical viewport offset in pixels.
	int getViewportPosY(void) const {return viewportXywh[1];}
	//! Get the viewport width in pixels.
	int getViewportWidth(void) const {return viewportXywh[2];}
	//! Get the viewport height in pixels.
	int getViewportHeight(void) const {return viewportXywh[3];}
	
	//! Get the maximum ratio between the viewport height and width
	float getViewportRatio() const {return getViewportWidth()>getViewportHeight() ? getViewportWidth()/getViewportHeight() : getViewportHeight()/getViewportWidth();}
	
	//! Return a convex polygon on the sphere which includes the viewport in the current frame.
	//! @param marginX an extra margin in pixel which extends the polygon size in the X direction
	//! @param marginY an extra margin in pixel which extends the polygon size in the Y direction
	//! @TODO Should be unified with unprojectViewport
	StelGeom::ConvexPolygon getViewportConvexPolygon(double marginX=0., double marginY=0.) const;

	//! Un-project the entire viewport depending on mapping, maskType,
	//! viewportFovDiameter, viewportCenter, and viewport dimensions.
	//! @TODO Should be unified with getViewportConvexPolygon
	StelGeom::ConvexS unprojectViewport() const;

	//! Return a Halfspace containing the whole viewport
	StelGeom::HalfSpace getBoundingHalfSpace() const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods for controlling the PROJECTION matrix
	
	//! Get whether front faces need to be oriented in the clockwise direction
	bool needGlFrontFaceCW(void) const {return (flipHorz*flipVert < 0.0);}
	
	//! Get size of a radian in pixels at the center of the viewport disk
	double getPixelPerRadAtCenter(void) const {return pixelPerRad;}
	
	///////////////////////////////////////////////////////////////////////////
	// Full projection methods
	//! Check to see if a 2d position is inside the viewport.
	//! TODO Optimize by storing viewportXywh[1] + viewportXywh[3] and viewportXywh[0] + viewportXywh[2] already computed
	bool checkInViewport(const Vec3d& pos) const
		{return (pos[1]>=viewportXywh[1] && pos[0]>=viewportXywh[0] &&
		pos[1]<=(viewportXywh[1] + viewportXywh[3]) && pos[0]<=(viewportXywh[0] + viewportXywh[2]));}

	//! Return the position where the 2 2D point p1 and p2 cross the viewport edge
	//! P1 must be inside the viewport and P2 outside (check with checkInViewport() before calling this method)
	Vec3d viewPortIntersect(const Vec3d& p1, const Vec3d& p2) const
	{
		Vec3d v1=p1;
		Vec3d v2=p2;
		Vec3d v;
		for (int i=0;i<8;++i)
		{
			v=(v1+v2)*0.5;
			if (!checkInViewport(v))
				v2=v;
			else
				v1=v;
		}
		return v;
	}
		
	//! Project the vector v from the current frame into the viewport.
	//! @param v the vector in the current frame.
	//! @param win the projected vector in the viewport 2D frame.
	//! @return true if the projected coordinate is valid.
	bool project(const Vec3d& v, Vec3d& win) const
	{
		// really important speedup:
		win[0] = modelViewMatrix.r[0]*v[0] + modelViewMatrix.r[4]*v[1]
				+ modelViewMatrix.r[8]*v[2] + modelViewMatrix.r[12];
		win[1] = modelViewMatrix.r[1]*v[0] + modelViewMatrix.r[5]*v[1]
				+ modelViewMatrix.r[9]*v[2] + modelViewMatrix.r[13];
		win[2] = modelViewMatrix.r[2]*v[0] + modelViewMatrix.r[6]*v[1]
				+ modelViewMatrix.r[10]*v[2] + modelViewMatrix.r[14];
		const bool rval = forward(win);
		// very important: even when the projected point comes from an
		// invisible region of the sky (rval=false), we must finish
		// reprojecting, so that OpenGl can successfully eliminate
		// polygons by culling.
		win[0] = viewportCenter[0] + flipHorz * pixelPerRad * win[0];
		win[1] = viewportCenter[1] + flipVert * pixelPerRad * win[1];
		win[2] = (win[2] - zNear) / (zNear - zFar);
		return rval;
	}

	//! Project the vector v from the current frame into the viewport.
	//! @param v the vector in the current frame.
	//! @param win the projected vector in the viewport 2D frame.
	//! @return true if the projected point is inside the viewport.
	bool projectCheck(const Vec3d& v, Vec3d& win) const {return (project(v, win) && checkInViewport(win));}

	//! Project the vector v from the viewport frame into the current frame.
	//! @param win the vector in the viewport 2D frame.
	//! @param v the projected vector in the current frame.
	//! @return true if the projected coordinate is valid.
	bool unProject(const Vec3d& win, Vec3d& v) const {return unProject(win[0], win[1], v);}
	bool unProject(double x, double y, Vec3d& v) const;

	//! Project the vectors v1 and v2 from the current frame into the viewport.
	//! @param v1 the first vector in the current frame.
	//! @param v2 the second vector in the current frame.
	//! @param win1 the first projected vector in the viewport 2D frame.
	//! @param win2 the second projected vector in the viewport 2D frame.
	//! @return true if at least one of the projected vector is within the viewport.
	bool projectLineCheck(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project(v1, win1) && project(v2, win2) && (checkInViewport(win1) || checkInViewport(win2));}

	//! Get the current model view matrix.
	const Mat4d& getModelViewMatrix() const {return modelViewMatrix;}
	
	///////////////////////////////////////////////////////////////////////////
	//! Get a string description of a ProjectorMaskType.
	static const QString maskTypeToString(ProjectorMaskType type);
	//! Get a ProjectorMaskType from a string description.
	static ProjectorMaskType stringToMaskType(const QString &s);
	
	//! Get the current type of the mask if any.
	ProjectorMaskType getMaskType(void) const {return maskType;}
	
private:

	ProjectorMaskType maskType;    // The current projector mask
	double zNear, zFar;            // Near and far clipping planes
	Vector4<int> viewportXywh;   // Viewport parameters
	Vec2d viewportCenter;          // Viewport center in screen pixel
	double viewportFovDiameter;    // diameter of the FOV disk in pixel
	double pixelPerRad;            // pixel per rad at the center of the viewport disk
	double flipHorz,flipVert;      // Whether to flip in horizontal or vertical directions
	bool gravityLabels;            // should label text align with the horizon?
	Mat4d modelViewMatrix;         // openGL MODELVIEW Matrix
};

#endif // _PROJECTOR_HPP_

