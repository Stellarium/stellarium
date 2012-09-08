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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELSKYDRAWER_HPP_
#define _STELSKYDRAWER_HPP_

#include "StelProjectorType.hpp"
#include "VecMath.hpp"
#include "RefractionExtinction.hpp"

#include "renderer/StelIndexBuffer.hpp"
#include "renderer/StelVertexBuffer.hpp"

#include <QObject>


class StelToneReproducer;
class StelCore;

//! @class StelSkyDrawer
//! Provide a set of methods used to draw sky objects taking into account
//! eyes adaptation, zoom level and instrument model
class StelSkyDrawer : public QObject
{
	Q_OBJECT
public:
	//! Constructor
	StelSkyDrawer(StelCore* core, class StelRenderer* renderer);
	//! Destructor
	~StelSkyDrawer();

	//! Init parameters from config file
	void init();

	//! Update with respect to the time and StelProjector/StelToneReproducer state
	//! @param deltaTime the time increment in second since last call.
	void update(double deltaTime);

	//! Prepare to draw point sources (must be called before drawing).
	void preDrawPointSource();

	//! Finalize the drawing of point sources.
	void postDrawPointSource(StelProjectorP projector);


	//! Determine if a point source is visible (should be drawn).
	//!
	//! This function is separate from drawPointSource for optimization.
	//!
	//! Also projects the point source to window coordinates.
	//!
	//! @param projector            Projector to project the point source.
	//! @param v                    the 3d position of the source in J2000 reference frame
	//! @param rcMag                the radius and luminance of the source as computed by computeRCMag()
	//! @param checkInScreen        Whether source in screen should be checked to avoid unnecessary drawing.
	//! @param outWindowCoordinates Window coordinates out the point source are written here.
	//!
	//! @return true if the point source is visible, false otherwise.
	bool pointSourceVisible(StelProjector* projector, const Vec3f& v, const float rcMag[2],
	                        bool checkInScreen, Vec3f& outWindowCoordinates)
	{
		// If radius is negative
		if (rcMag[0] <= 0.0f){return false;}
		return checkInScreen ? projector->projectCheck(v, outWindowCoordinates) 
		                     : projector->project(v, outWindowCoordinates);
	}

	//! Draw a point source halo.
	//!
	//! This is used in combination with pointSourceVisible (which 
	//! avoids unnecessary draws and projects a point source to window coordinates).
	//!
	//! Example:
	//!
	//! @code
	//! Vec3f win;
	//! if(skyDrawer->pointSourceVisible(&(*projector), pos3D, rcMag, checkInScreen, win))
	//! {
	//! 	skyDrawer->drawPointSource(win, rcMag, bV);
	//! }
	//! @endcode
	//!
	//! @param win   Coordinates of the point source in the window 
	//!              (computed by pointSourceVisible)
	//! @param rcMag the radius and luminance of the source as computed by computeRCMag()
	//! @param bV    the source B-V index
	void drawPointSource(const Vec3f& win, const float rcMag[2], unsigned int bV)
	{
		return drawPointSource(win, rcMag, colorTable[bV]);
	}

	void drawPointSource(const Vec3f& win, const float rcMag[2], const Vec3f& bcolor);

	//! Draw's the sun's corona during a solar eclipse on earth.
	void drawSunCorona(StelProjectorP projector, const Vec3d& v, float radius, float alpha);

	//! Terminate drawing of a 3D model, draw the halo
	//! @param projector Projector to use for this drawing operation
	//! @param v the 3d position of the source in J2000 reference frame
	//! @param illuminatedArea the illuminated area in arcmin^2
	//! @param mag the source integrated magnitude
	//! @param color the object halo RGB color
	void postDrawSky3dModel(StelProjectorP projector, const Vec3d& v, float illuminatedArea, float mag, const Vec3f& color = Vec3f(1.f,1.f,1.f));

	//! Compute RMag and CMag from magnitude.
	//! @param mag the object integrated V magnitude
	//! @param rcMag array of 2 floats containing the radius and luminance
	//! @return false if the object is too faint to be displayed
	bool computeRCMag(float mag, float rcMag[2]) const;

	//! Report that an object of luminance lum with an on-screen area of area pixels is currently displayed
	//! This information is used to determine the world adaptation luminance
	//! This method should be called during the update operations of the main loop
	//! @param lum luminance in cd/m^2
	//! @param fastAdaptation adapt the eye quickly if true, other wise use a smooth adaptation
	void reportLuminanceInFov(float lum, bool fastAdaptation=false);

	//! To be called before the drawing stage starts
	void preDraw();

