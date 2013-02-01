/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _MILKYWAY_HPP_
#define _MILKYWAY_HPP_

#include "StelModule.hpp"
#include "VecMath.hpp"
#include "renderer/GenericVertexTypes.hpp"
#include "renderer/StelIndexBuffer.hpp"
#include "renderer/StelVertexBuffer.hpp"

//! @class MilkyWay 
//! Manages the displaying of the Milky Way.
class MilkyWay : public StelModule
{
	Q_OBJECT

public:
	MilkyWay();
	virtual ~MilkyWay();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the class.  Here we load the texture for the Milky Way and 
	//! get the display settings from application settings, namely the flag which
	//! determines if the Milky Way is displayed or not, and the intensity setting.
	virtual void init();

	//! Draw the Milky Way.
	//!
	//! @param core     The StelCore object.
	//! @param renderer Renderer to draw with.
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	
	//! Update and time-dependent state.  Updates the fade level while the 
	//! Milky way rendering is being changed from on to off or off to on.
	virtual void update(double deltaTime);
	
	//! Used to determine the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const {Q_UNUSED(actionName); return 1.;}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Setter and getters
public slots:
	//! Get Milky Way intensity.
	double getIntensity() const {return intensity;}
	//! Set Milky Way intensity.
	void setIntensity(double aintensity) {intensity = aintensity;}
	
	//! Get the color used for rendering the milky way
	Vec3f getColor() const {return color;}
	//! Sets the color to use for rendering the milky way
	void setColor(const Vec3f& c) {color=c;}
	
	//! Sets whether to show the Milky Way
	void setFlagShow(bool b);
	//! Gets whether the Milky Way is displayed
	bool getFlagShow(void) const;
	
private:
	float radius;
	class StelTextureNew* tex;
	Vec3f color;
	float intensity;
	class LinearFader* fader;

	//! Sphere used to draw the Milky Way.
	class StelGeometrySphere* skySphere;
};

#endif // _MILKYWAY_HPP_
