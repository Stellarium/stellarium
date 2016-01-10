/*
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

//! Class which computes the daylight sky color
//! Fast implementation of the algorithm from the article
//! "A Practical Analytic Model for Daylight" by A. J. Preetham, Peter Shirley and Brian Smits.

#ifndef SKYLIGHT_HPP
#define SKYLIGHT_HPP

#include "StelUtils.hpp"
#include "StelApp.hpp"

#include <cmath>
#include <QDebug>
#include <QObject>
#include <QSettings>

// set this to 1 to use values from the original paper, or 0 to use our own tweaked values.
#define PREETHAM_ORIGINAL 1

typedef struct {
	float pos[3];       //! Vector to the position (vertical = pos[2])
	float color[3];     //! 3 component color, can be RGB or CIE color system
} skylightStruct2;

// GZ We must derive from QObject now to set parameters via GUI
class Skylight: QObject
{
	Q_OBJECT
	Q_PROPERTY(float AYt  READ getAYt WRITE setAYt)
	Q_PROPERTY(float BYt  READ getBYt WRITE setBYt)
	Q_PROPERTY(float CYt  READ getCYt WRITE setCYt)
	Q_PROPERTY(float DYt  READ getDYt WRITE setDYt)
	Q_PROPERTY(float EYt  READ getEYt WRITE setEYt)
	Q_PROPERTY(float AYc  READ getAYc WRITE setAYc)
	Q_PROPERTY(float BYc  READ getBYc WRITE setBYc)
	Q_PROPERTY(float CYc  READ getCYc WRITE setCYc)
	Q_PROPERTY(float DYc  READ getDYc WRITE setDYc)
	Q_PROPERTY(float EYc  READ getEYc WRITE setEYc)

	Q_PROPERTY(float Axt  READ getAxt WRITE setAxt)
	Q_PROPERTY(float Bxt  READ getBxt WRITE setBxt)
	Q_PROPERTY(float Cxt  READ getCxt WRITE setCxt)
	Q_PROPERTY(float Dxt  READ getDxt WRITE setDxt)
	Q_PROPERTY(float Ext  READ getExt WRITE setExt)
	Q_PROPERTY(float Axc  READ getAxc WRITE setAxc)
	Q_PROPERTY(float Bxc  READ getBxc WRITE setBxc)
	Q_PROPERTY(float Cxc  READ getCxc WRITE setCxc)
	Q_PROPERTY(float Dxc  READ getDxc WRITE setDxc)
	Q_PROPERTY(float Exc  READ getExc WRITE setExc)

	Q_PROPERTY(float Ayt  READ getAyt WRITE setAyt)
	Q_PROPERTY(float Byt  READ getByt WRITE setByt)
	Q_PROPERTY(float Cyt  READ getCyt WRITE setCyt)
	Q_PROPERTY(float Dyt  READ getDyt WRITE setDyt)
	Q_PROPERTY(float Eyt  READ getEyt WRITE setEyt)
	Q_PROPERTY(float Ayc  READ getAyc WRITE setAyc)
	Q_PROPERTY(float Byc  READ getByc WRITE setByc)
	Q_PROPERTY(float Cyc  READ getCyc WRITE setCyc)
	Q_PROPERTY(float Dyc  READ getDyc WRITE setDyc)
	Q_PROPERTY(float Eyc  READ getEyc WRITE setEyc)

	Q_PROPERTY(float zX11  READ getZX11 WRITE setZX11)
	Q_PROPERTY(float zX12  READ getZX12 WRITE setZX12)
	Q_PROPERTY(float zX13  READ getZX13 WRITE setZX13)
	Q_PROPERTY(float zX21  READ getZX21 WRITE setZX21)
	Q_PROPERTY(float zX22  READ getZX22 WRITE setZX22)
	Q_PROPERTY(float zX23  READ getZX23 WRITE setZX23)
	Q_PROPERTY(float zX24  READ getZX24 WRITE setZX24)
	Q_PROPERTY(float zX31  READ getZX31 WRITE setZX31)
	Q_PROPERTY(float zX32  READ getZX32 WRITE setZX32)
	Q_PROPERTY(float zX33  READ getZX33 WRITE setZX33)
	Q_PROPERTY(float zX34  READ getZX34 WRITE setZX34)

	Q_PROPERTY(float zY11  READ getZY11 WRITE setZY11)
	Q_PROPERTY(float zY12  READ getZY12 WRITE setZY12)
	Q_PROPERTY(float zY13  READ getZY13 WRITE setZY13)
	Q_PROPERTY(float zY21  READ getZY21 WRITE setZY21)
	Q_PROPERTY(float zY22  READ getZY22 WRITE setZY22)
	Q_PROPERTY(float zY23  READ getZY23 WRITE setZY23)
	Q_PROPERTY(float zY24  READ getZY24 WRITE setZY24)
	Q_PROPERTY(float zY31  READ getZY31 WRITE setZY31)
	Q_PROPERTY(float zY32  READ getZY32 WRITE setZY32)
	Q_PROPERTY(float zY33  READ getZY33 WRITE setZY33)
	Q_PROPERTY(float zY34  READ getZY34 WRITE setZY34)


friend class AtmosphereDialog;

public:
	Skylight();
	virtual ~Skylight();
	//! Set the fixed parameters and precompute what can be
	//! This function has to be called once before any call to get_*_value()
	void setParams(float sunZenithAngle, float turbidity);

	//! Same functions but in vector mode : faster because prevents extra cosine calculations
	//! The position vectors MUST be normalized, and the vertical z component is the third one
	void setParamsv(const float * sunPos, float turbidity);
	
	// Compute the sky color at the given position in the xyY color system and store it in position.color
	// void getxyYValue(skylightStruct * position);
	//! Return the current zenith color in the xyY color system
	//! @param v 3-element vector to receive x, y, Y values
	inline void getZenithColor(float * v) const;

	//! Compute the sky color at the given position in the CIE color system and store it in p.color
	//! @param p.color[0] is CIE x color component
	//! @param p.color[1] is CIE y color component
	//! @param p.color[2] is undefined (CIE Y color component (luminance) if uncommented)
	void getxyYValuev(skylightStruct2& p) const
	{
		const float cosDistSun = sunPos[0]*p.pos[0] + sunPos[1]*p.pos[1] + sunPos[2]*p.pos[2];
		const float distSun = StelUtils::fastAcos(cosDistSun);
		const float cosDistSun_q = cosDistSun*cosDistSun;

		Q_ASSERT(p.pos[2] >= 0.f);
		const float oneOverCosZenithAngle = (p.pos[2]==0.f) ? 1e38f : 1.f / p.pos[2];
		p.color[0] = term_x * (1.f + Ax * std::exp(Bx*oneOverCosZenithAngle))
				* (1.f + Cx * std::exp(Dx*distSun) + Ex * cosDistSun_q);

		p.color[1] = term_y * (1.f + Ay * std::exp(By*oneOverCosZenithAngle))
				* (1.f + Cy * std::exp(Dy*distSun) + Ey * cosDistSun_q);

// 		p.color[2] = term_Y * (1.f + AY * std::exp(BY*oneOverCosZenithAngle))
// 				* (1.f + CY * std::exp(DY*distSun) + EY * cosDistSun_q);


		if (/*p.color[2] < 0. || */p.color[0] < 0.f || p.color[1] < 0.f)
		{
			p.color[0] = 0.25;
			p.color[1] = 0.25;
			p.color[2] = 0.;
		}
	}

	void getShadersParams(Vec3f& asunPos, float& aterm_x, float& aAx, float& aBx, float& aCx, float& aDx, float& aEx,
		float& aterm_y, float& aAy, float& aBy, float& aCy, float& aDy, float& aEy) const
	{
		asunPos=sunPos;
		aterm_x=term_x;aAx=Ax;aBx=Bx;aCx=Cx;aDx=Dx;aEx=Ex;
		aterm_y=term_y;aAy=Ay;aBy=By;aCy=Cy;aDy=Dy;aEy=Ey;
	}
	
	// GZ new: reset all parameters...