	//! Compute the luminance for an extended source with the given surface brightness
	//! @param sb surface brightness in V magnitude/arcmin^2
	//! @return the luminance in cd/m^2
	static float surfacebrightnessToLuminance(float sb);
	//! Compute the surface brightness from the luminance of an extended source
	//! @param lum luminance in cd/m^2
	//! @return surface brightness in V magnitude/arcmin^2
	static float luminanceToSurfacebrightness(float lum);

	//! Convert quantized B-V index to float B-V
	static inline float indexToBV(unsigned char bV)
	{
		return (float)bV*(4.f/127.f)-0.5f;
	}

	//! Convert quantized B-V index to RGB colors
	static inline const Vec3f& indexToColor(unsigned char bV)
	{
		return colorTable[bV];
	}

public slots:
	//! Set the way brighter stars will look bigger as the fainter ones
	void setRelativeStarScale(double b=1.0) {starRelativeScale=b;}
	//! Get the way brighter stars will look bigger as the fainter ones
	float getRelativeStarScale() const {return starRelativeScale;}

	//! Set the absolute star brightness scale
	void setAbsoluteStarScale(double b=1.0) {starAbsoluteScaleF=b;}
	//! Get the absolute star brightness scale
	float getAbsoluteStarScale() const {return starAbsoluteScaleF;}

	//! Set source twinkle amount.
	void setTwinkleAmount(double b) {twinkleAmount=b;}
	//! Get source twinkle amount.
	float getTwinkleAmount() const {return twinkleAmount;}

	//! Set flag for source twinkling.
	void setFlagTwinkle(bool b) {flagStarTwinkle=b;}
	//! Get flag for source twinkling.
	bool getFlagTwinkle() const {return flagStarTwinkle;}

	//! Set flag for displaying point sources as points (faster on some hardware but not so nice).
	void setDrawStarsAsPoints(bool b) {drawStarsAsPoints=b;}
	//! Get flag for displaying point sources as points (faster on some hardware but not so nice).
	bool getDrawStarsAsPoints() const {return drawStarsAsPoints;}

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

	//! Get the luminance of the faintest visible object (e.g. RGB<0.05)
	//! It depends on the zoom level, on the eye adapation and on the point source rendering parameters
	//! @return the limit V luminance at which an object will be visible
	float getLimitLuminance() const {return limitLuminance;}

	//! Set the value of the eye adaptation flag
	void setFlagLuminanceAdaptation(bool b) {flagLuminanceAdaptation=b;}
	//! Get the current value of eye adaptation flag
	bool getFlagLuminanceAdaptation() const {return flagLuminanceAdaptation;}

	//! Informing the drawer whether atmosphere is displayed.
	//! This is used to avoid twinkling/simulate extinction/refraction.
	void setFlagHasAtmosphere(bool b) {flagHasAtmosphere=b;}
	//! This is used to decide whether to apply refraction/extinction before rendering point sources et al.
	bool getFlagHasAtmosphere() const {return flagHasAtmosphere;}

	//! Set extinction coefficient, mag/airmass (for extinction).
	void setExtinctionCoefficient(double extCoeff) {extinction.setExtinctionCoefficient(extCoeff);}
	//! Get extinction coefficient, mag/airmass (for extinction).
	double getExtinctionCoefficient() const {return extinction.getExtinctionCoefficient();}
	//! Set atmospheric (ground) temperature in deg celsius (for refraction).
	void setAtmosphereTemperature(double celsius) {refraction.setTemperature(celsius);}
	//! Get atmospheric (ground) temperature in deg celsius (for refraction).
	double getAtmosphereTemperature() const {return refraction.getTemperature();}
	//! Set atmospheric (ground) pressure in mbar (for refraction).
	void setAtmospherePressure(double mbar) {refraction.setPressure(mbar);}
	//! Get atmospheric (ground) pressure in mbar (for refraction).
	double getAtmospherePressure() const {return refraction.getPressure();}

	//! Get the current valid extinction computation class.
	const Extinction& getExtinction() const {return extinction;}
	//! Get the current valid fefraction computation class.
	const Refraction& getRefraction() const {return refraction;}

	//! Get the radius of the big halo texture used when a 3d model is very bright.
	float getBig3dModelHaloRadius() const {return big3dModelHaloRadius;}
	//! Set the radius of the big halo texture used when a 3d model is very bright.
	void setBig3dModelHaloRadius(float r) {big3dModelHaloRadius=r;}
	
private:
	// Debug
	float reverseComputeRCMag(float rmag) const;

	//! Compute the current limit magnitude by dichotomy
	float computeLimitMagnitude() const;

