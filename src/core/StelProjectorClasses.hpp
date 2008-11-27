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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _PROJECTIONS_HPP_
#define _PROJECTIONS_HPP_

#include "StelProjector.hpp"

class StelProjectorPerspective : public StelProjector
{
public:
	StelProjectorPerspective(const Mat4d& modelViewMat) : StelProjector(modelViewMat) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 120.;}
	bool forward(Vec3d &v) const
	{
		const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		if (v[2] < 0) {
			v[0] /= (-v[2]);
			v[1] /= (-v[2]);
			v[2] = r;
			return true;
		}
		if (v[2] > 0) {
			v[0] /= v[2];
			v[1] /= v[2];
			v[2] = r;
			return false;
		}
		v[0] *= 1e99;
		v[1] *= 1e99;
		v[2] = r;
		return false;
	}
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class StelProjectorEqualArea : public StelProjector
{
public:
	StelProjectorEqualArea(const Mat4d& modelViewMat) : StelProjector(modelViewMat) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 360.;}
	bool forward(Vec3d &v) const
	{
		const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		const double f = std::sqrt(2.0/(r*(r-v[2])));
		v[0] *= f;
		v[1] *= f;
		v[2] = r;
		return true;
	}
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class StelProjectorStereographic : public StelProjector
{
public:
	StelProjectorStereographic(const Mat4d& modelViewMat) : StelProjector(modelViewMat) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 235.;}
	bool forward(Vec3d &v) const
	{
		const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		const double h = 0.5*(r-v[2]);
		if (h <= 0.0) {
			v[0] = 1e99;
			v[1] = 1e99;
			v[2] = -1e99;
			return false;
		}
		const double f = 1.0 / h;
		v[0] *= f;
		v[1] *= f;
		v[2] = r;
		return true;
	}
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class StelProjectorFisheye : public StelProjector
{
public:
	StelProjectorFisheye(const Mat4d& modelViewMat) : StelProjector(modelViewMat) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 180.00001;}
	bool forward(Vec3d &v) const
	{
		const double rq1 = v[0]*v[0] + v[1]*v[1];
		if (rq1 > 0.0) {
			const double h = std::sqrt(rq1);
			const double f = std::atan2(h,-v[2]) / h;
			v[0] *= f;
			v[1] *= f;
			v[2] = std::sqrt(rq1 + v[2]*v[2]);
			return true;
		}
		if (v[2] < 0.0) {
			v[0] = 0.0;
			v[1] = 0.0;
			v[2] = 1.0;
			return true;
		}
		v[0] = 1e99;
		v[1] = 1e99;
		v[2] = -1e99;
		return false;
	}
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class StelProjectorCylinder : public StelProjector
{
public:
	StelProjectorCylinder(const Mat4d& modelViewMat) : StelProjector(modelViewMat) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 175. * 4./3.;} // assume aspect ration of 4/3 for getting a full 360 degree horizon
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class StelProjectorMercator : public StelProjector
{
public:
	StelProjectorMercator(const Mat4d& modelViewMat) : StelProjector(modelViewMat) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 175. * 4./3.;} // assume aspect ration of 4/3 for getting a full 360 degree horizon
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class StelProjectorOrthographic : public StelProjector
{
public:
	StelProjectorOrthographic(const Mat4d& modelViewMat) : StelProjector(modelViewMat) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 179.9999;}
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class StelProjector2d : public StelProjector
{
public:
	StelProjector2d() : StelProjector(Mat4d::identity()) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 360.;}
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

#endif // _PROJECTIONS_HPP_

