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
	StelProjectorPerspective(ModelViewTranformP func) : StelProjector(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const override {return 120.f;}
	bool forward(Vec3f &v) const override;
	bool backward(Vec3d &v) const override;
	float fovToViewScalingFactor(float fov) const override;
	float viewScalingFactorToFov(float vsf) const override;
	float deltaZoom(float fov) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
};

class StelProjectorEqualArea : public StelProjector
{
public:
	StelProjectorEqualArea(ModelViewTranformP func) : StelProjector(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const override{return 360.f;}
	bool forward(Vec3f &v) const override;
	bool backward(Vec3d &v) const override;
	float fovToViewScalingFactor(float fov) const override;
	float viewScalingFactorToFov(float vsf) const override;
	// float deltaZoom(float fov) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
};

class StelProjectorStereographic : public StelProjector
{
public:
	StelProjectorStereographic(ModelViewTranformP func) : StelProjector(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const  override {return 235.f;}
	bool forward(Vec3f &v) const override;
	bool backward(Vec3d &v) const override;
	float fovToViewScalingFactor(float fov) const override;
	float viewScalingFactorToFov(float vsf) const override;
	float deltaZoom(float fov) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
};

class StelProjectorFisheye : public StelProjector
{
public:
	StelProjectorFisheye(ModelViewTranformP func) : StelProjector(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const override {return 360.0f;}
	bool forward(Vec3f &v) const override;
	bool backward(Vec3d &v) const override;
	//float fovToViewScalingFactor(float fov) const override;
	//float viewScalingFactorToFov(float vsf) const override;
	//float deltaZoom(float fov) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
};

class StelProjectorHammer : public StelProjector
{
public:
	StelProjectorHammer(ModelViewTranformP func) : StelProjector(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const override {return 185.f;}
	bool forward(Vec3f &v) const override;
	bool backward(Vec3d &v) const override;
	//float fovToViewScalingFactor(float fov) const override;
	//float viewScalingFactorToFov(float vsf) const override;
	//float deltaZoom(float fov) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
protected:
	bool hasDiscontinuity() const override {return true;}
	bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const override {return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);}
	bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const override
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
	StelProjectorCylinder(ModelViewTranformP func) : StelProjector(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	virtual float getMaxFov() const override {return 185.f;} // slight overshoot
	bool forward(Vec3f &win) const override;
	bool backward(Vec3d &v) const override;
	//float fovToViewScalingFactor(float fov) const override;
	//float viewScalingFactorToFov(float vsf) const override;
	//float deltaZoom(float fov) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
protected:
	bool hasDiscontinuity() const override {return true;}
	bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const override
	{
		return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);
	}
	bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const override
	{
		static const SphericalCap cap1(1,0,0);
		static const SphericalCap cap2(-1,0,0);
		static const SphericalCap cap3(0,0,-1);
		SphericalCap cap(capN, capD);
		return cap.intersects(cap1) && cap.intersects(cap2) && cap.intersects(cap3);
	}
};

// A variant which fills the viewport with a plate carrée, regardless of screen dimensions.
class StelProjectorCylinderFill : public StelProjectorCylinder
{
public:
	StelProjectorCylinderFill(ModelViewTranformP func) : StelProjectorCylinder(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const override {return 180.f;} // vertical fov always max 180.
};

class StelProjectorMercator : public StelProjector
{
public:
	StelProjectorMercator(ModelViewTranformP func) : StelProjector(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const override {return 270.f; } // Despite being named 270 degrees this does not even show the 180° VFoV.
	bool forward(Vec3f &win) const override;
	bool backward(Vec3d &v) const override;
	//float fovToViewScalingFactor(float fov) const override;
	//float viewScalingFactorToFov(float vsf) const override;
	//float deltaZoom(float fov) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
protected:
	bool hasDiscontinuity() const override {return true;}
	bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const override
	{
		return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);
	}
	bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const override
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
	StelProjectorOrthographic(ModelViewTranformP func) : StelProjector(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const override {return 179.9999f;}
	bool forward(Vec3f &win) const override;
	bool backward(Vec3d &v) const override;
	float fovToViewScalingFactor(float fov) const override;
	float viewScalingFactorToFov(float vsf) const override;
	//float deltaZoom(float fov) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
};

class StelProjectorSinusoidal : public StelProjectorCylinder
{
public:
	StelProjectorSinusoidal(ModelViewTranformP func) : StelProjectorCylinder(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	bool forward(Vec3f &win) const override;
	bool backward(Vec3d &v) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
};

class StelProjectorMiller : public StelProjectorMercator
{
public:
	StelProjectorMiller(ModelViewTranformP func) : StelProjectorMercator(func) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const override {return 270.f; } // Show a slight overshoot of the full map.
	bool forward(Vec3f &win) const override;
	bool backward(Vec3d &v) const override;
	QByteArray getForwardTransformShader() const override;
	QByteArray getBackwardTransformShader() const override;
};

class StelProjector2d : public StelProjector
{
public:
	StelProjector2d() : StelProjector(ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity(),Mat4d::identity()))) {}
	QString getNameI18() const override;
	QString getDescriptionI18() const override;
	float getMaxFov() const override {return 360.f;}
	bool forward(Vec3f &win) const override;
	bool backward(Vec3d &v) const override;
	float fovToViewScalingFactor(float fov) const override;
	float viewScalingFactorToFov(float vsf) const override;
	float deltaZoom(float fov) const override;
	QByteArray getForwardTransformShader() const override;
	void setForwardTransformUniforms(QOpenGLShaderProgram& program) const override;
	QByteArray getBackwardTransformShader() const override;
	void setBackwardTransformUniforms(QOpenGLShaderProgram& program) const override;
protected:
	bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const override {Q_ASSERT(0); return false;}
	bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const override {Q_ASSERT(0); return false;}
	void computeBoundingCap() override {}
};

#endif // STELPROJECTIONS_HPP