	//! Compute the current limit luminance by dichotomy
	float computeLimitLuminance() const;

	//! Get StelSkyDrawer maximum FOV.
	float getMaxAdaptFov(void) const {return maxAdaptFov;}
	//! Set StelSkyDrawer maximum FOV.
	//! Usually stars/planet halos are drawn fainter when FOV gets larger,
	//! but when FOV gets larger than this value, the stars do not become
	//! fainter any more. Must be >= 60.0.
	void setMaxAdaptFov(float fov) {maxAdaptFov = (fov < 60.f) ? 60.f : fov;}

	//! Get StelSkyDrawer minimum FOV.
	float getMinAdaptFov(void) const {return minAdaptFov;}
	//! Set StelSkyDrawer minimum FOV.
	//! Usually stars/planet halos are drawn brighter when FOV gets smaller.
	//! But when FOV gets smaller than this value, the stars do not become
	//! brighter any more. Must be <= 60.0.
	void setMinAdaptFov(float fov) {minAdaptFov = (fov > 60.f) ? 60.f : fov;}

	//! Set the scaling applied to input luminance before they are converted by the StelToneReproducer
	void setInputScale(float in) {inScale = in;}
	//! Get the scaling applied to input luminance before they are converted by the StelToneReproducer
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

	//! Used to draw the sky.
	class StelRenderer* renderer;
	StelToneReproducer* eye;

	Extinction extinction;
	Refraction refraction;

	float maxAdaptFov, minAdaptFov, lnfovFactor;
	bool drawStarsAsPoints;
	bool flagStarTwinkle;
	float twinkleAmount;

	//! Informing the drawer whether atmosphere is displayed.
	//! This is used to avoid twinkling/simulate extinction/refraction.
	bool flagHasAtmosphere;


	float starRelativeScale;
	float starAbsoluteScaleF;

	float starLinearScale;	// optimization variable

	//! Current magnitude limit for point sources
	float limitMagnitude;

	//! Current magnitude luminance
	float limitLuminance;

	//! Little halo texture
	class StelTextureNew* texHalo;

	//! Load B-V conversion parameters from config file
	void initColorTableFromConfigFile(class QSettings* conf);

	//! Contains the list of colors matching a given B-V index
	static Vec3f colorTable[128];

	//! The current Bortle Scale index
	int bortleScaleIndex;

	//! The scaling applied to input luminance before they are converted by the StelToneReproducer
	float inScale;

	//! 2D vertex with position and color.
	struct ColoredVertex
	{
		Vec2f position;
		Vec3f color;
		ColoredVertex(Vec2f position, Vec3f color):position(position), color(color){}

		VERTEX_ATTRIBUTES(Vec2f Position, Vec3f Color);
	};

	//! 2D vertex with position, color and texture coordinate.
	struct ColoredTexturedVertex
	{
		Vec2f position;
		Vec3f color;
		Vec2f texCoord;
		ColoredTexturedVertex(Vec2f position, Vec3f color, Vec2f texCoord)
			:position(position), color(color), texCoord(texCoord){}

		VERTEX_ATTRIBUTES(Vec2f Position, Vec3f Color, Vec2f TexCoord);
	};

	//! When stars are drawn as points, these are stored in this buffer.
	StelVertexBuffer<ColoredVertex>* starPointBuffer;

	//! Star sprite triangles.
	StelVertexBuffer<ColoredTexturedVertex>* starSpriteBuffer;

	//! Big halo triangles.
	StelVertexBuffer<ColoredTexturedVertex>* bigHaloBuffer;

	//! Sun halo triangles.
	StelVertexBuffer<ColoredTexturedVertex>* sunHaloBuffer;

	//! Sun corona triangles.
	StelVertexBuffer<ColoredTexturedVertex>* coronaBuffer;

	//! Are we drawing point sources at the moment?
	bool drawing;

	//! The maximum transformed luminance to apply at the next update
	float maxLum;
	//! The previously used world luminance
	float oldLum;

	//! Big halo texture
	class StelTextureNew* texBigHalo;
	class StelTextureNew* texSunHalo;
	class StelTextureNew* texCorona;

	bool flagLuminanceAdaptation;

	float big3dModelHaloRadius;


	//! Are the statistics IDs initialized?
	bool statisticsInitialized;

	//! ID used to modify the big halo draw statistic.
	int bigHaloStatID;
	//! ID used to modify the sun halo draw statistic.
	int sunHaloStatID;
	//! ID used to modify the star draw statistic.
	int starStatID;
};

#endif // _STELSKYDRAWER_HPP_