//	void resetPreethamData()
//	{ // TODO put back the values.
//		AYt=newAYt; BYt=newBYt; CYt=newCYt; DYt=newDYt; EYt=newEYt; AYc=newAYc; BYc=newBYc; CYc=newCYc; DYc=newDYc; EYc=newEYc;
//		Axt=newAxt; Bxt=newBxt; Cxt=newCxt; Dxt=newDxt; Ext=newExt; Axc=newAxc; Bxc=newBxc; Cxc=newCxc; Dxc=newDxc; Exc=newExc;
//		Ayt=newAyt; Byt=newByt; Cyt=newCyt; Dyt=newDyt; Eyt=newEyt; Ayc=newAyc; Byc=newByc; Cyc=newCyc; Dyc=newDyc; Eyc=newEyc;
//	}
	
public slots:
	void setAYt(const double newVal){AYt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/AYt", newVal); computeLuminanceDistributionCoefs();}
	void setBYt(const double newVal){BYt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/BYt", newVal); computeLuminanceDistributionCoefs();}
	void setCYt(const double newVal){CYt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/CYt", newVal); computeLuminanceDistributionCoefs();}
	void setDYt(const double newVal){DYt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/DYt", newVal); computeLuminanceDistributionCoefs();}
	void setEYt(const double newVal){EYt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/EYt", newVal); computeLuminanceDistributionCoefs();}
	void setAYc(const double newVal){AYc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/AYc", newVal); computeLuminanceDistributionCoefs();}
	void setBYc(const double newVal){BYc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/BYc", newVal); computeLuminanceDistributionCoefs();}
	void setCYc(const double newVal){CYc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/CYc", newVal); computeLuminanceDistributionCoefs();}
	void setDYc(const double newVal){DYc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/DYc", newVal); computeLuminanceDistributionCoefs();}
	void setEYc(const double newVal){EYc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/EYc", newVal); computeLuminanceDistributionCoefs();}
	void setAxt(const double newVal){Axt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Axt", newVal); computeColorDistributionCoefs();}
	void setBxt(const double newVal){Bxt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Bxt", newVal); computeColorDistributionCoefs();}
	void setCxt(const double newVal){Cxt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Cxt", newVal); computeColorDistributionCoefs();}
	void setDxt(const double newVal){Dxt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Dxt", newVal); computeColorDistributionCoefs();}
	void setExt(const double newVal){Ext=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Ext", newVal); computeColorDistributionCoefs();}
	void setAxc(const double newVal){Axc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Axc", newVal); computeColorDistributionCoefs();}
	void setBxc(const double newVal){Bxc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Bxc", newVal); computeColorDistributionCoefs();}
	void setCxc(const double newVal){Cxc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Cxc", newVal); computeColorDistributionCoefs();}
	void setDxc(const double newVal){Dxc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Dxc", newVal); computeColorDistributionCoefs();}
	void setExc(const double newVal){Exc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Exc", newVal); computeColorDistributionCoefs();}
	void setAyt(const double newVal){Ayt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Ayt", newVal); computeColorDistributionCoefs();}
	void setByt(const double newVal){Byt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Byt", newVal); computeColorDistributionCoefs();}
	void setCyt(const double newVal){Cyt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Cyt", newVal); computeColorDistributionCoefs();}
	void setDyt(const double newVal){Dyt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Dyt", newVal); computeColorDistributionCoefs();}
	void setEyt(const double newVal){Eyt=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Eyt", newVal); computeColorDistributionCoefs();}
	void setAyc(const double newVal){Ayc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Ayc", newVal); computeColorDistributionCoefs();}
	void setByc(const double newVal){Byc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Byc", newVal); computeColorDistributionCoefs();}
	void setCyc(const double newVal){Cyc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Cyc", newVal); computeColorDistributionCoefs();}
	void setDyc(const double newVal){Dyc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Dyc", newVal); computeColorDistributionCoefs();}
	void setEyc(const double newVal){Eyc=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/Eyc", newVal); computeColorDistributionCoefs();}
	void setZX11(const double newVal){zX11=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX11", newVal); computeZenithColor();}
	void setZX12(const double newVal){zX12=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX12", newVal); computeZenithColor();}
	void setZX13(const double newVal){zX13=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX13", newVal); computeZenithColor();}
	void setZX21(const double newVal){zX21=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX21", newVal); computeZenithColor();}
	void setZX22(const double newVal){zX22=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX22", newVal); computeZenithColor();}
	void setZX23(const double newVal){zX23=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX23", newVal); computeZenithColor();}
	void setZX24(const double newVal){zX24=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX24", newVal); computeZenithColor();}
	void setZX31(const double newVal){zX31=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX31", newVal); computeZenithColor();}
	void setZX32(const double newVal){zX32=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX32", newVal); computeZenithColor();}
	void setZX33(const double newVal){zX33=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX33", newVal); computeZenithColor();}
	void setZX34(const double newVal){zX34=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZX34", newVal); computeZenithColor();}
	void setZY11(const double newVal){zY11=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY11", newVal); computeZenithColor();}
	void setZY12(const double newVal){zY12=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY12", newVal); computeZenithColor();}
	void setZY13(const double newVal){zY13=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY13", newVal); computeZenithColor();}
	void setZY21(const double newVal){zY21=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY21", newVal); computeZenithColor();}
	void setZY22(const double newVal){zY22=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY22", newVal); computeZenithColor();}
	void setZY23(const double newVal){zY23=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY23", newVal); computeZenithColor();}
	void setZY24(const double newVal){zY24=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY24", newVal); computeZenithColor();}
	void setZY31(const double newVal){zY31=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY31", newVal); computeZenithColor();}
	void setZY32(const double newVal){zY32=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY32", newVal); computeZenithColor();}
	void setZY33(const double newVal){zY33=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY33", newVal); computeZenithColor();}
	void setZY34(const double newVal){zY34=newVal; QSettings* conf = StelApp::getInstance().getSettings(); conf->setValue("Skylight/ZY34", newVal); computeZenithColor();}

	float getAYt(void) const {return AYt; }
	float getBYt(void) const {return BYt; }
	float getCYt(void) const {return CYt; }
	float getDYt(void) const {return DYt; }
	float getEYt(void) const {return EYt; }
	float getAYc(void) const {return AYc; }
	float getBYc(void) const {return BYc; }
	float getCYc(void) const {return CYc; }
	float getDYc(void) const {return DYc; }
	float getEYc(void) const {return EYc; }
	float getAxt(void) const {return Axt; }
	float getBxt(void) const {return Bxt; }
	float getCxt(void) const {return Cxt; }
	float getDxt(void) const {return Dxt; }
	float getExt(void) const {return Ext; }
	float getAxc(void) const {return Axc; }
	float getBxc(void) const {return Bxc; }
	float getCxc(void) const {return Cxc; }
	float getDxc(void) const {return Dxc; }
	float getExc(void) const {return Exc; }
	float getAyt(void) const {return Ayt; }
	float getByt(void) const {return Byt; }
	float getCyt(void) const {return Cyt; }
	float getDyt(void) const {return Dyt; }
	float getEyt(void) const {return Eyt; }
	float getAyc(void) const {return Ayc; }
	float getByc(void) const {return Byc; }
	float getCyc(void) const {return Cyc; }
	float getDyc(void) const {return Dyc; }
	float getEyc(void) const {return Eyc; }

	float getZX11(void) const {return zX11; }
	float getZX12(void) const {return zX12; }
	float getZX13(void) const {return zX13; }
	float getZX21(void) const {return zX21; }
	float getZX22(void) const {return zX22; }
	float getZX23(void) const {return zX23; }
	float getZX24(void) const {return zX24; }
	float getZX31(void) const {return zX31; }
	float getZX32(void) const {return zX32; }
	float getZX33(void) const {return zX33; }
	float getZX34(void) const {return zX34; }

	float getZY11(void) const {return zY11; }
	float getZY12(void) const {return zY12; }
	float getZY13(void) const {return zY13; }
	float getZY21(void) const {return zY21; }
	float getZY22(void) const {return zY22; }
	float getZY23(void) const {return zY23; }
	float getZY24(void) const {return zY24; }
	float getZY31(void) const {return zY31; }
	float getZY32(void) const {return zY32; }
	float getZY33(void) const {return zY33; }
	float getZY34(void) const {return zY34; }

	float getT(void) const {return T;}
	void setT(float newT){T=newT;}

private:
	float thetas;  // angular distance between the zenith and the sun in radian
	float T;       // Turbidity : i.e. sky "clarity"
	               //  1 : pure air
	               //  2 : exceptionnally clear
	               //  4 : clear
	               //  8 : light haze
	               // 25 : haze
	               // 64 : thin fog

	// Computed variables depending on the 2 above
	float zenithLuminance;     // Y color component of the CIE color at zenith (luminance)
	float zenithColorX;        // x color component of the CIE color at zenith
	float zenithColorY;        // y color component of the CIE color at zenith

	float eyeLumConversion;    // luminance conversion for an eye adapted to screen luminance 
	                           // (around 40 cd/m^2)

	float AY, BY, CY, DY, EY;  // Distribution coefficients for the luminance distribution function
	float Ax, Bx, Cx, Dx, Ex;  // Distribution coefficients for x distribution function
	float Ay, By, Cy, Dy, Ey;  // Distribution coefficients for y distribution function

	// GZ Experimental: make fully GUI configurable parameters to play! Naming: AY=AYt*T+AYc, etc. (i.e., Tfactor, Constant)
	float AYt, BYt, CYt, DYt, EYt;  // Distribution coefficients for the luminance distribution function
	float Axt, Bxt, Cxt, Dxt, Ext;  // Distribution coefficients for x distribution function
	float Ayt, Byt, Cyt, Dyt, Eyt;  // Distribution coefficients for y distribution function
	float AYc, BYc, CYc, DYc, EYc;  // Distribution coefficients for the luminance distribution function
	float Axc, Bxc, Cxc, Dxc, Exc;  // Distribution coefficients for x distribution function
	float Ayc, Byc, Cyc, Dyc, Eyc;  // Distribution coefficients for y distribution function

	float zX11, zX12, zX13;
	float zX21, zX22, zX23, zX24;
	float zX31, zX32, zX33, zX34;
	float zY11, zY12, zY13;
	float zY21, zY22, zY23, zY24;
	float zY31, zY32, zY33, zY34;

	float term_x;              // Precomputed term for x calculation
	float term_y;              // Precomputed term for y calculation
	float term_Y;              // Precomputed term for luminance calculation. We will actually not use it because luminance comes from SkyBright (Schaefer's model)

	float sunPos[3];

	// Compute CIE Y (luminance) for zenith in cd/m^2
	inline void computeZenithLuminance(void);
	// Compute CIE x and y color components
	inline void computeZenithColor(void);
	// Compute the luminance distribution coefficients
	inline void computeLuminanceDistributionCoefs(void);
	// Compute the color distribution coefficients
	inline void computeColorDistributionCoefs(void);
};

// Return the current zenith color in xyY color system
inline void Skylight::getZenithColor(float * v) const
{
	v[0] = zenithColorX;
	v[1] = zenithColorY;
	v[2] = zenithLuminance;
}

// Compute CIE luminance for zenith in cd/m^2
inline void Skylight::computeZenithLuminance(void)
{
	zenithLuminance = 1000.f * ((4.0453f*T - 4.9710f) * std::tan( (0.4444f - T*(1.f/120.f)) * (M_PIf-2.f*thetas) ) -
		0.2155f*T + 2.4192f);
	zenithLuminance=qMax(zenithLuminance, 0.00000000001f);
}


// Compute CIE x and y color components
// Edit: changed some coefficients to get new sky color
// GZ: 2016-01 changed back to original Preetham values.
// GZ: 2016-01b made them configurable with 2 presets: Preetham and Stellarium.
inline void Skylight::computeZenithColor(void)
{

#ifdef PREETHAM_ORIGINAL
//	zenithColorX = (( ( (( 0.00166f*thetas - 0.00375f)*thetas + 0.00209f)*thetas)             * T
//			   +((-0.02903f*thetas + 0.06377f)*thetas - 0.03202f)*thetas + 0.00394f)  * T
//			   +(( 0.11693f*thetas - 0.21196f)*thetas + 0.06052f)*thetas + 0.25886f);
//	zenithColorY = (( ( (( 0.00275f*thetas - 0.00610f)*thetas + 0.00317f)*thetas)             * T
//			   +((-0.04214f*thetas + 0.08970f)*thetas - 0.04153f)*thetas + 0.00516f)  * T
//			   +(( 0.15346f*thetas - 0.26756f)*thetas + 0.06670f)*thetas + 0.26688f);

	zenithColorX=(( ( (( zX11*thetas + zX12)*thetas + zX13)*thetas)       * T
					+ (( zX21*thetas + zX22)*thetas + zX23)*thetas + zX24)* T
				  +((    zX31*thetas + zX32)*thetas + zX33)*thetas + zX34);
	zenithColorY=(( ( (( zY11*thetas + zY12)*thetas + zY13)*thetas)       * T
					+ (( zY21*thetas + zY22)*thetas + zY23)*thetas + zY24)* T
				  +((    zY31*thetas + zY32)*thetas + zY33)*thetas + zY34);

#else
	static float thetas2;
	static float thetas3;
	static float T2;

	thetas2 = thetas * thetas;
	thetas3 = thetas2 * thetas;
	T2 = T * T;
	zenithColorX = ( 0.00216f*thetas3 - 0.00375f*thetas2 + 0.00209f*thetas) * T2 +
	               (-0.02903f*thetas3 + 0.06377f*thetas2 - 0.03202f*thetas + 0.00394f) * T +
	               ( 0.10169f*thetas3 - 0.21196f*thetas2 + 0.06052f*thetas + 0.25886f);

	zenithColorY = ( 0.00275f*thetas3 - 0.00610f*thetas2 + 0.00317f*thetas) * T2 +
	               (-0.04214f*thetas3 + 0.08970f*thetas2 - 0.04153f*thetas + 0.00516f) * T +
	               ( 0.14535f*thetas3 - 0.26756f*thetas2 + 0.06670f*thetas + 0.26688f);
#endif
}

// Compute the luminance distribution coefficients
// FC Edit 2003(?) changed some coefficients to get new sky color
// GZ: 2016-01 changed back to original Preetham values.
// GZ: 2016-01b made them configurable with 2 presets: Preetham and Stellarium.
inline void Skylight::computeLuminanceDistributionCoefs(void)
{
	AY=AYt*T+AYc;
	BY=BYt*T+BYc; BY=qMin(0.0f, BY);
	CY=CYt*T+CYc;
	DY=DYt*T+DYc;
	EY=EYt*T+EYc;

//#ifdef PREETHAM_ORIGINAL
//	AY = 0.1787f*T - 1.4630f; // Original
//	CY =-0.0227f*T + 5.3251f; // Original
//#else
//	AY = 0.2787f*T - 1.0630f; // FC values
//	CY =-0.0227f*T + 6.3251f; // FC values
//#endif
//	BY =-0.3554f*T + 0.4275f;
//	DY = 0.1206f*T - 2.5771f;
//	EY =-0.0670f*T + 0.3703f;
//	// with BY>0 the formulas in getxyYValuev make no sense, from which follows T>0.4275/0.3554(=1.203)

	// GZ For the experiments this is dangerous...
	//Q_ASSERT(BY <= 0.0);
}

// Compute the color distribution coefficients
// FC Edit 2003(?) changed some coefficients to get new sky color
// GZ: TODO 2016-01 find and change back to original Preetham values.
inline void Skylight::computeColorDistributionCoefs(void)
{
	Ax=Axt*T+Axc;
	Bx=Bxt*T+Bxc; Bx=qMin(0.0f, Bx);
	Cx=Cxt*T+Cxc;
	Dx=Dxt*T+Dxc;
	Ex=Ext*T+Exc;
	Ay=Ayt*T+Ayc;
	By=Byt*T+Byc; By=qMin(0.0f, By);
	Cy=Cyt*T+Cyc;
	Dy=Dyt*T+Dyc;
	Ey=Eyt*T+Eyc;

//#ifdef PREETHAM_ORIGINAL
//	Ax =-0.0193f*T - 0.2592f;
//	Bx =-0.0665f*T + 0.0008f;
//	Cx =-0.0004f*T + 0.2125f;
//	Dx =-0.0641f*T - 0.8989f;
//	Ex =-0.0033f*T + 0.0452f;

//	Ay =-0.0167f*T - 0.2608f;
//	By =-0.0950f*T + 0.0092f;
//	Cy =-0.0079f*T + 0.2102f;
//	Dy =-0.0441f*T - 1.6537f;
//	Ey =-0.0109f*T + 0.0529f;
//#else
//	Ax =-0.0148f*T - 0.1703f;
//	Bx =-0.0664f*T + 0.0011f;
//	Cx =-0.0005f*T + 0.2127f;
//	Dx =-0.0641f*T - 0.8992f;
//	Ex =-0.0035f*T + 0.0453f;

//	Ay =-0.0131f*T - 0.2498f;
//	By =-0.0951f*T + 0.0092f;
//	Cy =-0.0082f*T + 0.2404f;
//	Dy =-0.0438f*T - 1.0539f;
//	Ey =-0.0109f*T + 0.0531f;
//#endif

	// with Bx,By>0 the formulas in getxyYValuev make no sense. --> T>0.0011/0.0664=0.017; T>0.0092/0.0951=0.097
	// GZ Deactivated for experimenting...
	//Q_ASSERT(Bx <= 0.0);
	//Q_ASSERT(By <= 0.0);
}

#endif // SKYLIGHT_HPP

