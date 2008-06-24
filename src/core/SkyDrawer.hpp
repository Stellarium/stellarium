/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#ifndef SKYDRAWER_HPP
#define SKYDRAWER_HPP

#include "STextureTypes.hpp"
#include "vecmath.h"

#include <QObject>

class Projector;
class ToneReproducer;
class StelCore;

//! @class SkyDrawer
//! Provide a set of methods used to draw sky objects taking into account
//! eyes adaptation, zoom level and instrument model
class SkyDrawer : public QObject
{
	Q_OBJECT;
public:
	//! Constructor
    SkyDrawer(StelCore* core);
	//! Destructor
    ~SkyDrawer();

	//! Init parameters from config file
	void init();
	
	//! Update with respect to the time and Projector/ToneReproducer state
	//! @param deltaTime the time increment in second since last call.
	void update(double deltaTime);
	
	//! Set the proper openGL state before making calls to drawPointSource
	void preDrawPointSource();
		
	//! Finalize the drawing of point sources
	void postDrawPointSource();
	
	//! Draw a point source halo.
	//! @param x the x position of the object on the screen
	//! @param y the y position of the object on the screen
	//! @param mag the source magnitude
	//! @param b_v the source B-V index
	//! @return true if the source was actually visible and drawn
	bool drawPointSource(double x, double y, const float rc_mag[2], unsigned int b_v)
		{return drawPointSource(x, y, rc_mag, colorTable[b_v]);}
	
	bool drawPointSource(double x, double y, const float rc_mag[2], const Vec3f& color);
	
	//! Draw a disk source halo. The real surface brightness is smaller as if it were a 
	//! point source because the flux is spread on the disk area
	//! @param x the x position of the disk center in pixel
	//! @param y the y position of the disk centre in pixel
	//! @param r radius of the disk in pixel
	//! @param mag the source integrated magnitude
	//! @param color the source RGB color
	//! @return true if the source was actually visible and drawn
	bool drawDiskSource(double x, double y, double r, float mag, const Vec3f& color);
	
	//! Set-up openGL lighting and color before drawing a 3d model.
	//! @param illuminatedArea the total illuminated area in arcmin^2
	//! @param mag the object integrated V magnitude
	//! @param lighting whether lighting computations should be activated for rendering
	void preDrawSky3dModel(double illuminatedArea, float mag, bool lighting=true);
	
	//! Terminate drawing of a 3D model, draw the halo
	//! @param x the x position of the object centroid in pixel
	//! @param y the y position of the object centroid in pixel
	//! @param color the object halo RGB color
	void postDrawSky3dModel(double x, double y, double illuminatedArea, float mag, const Vec3f& color = Vec3f(1.f,1.f,1.f));
	
	//! Compute RMag and CMag from magnitude.
	//! @param mag the object integrated V magnitude
	//! @param rc_mag array of 2 floats containing the radius and luminance
	//! @return false if the object is too faint to be displayed
	bool computeRCMag(float mag, float rc_mag[2]) const;
	
	//! Report that an object of luminance lum with an on-screen area of area pixels is currently displayed
	//! This information is used to determine the world adaptation luminance
	//! This method should be called during the update operations of the main loop
	//! @param lum luminance in cd/m^2
	void reportLuminanceInFov(double lum);
	
	//! To be called before the drawing stage starts
	void preDraw();
	
	//! Compute the luminance for an extended source with the given surface brightness
	//! @param sb Surface brightness in V magnitude/arcmin^2
	//! @return the luminance in cd/m^2
	static float surfacebrightnessToLuminance(float sb);
	
	//! Convert quantized B-V index to float B-V
	static inline float indexToBV(unsigned char b_v)
	{
		return (float)b_v*(4.f/127.f)-0.5f;
	}
	
	//! Convert quantized B-V index to RGB colors
	static inline const Vec3f& indexToColor(unsigned char b_v)
	{
		return colorTable[b_v];
	}
	
public slots:
	//! Set the way brighter stars will look bigger as the fainter ones
	void setRelativeStarScale(double b=1.0) {starRelativeScale=b;}
	//! Get the way brighter stars will look bigger as the fainter ones
	double getRelativeStarScale(void) const {return starRelativeScale;}
	
	//! Set the absolute star brightness scale
	void setAbsoluteStarScale(double b=1.0) {starAbsoluteScaleF=b;}
	//! Get the absolute star brightness scale
	double getAbsoluteStarScale(void) const {return starAbsoluteScaleF;}
	
	//! Set source twinkle amount.
	void setTwinkleAmount(double b) {twinkleAmount=b;}
	//! Get source twinkle amount.
	double getTwinkleAmount(void) const {return twinkleAmount;}
	
	//! Set flag for source twinkling.
	void setFlagTwinkle(bool b) {flagStarTwinkle=b;}
	//! Get flag for source twinkling.
	bool getFlagTwinkle(void) const {return flagStarTwinkle;}
	
