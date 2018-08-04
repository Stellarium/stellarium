/*
 * Stellarium
 * Copyright (C) 2008 Stellarium Developers
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

#ifndef STELPROJECTIONS_HPP
#define STELPROJECTIONS_HPP

#include "StelProjector.hpp"

class StelProjectorPerspective : public StelProjector
{
public:
	StelProjectorPerspective(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 120.f;}
	bool forward(Vec3f &v) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}
};

class StelProjectorEqualArea : public StelProjector
{
public:
	StelProjectorEqualArea(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 360.f;}
	bool forward(Vec3f &v) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}
};

class StelProjectorStereographic : public StelProjector
{
public:
	StelProjectorStereographic(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 235.f;}
	virtual void project(int n, const Vec3d* in, Vec3f* out)
	{
		Vec3d v;
		for (int i = 0; i < n; ++i, ++out)
		{
			v = in[i];
			modelViewTransform->forward(v);
			out->set(v[0], v[1], v[2]);
			StelProjectorStereographic::forward(*out);
			out->set(viewportCenter[0] + flipHorz * pixelPerRad * (*out)[0],
				viewportCenter[1] + flipVert * pixelPerRad * (*out)[1],
				((*out)[2] - zNear) * oneOverZNearMinusZFar);
		}
	}

	bool forward(Vec3f &v) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}
};

class StelProjectorFisheye : public StelProjector
{
public:
	StelProjectorFisheye(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 180.00001f;}
	bool forward(Vec3f &v) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}
};

class StelProjectorHammer : public StelProjector
{
public:
	StelProjectorHammer(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 360.f;}
	virtual void project(int n, const Vec3d* in, Vec3f* out)
	{
		Vec3d v;
		for (int i = 0; i < n; ++i)
		{
			v = in[i];
			modelViewTransform->forward(v);
			out[i].set(v[0], v[1], v[2]);
			StelProjectorHammer::forward(out[i]);
			out[i][0] = viewportCenter[0] + flipHorz * pixelPerRad * out[i][0];
			out[i][1] = viewportCenter[1] + flipVert * pixelPerRad * out[i][1];
			out[i][2] = (out[i][2] - zNear) * oneOverZNearMinusZFar;
		}
	}
	bool forward(Vec3f &v) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return true;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const {return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const
	{
		static const SphericalCap cap1(1,0,0);
		static const SphericalCap cap2(-1,0,0);
		static const SphericalCap cap3(0,0,-1);
		SphericalCap cap(capN, capD);
		return cap.intersects(cap1) && cap.intersects(cap2) && cap.intersects(cap3);
	}
};

class StelProjectorCylinder : public StelProjector
{
public:
	StelProjectorCylinder(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 175.f * 4.f/3.f;} // assume aspect ration of 4/3 for getting a full 360 degree horizon
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return true;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const
	{
		return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);
	}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const
	{
		static const SphericalCap cap1(1,0,0);
		static const SphericalCap cap2(-1,0,0);
		static const SphericalCap cap3(0,0,-1);
		SphericalCap cap(capN, capD);
		return cap.intersects(cap1) && cap.intersects(cap2) && cap.intersects(cap2);
	}
};

class StelProjectorMercator : public StelProjector
{
public:
	StelProjectorMercator(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 175.f * 4.f/3.f;} // assume aspect ration of 4/3 for getting a full 360 degree horizon
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return true;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const
	{
		return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);
	}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const
	{
		static const SphericalCap cap1(1,0,0);
		static const SphericalCap cap2(-1,0,0);
		static const SphericalCap cap3(0,0,-1);
		SphericalCap cap(capN, capD);
		return cap.intersects(cap1) && cap.intersects(cap2) && cap.intersects(cap2);
	}
};

class StelProjectorOrthographic : public StelProjector
{
public:
	StelProjectorOrthographic(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 179.9999f;}
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}
};

class StelProjectorSinusoidal : public StelProjectorCylinder
{
public:
	StelProjectorSinusoidal(ModelViewTranformP func) : StelProjectorCylinder(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
};

class StelProjectorMiller : public StelProjectorMercator
{
public:
	StelProjectorMiller(ModelViewTranformP func) : StelProjectorMercator(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 175.f * 4.f/3.f;} // or 180?
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
};

class StelProjector2d : public StelProjector
{
public:
	StelProjector2d() : StelProjector(ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity()))) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 360.f;}
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {Q_ASSERT(0); return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {Q_ASSERT(0); return false;}
	virtual void computeBoundingCap() {;}
};

#endif // STELPROJECTIONS_HPP

