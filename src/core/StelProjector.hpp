/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Copyright (C) 2012 Matthew Gates
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

#ifndef STELPROJECTOR_HPP
#define STELPROJECTOR_HPP

#include "StelProjectorType.hpp"
#include "VecMath.hpp"
#include "StelSphereGeometry.hpp"

//! @class StelProjector
//! Provide the main interface to all operations of projecting coordinates from sky to screen.
//! The StelProjector also defines the viewport size and position.
//! All positions in pixels are in real device pixels, which is not the same as device independent pixels. On a mac with
//! retina screen, there are 2 device pixels per pixels.
//! All methods from this class are threadsafe. The usual usage is to create local instances of StelProjectorP using the
//! getProjection() method from StelCore where needed.
//! For performing drawing using a particular projection, refer to the StelPainter class.
//! @sa StelProjectorP
class StelProjector
{
public:
	friend class StelPainter;
	friend class StelCore;

	class ModelViewTranform;
	//! @typedef ModelViewTranformP
	//! Shared pointer on a ModelViewTranform instance (implement reference counting)
	typedef QSharedPointer<ModelViewTranform> ModelViewTranformP;

	//! @class ModelViewTranform
	//! Allows to define non linear operations in addition to the standard linear (Matrix 4d) ModelView transformation.
	class ModelViewTranform
	{
	public:
		ModelViewTranform() {;}
		virtual ~ModelViewTranform() {;}
		virtual void forward(Vec3d&) const =0;
		virtual void backward(Vec3d&) const =0;
		virtual void forward(Vec3f&) const =0;
		virtual void backward(Vec3f&) const =0;

		virtual void combine(const Mat4d&)=0;
		virtual ModelViewTranformP clone() const=0;

		virtual Mat4d getApproximateLinearTransfo() const=0;
	};

	class Mat4dTransform: public ModelViewTranform
	{
	public:
        Mat4dTransform(const Mat4d& m);
        void forward(Vec3d& v) const;
        void backward(Vec3d& v) const;
        void forward(Vec3f& v) const;
        void backward(Vec3f& v) const;
        void combine(const Mat4d& m);
        Mat4d getApproximateLinearTransfo() const;
        ModelViewTranformP clone() const;

	private:
		//! transfo matrix and invert
		Mat4d transfoMat;
		Mat4f transfoMatf;
	};

	//! @enum StelProjectorMaskType
	//! Define viewport mask types
	enum StelProjectorMaskType
	{
		MaskNone,	//!< Regular - no mask.
		MaskDisk	//!< For disk viewport mode (circular mask to seem like bins/telescope)
	};

	//! @struct StelProjectorParams
	//! Contains all the param needed to initialize a StelProjector
	struct StelProjectorParams
	{
		StelProjectorParams()
			: viewportXywh(0, 0, 256, 256)
			, fov(60.f)
			, gravityLabels(false)
			, defaultAngleForGravityText(0.f)
			, maskType(MaskNone)
			, zNear(0.)
			, zFar(0.)
			, viewportCenter(128, 128)
			, viewportCenterOffset(0, 0)
			, viewportFovDiameter(0)
			, flipHorz(false)
			, flipVert(false)
			, devicePixelsPerPixel(1)
			, widthStretch(1) {;}

		Vector4<int> viewportXywh;       //! posX, posY, width, height
		float fov;                       //! FOV in degrees
		bool gravityLabels;              //! the flag to use gravity labels or not
		float defaultAngleForGravityText;//! a rotation angle to apply to gravity text (only if gravityLabels is set to false)
		StelProjectorMaskType maskType;  //! The current projector mask
		double zNear, zFar;              //! Near and far clipping planes
		Vector2<qreal> viewportCenter;   //! Viewport center in screen pixel
		Vector2<qreal> viewportCenterOffset;//! Viewport center's offset in fractions of screen width/height. Usable e.g. in cylindrical projection to move horizon down.
						 //! Currently only Y shift is fully implemented, X shift likely not too meaningful.
		qreal viewportFovDiameter;       //! diameter of the FOV disk in pixel
		bool flipHorz, flipVert;         //! Whether to flip in horizontal or vertical directions
		qreal devicePixelsPerPixel;      //! The number of device pixel per "Device Independent Pixels" (value is usually 1, but 2 for mac retina screens)
		qreal widthStretch;              //! A factor to adapt to special installation setups, e.g. multi-projector with edge blending. Allow to stretch/squeeze projected content. Larger than 1 means the image is stretched wider.
	};

	//! Destructor
	virtual ~StelProjector() {}