	//! Set flag for displaying point sources as GLpoints (faster on some hardware but not so nice).
	void setFlagPointStar(bool b) {flagPointStar=b;}
	//! Get flag for displaying point sources as GLpoints (faster on some hardware but not so nice).
	bool getFlagPointStar(void) const {return flagPointStar;}
	
	//! Set the parameters so that the stars disapear at about the limit given by the bortle scale
	//! The limit is valid only at a given zoom level (around 60 deg)
	//! See http://en.wikipedia.org/wiki/Bortle_Dark-Sky_Scale
	void setBortleScale(int index);
	//! Get the current Bortle scale index
	int getBortleScale() const {return bortleScaleIndex;}
	
	//! Get the magnitude of the currently faintest visible point source
	//! It depends on the zoom level, on the eye adapation and on the point source rendering parameters
	//! @return the limit V mag at which a point source will be displayed
	float getLimitMagnitude() const {return limitMagnitude;}

private:
	// Debug
	float reverseComputeRCMag(float rmag) const;
	
	//! Compute the current limit magnitude by dichotomy
	float computeLimitMagnitude() const;
			
	//! Get SkyDrawer maximum FOV.
	float getMaxFov(void) const {return max_fov;}
	//! Set SkyDrawer maximum FOV.
	//! Usually stars/planet halos are drawn fainter when FOV gets larger, 
	//! but when FOV gets larger than this value, the stars do not become
	//! fainter any more. Must be >= 60.0.
	void setMaxFov(float fov) {max_fov = (fov < 60.f) ? 60.f : fov;}
	
	//! Get SkyDrawer minimum FOV.
	float getMinFov(void) const {return min_fov;}
	//! Set SkyDrawer minimum FOV.
	//! Usually stars/planet halos are drawn brighter when FOV gets smaller.
	//! But when FOV gets smaller than this value, the stars do not become
	//! brighter any more. Must be <= 60.0.
	void setMinFov(float fov) {min_fov = (fov > 60.f) ? 60.f : fov;}
	
	//! Set the scaling applied to input luminance before they are converted by the ToneReproducer
	void setInputScale(double in) {inScale = in;}
	//! Get the scaling applied to input luminance before they are converted by the ToneReproducer
	float getInputScale() const {return inScale;}
	
	//! Compute the luminance for a point source with the given mag for the current FOV
	//! @param mag V magnitude of the point source
	//! @return the luminance in log(cd/m^2)
	inline float pointSourceMagToLuminance(float mag) const {return std::exp(pointSourceMagToLnLuminance(mag));}
	
	//! Compute the V magnitude for a point source with the given luminance for the current FOV
	//! @param lum the luminance in cd/m^2
	//! @return V magnitude of the point source
	float pointSourceLuminanceToMag(float lum);
	
	//! Compute the log of the luminance for a point source with the given mag for the current FOV
	//! @param mag V magnitude of the point source
	//! @return the luminance in cd/m^2
	float pointSourceMagToLnLuminance(float mag) const;
	
	//! Find the world adaptation luminance to use so that a point source of magnitude mag
	//! is displayed with a halo of size targetRadius
	float findWorldLumForMag(float mag, float targetRadius);
			
	StelCore* core;
	Projector* prj;
	ToneReproducer* eye;
	
	float max_fov, min_fov, lnfov_factor;
	bool flagPointStar;
	bool flagStarTwinkle;
	float twinkleAmount;
	
	float starRelativeScale;
	float starAbsoluteScaleF;
	
	float starLinearScale;	// optimization variable
	
	//! Current magnitude limit for point sources
	float limitMagnitude;
	
	//! Little halo texture
	STextureSP texHalo;
	
	//! Load B-V conversion parameters from config file
	static void initColorTableFromConfigFile(class QSettings* conf);
	
	//! Contains the list of colors matching a given B-V index
	static Vec3f colorTable[128];
	
	//! The current Bortle Scale index
	int bortleScaleIndex;
	
	//! The scaling applied to input luminance before they are converted by the ToneReproducer
	double inScale;
	
	// Variables used for GL optimization when displaying point sources
	//! Buffer for storing the vertex array data
	Vec2f* verticesGrid;
	//! Buffer for storing the color array data
	Vec3f* colorGrid;
	//! Buffer for storing the texture coordinate array data
	Vec2f* textureGrid;
	//! Current number of sources stored in the buffers (still to display)
	unsigned int nbPointSources;
	//! Maximum number of sources which can be stored in the buffers
	unsigned int maxPointSources;
	
	//! The maximum transformed luminance to apply at the next update
	float maxLum;
	//! The previously used world luminance
	float oldLum;
	float maxLumDraw;
	
	float tempRCMag[2];
	
	//! Big halo texture
	STextureSP texBigHalo;
	STextureSP texSunHalo;
};

#endif
