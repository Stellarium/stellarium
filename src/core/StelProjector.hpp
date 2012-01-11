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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELPROJECTOR_HPP_
#define _STELPROJECTOR_HPP_

#include "StelProjectorType.hpp"
#include "VecMath.hpp"
#include "StelSphereGeometry.hpp"

//! @class StelProjector
//! Provide the main interface to all operations of projecting coordinates from sky to screen.
//! The StelProjector also defines the viewport size and position.
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

	//! @class PreModelViewFunc
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
		Mat4dTransform(const Mat4d& m) : transfoMat(m),
			transfoMatf(m[0], m[1], m[2], m[3],
							 m[4], m[5], m[6], m[7],
							 m[8], m[9], m[10], m[11],
							 m[12], m[13], m[14], m[15]) {;}

		void forward(Vec3d& v) const {v.transfo4d(transfoMat);}
		void backward(Vec3d& v) const
		{
			// We need no matrix inversion because we always work with orthogonal matrices (where the transposed is the inverse).
			//v.transfo4d(inverseTransfoMat);
			const double x = v[0] - transfoMat.r[12];
			const double y = v[1] - transfoMat.r[13];
			const double z = v[2] - transfoMat.r[14];
			v[0] = transfoMat.r[0]*x + transfoMat.r[1]*y + transfoMat.r[2]*z;
			v[1] = transfoMat.r[4]*x + transfoMat.r[5]*y + transfoMat.r[6]*z;
			v[2] = transfoMat.r[8]*x + transfoMat.r[9]*y + transfoMat.r[10]*z;
		}
		void forward(Vec3f& v) const {v.transfo4d(transfoMatf);}
		void backward(Vec3f& v) const
		{
			// We need no matrix inversion because we always work with orthogonal matrices (where the transposed is the inverse).
			//v.transfo4d(inverseTransfoMat);
			const float x = v[0] - transfoMatf.r[12];
			const float y = v[1] - transfoMatf.r[13];
			const float z = v[2] - transfoMatf.r[14];
			v[0] = transfoMatf.r[0]*x + transfoMatf.r[1]*y + transfoMatf.r[2]*z;
			v[1] = transfoMatf.r[4]*x + transfoMatf.r[5]*y + transfoMatf.r[6]*z;
			v[2] = transfoMatf.r[8]*x + transfoMatf.r[9]*y + transfoMatf.r[10]*z;
		}
		void combine(const Mat4d& m)
		{
			Mat4f mf(m[0], m[1], m[2], m[3],
							 m[4], m[5], m[6], m[7],
							 m[8], m[9], m[10], m[11],
							 m[12], m[13], m[14], m[15]);
			transfoMat=transfoMat*m;
			transfoMatf=transfoMatf*mf;
		}
		Mat4d getApproximateLinearTransfo() const {return transfoMat;}
		ModelViewTranformP clone() const {return ModelViewTranformP(new Mat4dTransform(transfoMat));}

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
		StelProjectorParams() : viewportXywh(0, 0, 256, 256), fov(60.f), gravityLabels(false), defautAngleForGravityText(0.f), maskType(MaskNone), viewportCenter(128.f, 128.f), flipHorz(false), flipVert(false) {;}
		Vector4<int> viewportXywh;      //! posX, posY, width, height
		float fov;                      //! FOV in degrees
		bool gravityLabels;             //! the flag to use gravity labels or not
		float defautAngleForGravityText;//! a rotation angle to apply to gravity text (only if gravityLabels is set to false)
		StelProjectorMaskType maskType; //! The current projector mask
		float zNear, zFar;              //! Near and far clipping planes
		Vec2f viewportCenter;           //! Viewport center in screen pixel
		float viewportFovDiameter;      //! diameter of the FOV disk in pixel
		bool flipHorz, flipVert;        //! Whether to flip in horizontal or vertical directions
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
	bool intersectViewportDiscontinuity(const Vec3d& p1, const Vec3d& p2) const
	{
		if (hasDiscontinuity()==false)
			return false;
		Vec3d v1(p1);
		modelViewTransform->forward(v1);
		Vec3d v2(p2);
		modelViewTransform->forward(v2);
		return intersectViewportDiscontinuityInternal(v1, v2);
	}

	bool intersectViewportDiscontinuity(const SphericalCap& cap) const
	{
		if (hasDiscontinuity()==false)
			return false;
		Vec3d v1(cap.n);
		modelViewTransform->forward(v1);
		return intersectViewportDiscontinuityInternal(v1, cap.d);
	}

	//! Convert a Field Of View radius value in radians in ViewScalingFactor (used internally)
	virtual float fovToViewScalingFactor(float fov) const = 0;
	//! Convert a ViewScalingFactor value (used internally) in Field Of View radius in radians
	virtual float viewScalingFactorToFov(float vsf) const = 0;

	//! Get the current state of the flag which decides whether to
	//! arrage labels so that they are aligned with the bottom of a 2d
	//! screen, or a 3d dome.
	bool getFlagGravityLabels() const { return gravityLabels; }

	//! Get the lower left corner of the viewport and the width, height.
	const Vec4i& getViewport() const {return viewportXywh;}

	//! Get the center of the viewport relative to the lower left corner of the screen.
	Vec2f getViewportCenter() const
	{
		return Vec2f(viewportCenter[0]-viewportXywh[0],viewportCenter[1]-viewportXywh[1]);
	}

	//! Get the horizontal viewport offset in pixels.
	int getViewportPosX() const {return viewportXywh[0];}
	//! Get the vertical viewport offset in pixels.
	int getViewportPosY() const {return viewportXywh[1];}
	//! Get the viewport width in pixels.
	int getViewportWidth() const {return viewportXywh[2];}
	//! Get the viewport height in pixels.
	int getViewportHeight() const {return viewportXywh[3];}

	//! Return a convex polygon on the sphere which includes the viewport in the current frame.
	//! @param marginX an extra margin in pixel which extends the polygon size in the X direction.
	//! @param marginY an extra margin in pixel which extends the polygon size in the Y direction.
	//! @return a SphericalConvexPolygon or the special fullSky region if the viewport cannot be
	//! represented by a convex polygon (e.g. if aperture > 180 deg).
	SphericalRegionP getViewportConvexPolygon(float marginX=0., float marginY=0.) const;

	//! Return a SphericalCap containing the whole viewport
	const SphericalCap& getBoundingCap() const {return boundingCap;}

	//! Get size of a radian in pixels at the center of the viewport disk
	float getPixelPerRadAtCenter() const {return pixelPerRad;}

	//! Get the current FOV diameter in degree
	float getFov() const {return 360.f/M_PI*viewScalingFactorToFov(0.5f*viewportFovDiameter/pixelPerRad);}

	//! Get whether front faces need to be oriented in the clockwise direction
	bool needGlFrontFaceCW() const {return (flipHorz*flipVert < 0.f);}

	///////////////////////////////////////////////////////////////////////////
	// Full projection methods
	//! Check to see if a 2d position is inside the viewport.
	//! TODO Optimize by storing viewportXywh[1] + viewportXywh[3] and viewportXywh[0] + viewportXywh[2] already computed
	bool checkInViewport(const Vec3d& pos) const
	{
		return (pos[1]>=viewportXywh[1] && pos[0]>=viewportXywh[0] &&
			pos[1]<=(viewportXywh[1] + viewportXywh[3]) && pos[0]<=(viewportXywh[0] + viewportXywh[2]));
	}

	//! Check to see if a 2d position is inside the viewport.
	//! TODO Optimize by storing viewportXywh[1] + viewportXywh[3] and viewportXywh[0] + viewportXywh[2] already computed
	bool checkInViewport(const Vec3f& pos) const
	{
		return (pos[1]>=viewportXywh[1] && pos[0]>=viewportXywh[0] &&
			pos[1]<=(viewportXywh[1] + viewportXywh[3]) && pos[0]<=(viewportXywh[0] + viewportXywh[2]));
	}

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
	inline bool project(const Vec3d& v, Vec3d& win) const
	{
		win = v;
		return projectInPlace(win);
	}

	//! Project the vector v from the current frame into the viewport.
	//! @param v the vector in the current frame.
	//! @param win the projected vector in the viewport 2D frame.
	//! @return true if the projected coordinate is valid.
	inline bool project(const Vec3f& v, Vec3f& win) const
	{
		win = v;
		return projectInPlace(win);
	}

	virtual void project(int n, const Vec3d* in, Vec3f* out)
	{
		Vec3d v;
		for (int i = 0; i < n; ++i, ++out)
		{
			v = in[i];
			modelViewTransform->forward(v);
			out->set(v[0], v[1], v[2]);
			forward(*out);
			out->set(viewportCenter[0] + flipHorz * pixelPerRad * (*out)[0],
				viewportCenter[1] + flipVert * pixelPerRad * (*out)[1],
				((*out)[2] - zNear) * oneOverZNearMinusZFar);
		}
	}

	virtual void project(int n, const Vec3f* in, Vec3f* out)
	{
		for (int i = 0; i < n; ++i, ++out)
		{
			*out=in[i];
			modelViewTransform->forward(*out);
			forward(*out);
			out->set(viewportCenter[0] + flipHorz * pixelPerRad * (*out)[0],
				viewportCenter[1] + flipVert * pixelPerRad * (*out)[1],
				((*out)[2] - zNear) * oneOverZNearMinusZFar);
		}
	}

	//! Project the vector v from the current frame into the viewport.
	//! @param vd the vector in the current frame.
	//! @return true if the projected coordinate is valid.
	inline bool projectInPlace(Vec3d& vd) const
	{
		modelViewTransform->forward(vd);
		Vec3f v(vd[0], vd[1], vd[2]);
		const bool rval = forward(v);
		// very important: even when the projected point comes from an
		// invisible region of the sky (rval=false), we must finish
		// reprojecting, so that OpenGl can successfully eliminate
		// polygons by culling.
		vd[0] = viewportCenter[0] + flipHorz * pixelPerRad * v[0];
		vd[1] = viewportCenter[1] + flipVert * pixelPerRad * v[1];
		vd[2] = (v[2] - zNear) * oneOverZNearMinusZFar;
		return rval;
	}

	//! Project the vector v from the current frame into the viewport.
	//! @param v the vector in the current frame.
	//! @return true if the projected coordinate is valid.
	inline bool projectInPlace(Vec3f& v) const
	{
		modelViewTransform->forward(v);
		const bool rval = forward(v);
		// very important: even when the projected point comes from an
		// invisible region of the sky (rval=false), we must finish
		// reprojecting, so that OpenGl can successfully eliminate
		// polygons by culling.
		v[0] = viewportCenter[0] + flipHorz * pixelPerRad * v[0];
		v[1] = viewportCenter[1] + flipVert * pixelPerRad * v[1];
		v[2] = (v[2] - zNear) * oneOverZNearMinusZFar;
		return rval;
	}

	//! Project the vector v from the current frame into the viewport.
	//! @param v the direction vector in the current frame. Does not need to be normalized.
	//! @param win the projected vector in the viewport 2D frame. win[0] and win[1] are in screen pixels, win[2] is unused.
	//! @return true if the projected point is inside the viewport.
	bool projectCheck(const Vec3d& v, Vec3d& win) const {return (project(v, win) && checkInViewport(win));}

	//! Project the vector v from the current frame into the viewport.
	//! @param v the direction vector in the current frame. Does not need to be normalized.
	//! @param win the projected vector in the viewport 2D frame. win[0] and win[1] are in screen pixels, win[2] is unused.
	//! @return true if the projected point is inside the viewport.
	bool projectCheck(const Vec3f& v, Vec3f& win) const {return (project(v, win) && checkInViewport(win));}

	//! Project the vector v from the viewport frame into the current frame.
	//! @param win the vector in the viewport 2D frame. win[0] and win[1] are in screen pixels, win[2] is unused.
	//! @param v the unprojected direction vector in the current frame.
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
	ModelViewTranformP getModelViewTransform() const {return modelViewTransform;}

	//! Get the current projection matrix.
	Mat4f getProjectionMatrix() const {return Mat4f(2.f/viewportXywh[2], 0, 0, 0, 0, 2.f/viewportXywh[3], 0, 0, 0, 0, -1., 0., -(2.f*viewportXywh[0] + viewportXywh[2])/viewportXywh[2], -(2.f*viewportXywh[1] + viewportXywh[3])/viewportXywh[3], 0, 1);}

	///////////////////////////////////////////////////////////////////////////
	//! Get a string description of a StelProjectorMaskType.
	static const QString maskTypeToString(StelProjectorMaskType type);
	//! Get a StelProjectorMaskType from a string description.
	static StelProjectorMaskType stringToMaskType(const QString &s);

	//! Get the current type of the mask if any.
	StelProjectorMaskType getMaskType(void) const {return maskType;}

protected:
	//! Private constructor. Only StelCore can create instances of StelProjector.
	StelProjector(ModelViewTranformP amodelViewTransform) : modelViewTransform(amodelViewTransform) {;}

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

	ModelViewTranformP modelViewTransform;	// Operator to apply (if not NULL) before the modelview projection step

	float flipHorz,flipVert;            // Whether to flip in horizontal or vertical directions
	float pixelPerRad;                  // pixel per rad at the center of the viewport disk
	StelProjectorMaskType maskType;     // The current projector mask
	float zNear, oneOverZNearMinusZFar; // Near and far clipping planes
	Vec4i viewportXywh;                 // Viewport parameters
	Vec2f viewportCenter;               // Viewport center in screen pixel
	float viewportFovDiameter;          // diameter of the FOV disk in pixel
	bool gravityLabels;                 // should label text align with the horizon?
	float defautAngleForGravityText;    // a rotation angle to apply to gravity text (only if gravityLabels is set to false)
	SphericalCap boundingCap;           // Bounding cap of the whole viewport

private:
	//! Initialise the StelProjector from a param instance.
	void init(const StelProjectorParams& param);
};

#endif // _STELPROJECTOR_HPP_
