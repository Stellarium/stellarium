/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2014 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#ifndef _S3DENUM_HPP_
#define _S3DENUM_HPP_

#include <QObject>

//! @file This file contains some enums useful throughout the Scenery3d plugin (settings, etc)

struct S3DEnum
{
	Q_GADGET
	Q_ENUMS(CubemappingMode ShadowFilterQuality)

public:
	//! Determines the method used for cubemap creation
	enum CubemappingMode
	{
		//! Uses 6 textures, one for each side of the cube. Seems to be the best for old Intel drivers.
		TEXTURES,
		//! Uses a single GL_TEXTURE_CUBEMAP, seems to work a bit better on "modern" GPUs
		CUBEMAP,
		//! Uses a single GL_TEXTURE_CUBEMAP and a geometry shader to render all 6 sides in one pass.
		CUBEMAP_GSACCEL
	};

	//! Contains different shadow filter settings
	enum ShadowFilterQuality
	{
		//! Disables shadow filtering (hardware PCF is still applied)
		OFF,
		//! Uses a 16-tap Poisson disk
		LOW,
		//! Uses a 64-tap Poisson disk
		HIGH
	};
};

#endif
