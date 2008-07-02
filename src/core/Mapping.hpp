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

#ifndef _MAPPING_HPP_
#define _MAPPING_HPP_

#include "vecmath.h"
#include <QString>

class Mapping
{
public:
	Mapping(void);
	virtual ~Mapping(void) {}
	virtual QString getId() const = 0;
	virtual QString getNameI18() const = 0;
	virtual QString getDescriptionI18() const {return "No description";}
	QString getHtmlSummary() const;
	
	//! Apply the transformation in the forward direction
	//! After transformation v[2] will always contain the length
	//! of the original v: sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
	//! regardless of the projection type. This makes it possible to
	//! implement depth buffer testing in a way independent of the
	//! projection type. I would like to return the squared length
	//! instead of the length because of performance reasons.
	//! But then far away objects are not textured any more,
	//! perhaps because of a depth buffer overflow although
	//! the depth test is disabled?
	virtual bool forward(Vec3d &v) const = 0;
	virtual bool backward(Vec3d &v) const = 0;
	virtual double fovToViewScalingFactor(double fov) const = 0;
	virtual double viewScalingFactorToFov(double vsf) const = 0;
	virtual double deltaZoom(double fov) const = 0;
	//! Minimum FOV apperture in degree
	double minFov;
	//! Maximum FOV apperture in degree
	double maxFov;
};

#endif // _MAPPING_HPP_
