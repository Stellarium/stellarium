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

class Projector;
class ToneReproducer;

//! @class SkyDrawer
//! Provide a set of methods used to draw sky objects taking into account
//! eyes adaptation, zoom level and instrument model
class SkyDrawer
{
public:
	//! Constructor
    SkyDrawer(Projector* prj, ToneReproducer* eye);
	//! Destructor
    ~SkyDrawer();

	//! Init parameters from config file
	void init();
	
	//! Update with respect to the time and Projector/ToneReproducer state
	//! @param deltaTime the time increment in second since last call.
	void update(double deltaTime);
	
	//! Set the proper openGL state before making calls to drawPointSource
	void prepareDraw();
	
	//! Draw a point source halo.
	//! @param x the x position of the object on the screen
	//! @param y the y position of the object on the screen
	//! @param mag the source magnitude
	//! @param b_v the source B-V
	//! @return true if the source was actually visible and drawn
	int drawPointSource(double x, double y, float mag, float b_v);
	int drawPointSource(double x, double y, const float rc_mag[2], unsigned int b_v);
	
	//! Draw a disk source halo.
	//! @param x the x position of the disk center in pixel
	//! @param y the y position of the disk centre in pixel
	//! @param r radius of the disk in pixel
	//! @param mag the source magnitude
	//! @param color the source RGB color
	//! @return true if the source was actually visible and drawn
	bool drawDiskSource(double x, double y, double r, float mag, const Vec3f& color);
	
	//! Set base source display scaling factor.
	void setScale(float b) {starScale=b;}
	//! Get base source display scaling factor.
	float getScale(void) const {return starScale;}
	
	//! Set source display scaling factor wrt magnitude.
	void setMagScale(float b) {starMagScale=b;}
	//! Get base source display scaling factor wrt magnitude.
	float getMagScale(void) const {return starMagScale;}
	
	//! Set source twinkle amount.
	void setTwinkleAmount(float b) {twinkleAmount=b;}
	//! Get source twinkle amount.
	float getTwinkleAmount(void) const {return twinkleAmount;}
	
	//! Set flag for source twinkling.
	void setFlagTwinkle(bool b) {flagStarTwinkle=b;}
	//! Get flag for source twinkling.
	bool getFlagTwinkle(void) const {return flagStarTwinkle;}
	
	//! Set flag for displaying point sources as GLpoints (faster on some hardware but not so nice).
	void setFlagPointStar(bool b) {flagPointStar=b;}
	//! Get flag for displaying point sources as GLpoints (faster on some hardware but not so nice).
	bool getFlagPointStar(void) const {return flagPointStar;}
	
	
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
	
	//! Get SkyDrawer magnitude shift.
	float getMagShift(void) const {return mag_shift;}
	//! Set SkyDrawer magnitude shift.
	//! draw the stars/planet halos as if they were brighter of fainter
	//! by this amount of magnitude
	void setMagShift(float d) {mag_shift = d;}
	
	//! Get SkyDrawer maximum magnitude.
	float getMaxMag(void) const {return max_mag;}
	//! Set SkyDrawer maximum magnitude.
	//! stars/planet halos, whose original (unshifted) magnitude is greater
	//! than this value will not be drawn.
	void setMaxMag(float mag) {max_mag = mag;}
	
	//! Get SkyDrawer maximum scaled magnitude wrt 60 degree FOV.
	float getMaxScaled60DegMag(void) const {return max_scaled_60deg_mag;}
	//! Set SkyDrawer maximum scaled magnitude wrt 60 degree FOV.
	//! Stars/planet halos, whose original (unshifted) magnitude is greater
	//! than this value will not be drawn at 60 degree FOV.
	void setMaxScaled60DegMag(float mag) {max_scaled_60deg_mag = mag;}
	

	//! Compute RMag and CMag from magnitude.
	int computeRCMag(float mag, float rc_mag[2]) const;
	
	//! Convert quantized B-V index to float B-V
	static inline float indexToBV(unsigned char b_v)
	{
		return (float)b_v*(4.f/127.f)-0.5f;
	}
	
	static inline const Vec3f& indexToColor(unsigned char b_v)
	{
		return colorTable[b_v];
	}
	
private:
	Projector* prj;
	ToneReproducer* eye;
	float max_fov, min_fov, mag_shift, max_mag, max_scaled_60deg_mag, min_rmag, lnfov_factor;
	float starScale;
	float starMagScale;
	bool flagPointStar;
	bool flagStarTwinkle;
	float twinkleAmount;
	
	//! Little halo texture
	STextureSP texHalo;
	
	//! Load B-V conversion parameters from config file
	static void initColorTableFromConfigFile(class QSettings* conf);
	
	//! Contains 
	static Vec3f colorTable[128];
};

#endif