	///////////////////////////////////////////////////////////////////////////
	// Methods which must be reimplemented by all instance of StelProjector
	//! Get a human-readable name for this projection type
	virtual QString getNameI18() const = 0;
	//! Get a human-readable short description for this projection type
	virtual QString getDescriptionI18() const {return "No description";}
	//! Get a HTML version of the short description for this projection type
	QString getHtmlSummary() const;
	//! Get the maximum FOV apperture in degree
	virtual float getMaxFov() const = 0;
	//! Apply the transformation in the forward direction in place.
	//! After transformation v[2] will always contain the length of the original v: sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
	//! regardless of the projection type. This makes it possible to implement depth buffer testing in a way independent of the
	//! projection type. I would like to return the squared length instead of the length because of performance reasons.
	//! But then far away objects are not textured any more, perhaps because of a depth buffer overflow although
	//! the depth test is disabled?
	virtual bool forward(Vec3f& v) const = 0;
	//! Apply the transformation in the backward projection in place.
	virtual bool backward(Vec3d& v) const = 0;
	//! Return the small zoom increment to use at the given FOV for nice movements
	virtual float deltaZoom(float fov) const = 0;

	//! Determine whether a great circle connection p1 and p2 intersects with a projection discontinuity.
	//! For many projections without discontinuity, this should return always false, but for other like
	//! cylindrical projection it will return true if the line cuts the wrap-around line (i.e. at lon=180 if the observer look at lon=0).
	bool intersectViewportDiscontinuity(const Vec3d& p1, const Vec3d& p2) const;
	bool intersectViewportDiscontinuity(const SphericalCap& cap) const;

	//! Convert a Field Of View radius value in radians in ViewScalingFactor (used internally)
	virtual float fovToViewScalingFactor(float fov) const = 0;
	//! Convert a ViewScalingFactor value (used internally) in Field Of View radius in radians
	virtual float viewScalingFactorToFov(float vsf) const = 0;

	//! Get the current state of the flag which decides whether to
	//! arrage labels so that they are aligned with the bottom of a 2d
	//! screen, or a 3d dome.
	bool getFlagGravityLabels() const;

	//! Get the lower left corner of the viewport and the width, height.
	const Vec4i& getViewport() const;

	//! Get the center of the viewport relative to the lower left corner of the screen.
	Vector2<qreal> getViewportCenter() const;
	Vector2<qreal> getViewportCenterOffset() const;

	//! Get the horizontal viewport offset in pixels.
	int getViewportPosX() const;
	//! Get the vertical viewport offset in pixels.
	int getViewportPosY() const;
	//! Get the viewport width in pixels.
	int getViewportWidth() const;
	//! Get the viewport height in pixels.
	int getViewportHeight() const;

	//! Get the number of device pixels per "Device Independent Pixels" (value is usually 1, but 2 for mac retina screens).
	qreal getDevicePixelsPerPixel() const {return devicePixelsPerPixel;}
	
	//! Return a convex polygon on the sphere which includes the viewport in the current frame.
	//! @param marginX an extra margin in pixel which extends the polygon size in the X direction.
	//! @param marginY an extra margin in pixel which extends the polygon size in the Y direction.
	//! @return a SphericalConvexPolygon or the special fullSky region if the viewport cannot be
	//! represented by a convex polygon (e.g. if aperture > 180 deg).
	SphericalRegionP getViewportConvexPolygon(float marginX=0., float marginY=0.) const;

	//! Return a SphericalCap containing the whole viewport
	const SphericalCap& getBoundingCap() const;

	//! Get size of a radian in pixels at the center of the viewport disk
	float getPixelPerRadAtCenter() const;

	//! Get the current FOV diameter in degrees
	float getFov() const;

	//! Get whether front faces need to be oriented in the clockwise direction
	bool needGlFrontFaceCW() const;

	///////////////////////////////////////////////////////////////////////////
	// Full projection methods
	//! Check to see if a 2d position is inside the viewport.
	//! TODO Optimize by storing viewportXywh[1] + viewportXywh[3] and viewportXywh[0] + viewportXywh[2] already computed
	bool checkInViewport(const Vec3d& pos) const;

	//! Check to see if a 2d position is inside the viewport.
	//! TODO Optimize by storing viewportXywh[1] + viewportXywh[3] and viewportXywh[0] + viewportXywh[2] already computed
	bool checkInViewport(const Vec3f& pos) const;

	//! Return the position where the 2 2D point p1 and p2 cross the viewport edge
	//! P1 must be inside the viewport and P2 outside (check with checkInViewport() before calling this method)
	Vec3d viewPortIntersect(const Vec3d& p1, const Vec3d& p2) const;

	//! Project the vector v from the current frame into the viewport.
	//! @param v the vector in the current frame.
	//! @param win the projected vector in the viewport 2D frame.
	//! @return true if the projected coordinate is valid.
	bool project(const Vec3d& v, Vec3d& win) const;

	//! Project the vector v from the current frame into the viewport.
	//! @param v the vector in the current frame.
	//! @param win the projected vector in the viewport 2D frame.
	//! @return true if the projected coordinate is valid.
	bool project(const Vec3f& v, Vec3f& win) const;

	virtual void project(int n, const Vec3d* in, Vec3f* out);

	virtual void project(int n, const Vec3f* in, Vec3f* out);

	//! Project the vector v from the current frame into the viewport.
	//! @param vd the vector in the current frame.
	//! @return true if the projected coordinate is valid.
	bool projectInPlace(Vec3d& vd) const;

