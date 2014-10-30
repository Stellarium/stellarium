/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2014 Georg Zotti: ZodiacalLight
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

#ifndef _ZODIACALLIGHT_
#define _ZODIACALLIGHT_

#include <QVector>
#include "StelModule.hpp"
#include "VecMath.hpp"
#include "StelTextureTypes.hpp"

//! @class ZodiacalLight 
//! Manages the displaying of the Zodiacal Light. The brightness values follow the paper:
//! S. M. Kwon, S. S. Hong, J. L. Weinberg
//! An observational model of the zzodiacal light brightness distribution
//! New Astronomy 10 (2004) 91-107. doi:10.1016/j.newast.2004.05.004
class ZodiacalLight : public StelModule
{
	Q_OBJECT

public:
	ZodiacalLight();
	virtual ~ZodiacalLight();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the class.  Here we load the texture for the Zodiacal Light and 
	//! get the display settings from application settings, namely the flag which
	//! determines if the Zodiacal Light is displayed or not, and the intensity setting.
	virtual void init();

	//! Draw the Zodiacal Light.
	virtual void draw(StelCore* core);
	
	//! Update and time-dependent state.  Updates the fade level while the 
	//! Zodiacal Light rendering is being changed from on to off or off to on.
	virtual void update(double deltaTime);
	
	//! Used to determine the order in which the various modules are drawn. MilkyWay=1, we use 6.
	virtual double getCallOrder(StelModuleActionName actionName) const {Q_UNUSED(actionName); return 6.;}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Setter and getters
public slots:
	//! Get Zodiacal Light intensity.
	double getIntensity() const {return intensity;}
	//! Set Zodiacal Light intensity.
	void setIntensity(double aintensity) {intensity = aintensity;}
	
	//! Get the color used for rendering the Zodiacal Light
	Vec3f getColor() const {return color;}
	//! Sets the color to use for rendering the Zodiacal Light
	void setColor(const Vec3f& c) {color=c;}
	
	//! Sets whether to show the Zodiacal Light
	void setFlagShow(bool b);
	//! Gets whether the Zodiacal Light is displayed
	bool getFlagShow(void) const;
	
private:
	StelTextureSP tex;
	Vec3f color; // global color
	float intensity;
	class LinearFader* fader;
	double lastJD; // keep date of last computation. Position will be updated only if far enough away from last computation.

	struct StelVertexArray* vertexArray;
	QVector<Vec3d> eclipticalVertices;
};

#endif // _ZODIACALLIGHT_HPP_
