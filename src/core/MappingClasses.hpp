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

#ifndef _MAPPINGCLASSES_HPP_
#define _MAPPINGCLASSES_HPP_

#include "Mapping.hpp"

class MappingPerspective : public Mapping
{
public:
	MappingPerspective(void);
	virtual QString getId() const {return "perspective";}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 120.;}
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingPerspective instance;
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class MappingEqualArea : public Mapping
{
public:
    MappingEqualArea(void);
	virtual QString getId(void) const {return "equal_area";}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 360.;}
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingEqualArea instance;
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

class MappingStereographic : public Mapping
{
public:
	MappingStereographic(void);
	virtual QString getId(void) const {return "stereographic";}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 235.;}
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingStereographic instance;
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

class MappingFisheye : public Mapping
{
public:
	MappingFisheye(void);
	virtual QString getId(void) const {return "fisheye";}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 180.00001;}
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingFisheye instance;
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

class MappingCylinder : public Mapping
{
public:
	MappingCylinder(void);
	virtual QString getId(void) const {return "cylinder";}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 175. * 4./3.;} // assume aspect ration of 4/3 for getting a full 360 degree horizon
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingCylinder instance;
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class MappingMercator : public Mapping
{
public:
	MappingMercator(void);
	virtual QString getId(void) const {return "mercator";}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 175. * 4./3.;} // assume aspect ration of 4/3 for getting a full 360 degree horizon
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingMercator instance;
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class MappingOrthographic : public Mapping
{
public:
	MappingOrthographic(void);
	virtual QString getId(void) const {return "orthographic";}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual double getMaxFov() const {return 179.9999;}
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingOrthographic instance;
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

#endif // _MAPPINGCLASSES_HPP_