	//! Project the vector v from the current frame into the viewport.
	//! @param v the vector in the current frame.
	//! @return true if the projected coordinate is valid.
	bool projectInPlace(Vec3f& v) const;

	//! Project the vector v from the current frame into the viewport.
	//! @param v the direction vector in the current frame. Does not need to be normalized.
	//! @param win the projected vector in the viewport 2D frame. win[0] and win[1] are in screen pixels, win[2] is unused.
	//! @return true if the projected point is inside the viewport.
	bool projectCheck(const Vec3d& v, Vec3d& win) const;

	//! Project the vector v from the current frame into the viewport.
	//! @param v the direction vector in the current frame. Does not need to be normalized.
	//! @param win the projected vector in the viewport 2D frame. win[0] and win[1] are in screen pixels, win[2] is unused.
	//! @return true if the projected point is inside the viewport.
	bool projectCheck(const Vec3f& v, Vec3f& win) const;

	//! Project the vector v from the viewport frame into the current frame.
	//! @param win the vector in the viewport 2D frame. win[0] and win[1] are in screen pixels, win[2] is unused.
	//! @param v the unprojected direction vector in the current frame.
	//! @return true if the projected coordinate is valid.
	bool unProject(const Vec3d& win, Vec3d& v) const;
	bool unProject(double x, double y, Vec3d& v) const;

	//! Project the vectors v1 and v2 from the current frame into the viewport.
	//! @param v1 the first vector in the current frame.
	//! @param v2 the second vector in the current frame.
	//! @param win1 the first projected vector in the viewport 2D frame.
	//! @param win2 the second projected vector in the viewport 2D frame.
	//! @return true if at least one of the projected vector is within the viewport.
	bool projectLineCheck(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const;

	//! Get the current model view matrix.
	ModelViewTranformP getModelViewTransform() const;

	//! Get the current projection matrix.
	Mat4f getProjectionMatrix() const;

	///////////////////////////////////////////////////////////////////////////
	//! Get a string description of a StelProjectorMaskType.
	static const QString maskTypeToString(StelProjectorMaskType type);
	//! Get a StelProjectorMaskType from a string description.
	static StelProjectorMaskType stringToMaskType(const QString &s);

	//! Get the current type of the mask if any.
	StelProjectorMaskType getMaskType(void) const;

protected:
	//! Private constructor. Only StelCore can create instances of StelProjector.
	StelProjector(ModelViewTranformP amodelViewTransform)
		: modelViewTransform(amodelViewTransform),
		  flipHorz(0.f),
		  flipVert(0.f),
		  pixelPerRad(0.f),
		  maskType(MaskNone),
		  zNear(0),
		  oneOverZNearMinusZFar(0),
		  viewportFovDiameter(0),
		  gravityLabels(true),
		  defaultAngleForGravityText(0.f),
		  devicePixelsPerPixel(1.),
		  widthStretch(1.0) {;}

	//! Return whether the projection presents discontinuities. Used for optimization.
	virtual bool hasDiscontinuity() const =0;
	//! Determine whether a great circle connection p1 and p2 intersects with a projection discontinuity.
	//! For many projections without discontinuity, this should return always false, but for other like
	//! cylindrical projection it will return true if the line cuts the wrap-around line (i.e. at lon=180 if the observer look at lon=0).
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const = 0;

	//! Determine whether a cap intersects with a projection discontinuity.
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const = 0;

	//! Initialize the bounding cap.
	virtual void computeBoundingCap();

	ModelViewTranformP modelViewTransform;	// Operator to apply (if not Q_NULLPTR) before the modelview projection step

	float flipHorz,flipVert;            // Whether to flip in horizontal or vertical directions. Values only -1 (flip) or +1 (no flip).
	float pixelPerRad;                  // pixel per rad at the center of the viewport disk
	StelProjectorMaskType maskType;     // The current projector mask
	double zNear, oneOverZNearMinusZFar;// Near and far clipping planes
	Vec4i viewportXywh;                 // Viewport parameters
	Vector2<qreal> viewportCenter;               // Viewport center in screen pixel
	Vector2<qreal> viewportCenterOffset;         // Viewport center's offset in fractions of screen width/height. Usable e.g. in cylindrical projection to move horizon down.
					    // Currently only Y shift is fully implemented, X shift likely not too meaningful.
	qreal viewportFovDiameter;          // diameter of the FOV disk in pixel
	bool gravityLabels;                 // should label text align with the horizon?
	float defaultAngleForGravityText;   // a rotation angle to apply to gravity text (only if gravityLabels is set to false)
	SphericalCap boundingCap;           // Bounding cap of the whole viewport
	qreal devicePixelsPerPixel;         // The number of device pixel per "Device Independent Pixels" (value is usually 1, but 2 for mac retina screens)
	qreal widthStretch;                 // A factor to adapt to special installation setups, e.g. multi-projector with edge blending. Allow to stretch/squeeze projected content. Larger than 1 means the image is stretched wider.
private:
	//! Initialise the StelProjector from a param instance.
	void init(const StelProjectorParams& param);
};

#endif // STELPROJECTOR_HPP
