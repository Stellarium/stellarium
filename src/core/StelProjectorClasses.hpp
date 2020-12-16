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
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const  Q_DECL_OVERRIDE{return 120.f;}
	virtual bool forward(Vec3f &v) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
	virtual float fovToViewScalingFactor(float fov) const Q_DECL_OVERRIDE;
	virtual float viewScalingFactorToFov(float vsf) const Q_DECL_OVERRIDE;
	virtual float deltaZoom(float fov) const Q_DECL_OVERRIDE;
protected:
	virtual bool hasDiscontinuity() const  Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const  Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const Q_DECL_OVERRIDE {return false;}
};

class StelProjectorEqualArea : public StelProjector
{
public:
	StelProjectorEqualArea(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const  Q_DECL_OVERRIDE{return 360.f;}
	virtual bool forward(Vec3f &v) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
	virtual float fovToViewScalingFactor(float fov) const Q_DECL_OVERRIDE;
	virtual float viewScalingFactorToFov(float vsf) const Q_DECL_OVERRIDE;
	virtual float deltaZoom(float fov) const Q_DECL_OVERRIDE;
protected:
	virtual bool hasDiscontinuity() const  Q_DECL_OVERRIDE{return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const  Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const  Q_DECL_OVERRIDE {return false;}
};

class StelProjectorStereographic : public StelProjector
{
public:
	StelProjectorStereographic(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const  Q_DECL_OVERRIDE {return 235.f;}
	virtual void project(int n, const Vec3d* in, Vec3f* out) Q_DECL_OVERRIDE
	{
		Vec3d v;
		for (int i = 0; i < n; ++i, ++out)
		{
			v = in[i];
			modelViewTransform->forward(v);
			out->set(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
			StelProjectorStereographic::forward(*out);
			out->set(static_cast<float>(viewportCenter[0]) + flipHorz * pixelPerRad * (*out)[0],
				static_cast<float>(viewportCenter[1]) + flipVert * pixelPerRad * (*out)[1],
				((*out)[2] - static_cast<float>(zNear)) * static_cast<float>(oneOverZNearMinusZFar));
		}
	}

	virtual bool forward(Vec3f &v) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
	virtual float fovToViewScalingFactor(float fov) const Q_DECL_OVERRIDE;
	virtual float viewScalingFactorToFov(float vsf) const Q_DECL_OVERRIDE;
	virtual float deltaZoom(float fov) const Q_DECL_OVERRIDE;
protected:
	virtual bool hasDiscontinuity() const Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const Q_DECL_OVERRIDE {return false;}
};

class StelProjectorFisheye : public StelProjector
{
public:
	StelProjectorFisheye(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const Q_DECL_OVERRIDE {return 360.0f;}
	virtual bool forward(Vec3f &v) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
	virtual float fovToViewScalingFactor(float fov) const Q_DECL_OVERRIDE;
	virtual float viewScalingFactorToFov(float vsf) const Q_DECL_OVERRIDE;
	virtual float deltaZoom(float fov) const Q_DECL_OVERRIDE;
protected:
	virtual bool hasDiscontinuity() const Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const Q_DECL_OVERRIDE {return false;}
};

class StelProjectorHammer : public StelProjector
{
public:
	StelProjectorHammer(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const Q_DECL_OVERRIDE {return 185.f;}
	virtual void project(int n, const Vec3d* in, Vec3f* out) Q_DECL_OVERRIDE
	{
		Vec3d v;
		for (int i = 0; i < n; ++i)
		{
			v = in[i];
			modelViewTransform->forward(v);
			out[i].set(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
			StelProjectorHammer::forward(out[i]);
			out[i][0] = static_cast<float>(viewportCenter[0]) + flipHorz * pixelPerRad * out[i][0];
			out[i][1] = static_cast<float>(viewportCenter[1]) + flipVert * pixelPerRad * out[i][1];
			out[i][2] = (out[i][2] - static_cast<float>(zNear)) * static_cast<float>(oneOverZNearMinusZFar);
		}
	}
	virtual bool forward(Vec3f &v) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
	virtual float fovToViewScalingFactor(float fov) const Q_DECL_OVERRIDE;
	virtual float viewScalingFactorToFov(float vsf) const Q_DECL_OVERRIDE;
	virtual float deltaZoom(float fov) const Q_DECL_OVERRIDE;
protected:
	virtual bool hasDiscontinuity() const Q_DECL_OVERRIDE {return true;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const Q_DECL_OVERRIDE {return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const Q_DECL_OVERRIDE
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
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const Q_DECL_OVERRIDE {return 200.f;} // slight overshoot
	virtual bool forward(Vec3f &win) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
	virtual float fovToViewScalingFactor(float fov) const Q_DECL_OVERRIDE;
	virtual float viewScalingFactorToFov(float vsf) const Q_DECL_OVERRIDE;
	virtual float deltaZoom(float fov) const Q_DECL_OVERRIDE;
protected:
	virtual bool hasDiscontinuity() const Q_DECL_OVERRIDE {return true;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const Q_DECL_OVERRIDE
	{
		return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);
	}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const Q_DECL_OVERRIDE
	{
		static const SphericalCap cap1(1,0,0);
		static const SphericalCap cap2(-1,0,0);
		static const SphericalCap cap3(0,0,-1);
		SphericalCap cap(capN, capD);
		return cap.intersects(cap1) && cap.intersects(cap2) && cap.intersects(cap3);
	}
};

class StelProjectorMercator : public StelProjector
{
public:
	StelProjectorMercator(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const Q_DECL_OVERRIDE {return 270.f; }
	virtual bool forward(Vec3f &win) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
	virtual float fovToViewScalingFactor(float fov) const Q_DECL_OVERRIDE;
	virtual float viewScalingFactorToFov(float vsf) const Q_DECL_OVERRIDE;
	virtual float deltaZoom(float fov) const Q_DECL_OVERRIDE;
protected:
	virtual bool hasDiscontinuity() const Q_DECL_OVERRIDE {return true;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const Q_DECL_OVERRIDE
	{
		return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);
	}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const Q_DECL_OVERRIDE
	{
		static const SphericalCap cap1(1,0,0);
		static const SphericalCap cap2(-1,0,0);
		static const SphericalCap cap3(0,0,-1);
		SphericalCap cap(capN, capD);
		return cap.intersects(cap1) && cap.intersects(cap2) && cap.intersects(cap3);
	}
};

class StelProjectorOrthographic : public StelProjector
{
public:
	StelProjectorOrthographic(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const Q_DECL_OVERRIDE {return 179.9999f;}
	virtual bool forward(Vec3f &win) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
	virtual float fovToViewScalingFactor(float fov) const Q_DECL_OVERRIDE;
	virtual float viewScalingFactorToFov(float vsf) const Q_DECL_OVERRIDE;
	virtual float deltaZoom(float fov) const Q_DECL_OVERRIDE;
protected:
	virtual bool hasDiscontinuity() const Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const Q_DECL_OVERRIDE {return false;}
};

class StelProjectorSinusoidal : public StelProjectorCylinder
{
public:
	StelProjectorSinusoidal(ModelViewTranformP func) : StelProjectorCylinder(func) {;}
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual bool forward(Vec3f &win) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
};

class StelProjectorMiller : public StelProjectorMercator
{
public:
	StelProjectorMiller(ModelViewTranformP func) : StelProjectorMercator(func) {;}
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const Q_DECL_OVERRIDE {return 270.f; }
	virtual bool forward(Vec3f &win) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
};

class StelProjector2d : public StelProjector
{
public:
	StelProjector2d() : StelProjector(ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity()))) {;}
	virtual QString getNameI18() const Q_DECL_OVERRIDE;
	virtual QString getDescriptionI18() const Q_DECL_OVERRIDE;
	virtual float getMaxFov() const Q_DECL_OVERRIDE {return 360.f;}
	virtual bool forward(Vec3f &win) const Q_DECL_OVERRIDE;
	virtual bool backward(Vec3d &v) const Q_DECL_OVERRIDE;
	virtual float fovToViewScalingFactor(float fov) const Q_DECL_OVERRIDE;
	virtual float viewScalingFactorToFov(float vsf) const Q_DECL_OVERRIDE;
	virtual float deltaZoom(float fov) const Q_DECL_OVERRIDE;
protected:
	virtual bool hasDiscontinuity() const Q_DECL_OVERRIDE {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const Q_DECL_OVERRIDE {Q_ASSERT(0); return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const Q_DECL_OVERRIDE {Q_ASSERT(0); return false;}
	virtual void computeBoundingCap() Q_DECL_OVERRIDE {;}
};

#endif // STELPROJECTIONS_HPP

