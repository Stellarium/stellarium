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

#include "RefractionExtinction.hpp"
#include "StelTextureTypes.hpp"
#include "StelProjectorType.hpp"
#include "VecMath.hpp"
#include "StelOpenGL.hpp"

#include <QObject>

class StelToneReproducer;
class StelCore;
class StelPainter;

//! Contains the 2 parameters necessary to draw a star on screen.
//! the radius and luminance of the star halo texture.
struct RCMag
{
	float radius;
	float luminance;
};

//! @class StelSkyDrawer
//! Provide a set of methods used to draw sky objects taking into account
//! eyes adaptation, zoom level, instrument model and artificially set magnitude limits
class StelSkyDrawer : public QObject, protected QOpenGLFunctions
{
	Q_OBJECT

	//! Sets how much brighter stars will be bigger than fainter stars
	Q_PROPERTY(double relativeStarScale READ getRelativeStarScale WRITE setRelativeStarScale NOTIFY relativeStarScaleChanged)
	//! The absolute star brightness scale
	Q_PROPERTY(double absoluteStarScale READ getAbsoluteStarScale WRITE setAbsoluteStarScale NOTIFY absoluteStarScaleChanged)
	Q_PROPERTY(double twinkleAmount READ getTwinkleAmount WRITE setTwinkleAmount NOTIFY twinkleAmountChanged)
	Q_PROPERTY(bool flagStarTwinkle READ getFlagTwinkle WRITE setFlagTwinkle NOTIFY flagTwinkleChanged)
	Q_PROPERTY(int bortleScaleIndex READ getBortleScaleIndex WRITE setBortleScaleIndex NOTIFY bortleScaleIndexChanged)
	Q_PROPERTY(bool flagDrawBigStarHalo READ getFlagDrawBigStarHalo WRITE setFlagDrawBigStarHalo NOTIFY flagDrawBigStarHaloChanged)

	Q_PROPERTY(bool flagStarMagnitudeLimit READ getFlagStarMagnitudeLimit WRITE setFlagStarMagnitudeLimit NOTIFY flagStarMagnitudeLimitChanged)
	Q_PROPERTY(bool flagNebulaMagnitudeLimit READ getFlagNebulaMagnitudeLimit WRITE setFlagNebulaMagnitudeLimit NOTIFY flagNebulaMagnitudeLimitChanged)
	Q_PROPERTY(bool flagPlanetMagnitudeLimit READ getFlagPlanetMagnitudeLimit WRITE setFlagPlanetMagnitudeLimit NOTIFY flagPlanetMagnitudeLimitChanged)

	Q_PROPERTY(double customStarMagLimit READ getCustomStarMagnitudeLimit WRITE setCustomStarMagnitudeLimit NOTIFY customStarMagLimitChanged)
	Q_PROPERTY(double customNebulaMagLimit READ getCustomNebulaMagnitudeLimit WRITE setCustomNebulaMagnitudeLimit NOTIFY customNebulaMagLimitChanged)
	Q_PROPERTY(double customPlanetMagLimit READ getCustomPlanetMagnitudeLimit WRITE setCustomPlanetMagnitudeLimit NOTIFY customPlanetMagLimitChanged)

	Q_PROPERTY(bool flagLuminanceAdaptation READ getFlagLuminanceAdaptation WRITE setFlagLuminanceAdaptation NOTIFY flagLuminanceAdaptationChanged)

	Q_PROPERTY(double extinctionCoefficient READ getExtinctionCoefficient WRITE setExtinctionCoefficient NOTIFY extinctionCoefficientChanged)
	Q_PROPERTY(double atmosphereTemperature READ getAtmosphereTemperature WRITE setAtmosphereTemperature NOTIFY atmosphereTemperatureChanged)
	Q_PROPERTY(double atmospherePressure READ getAtmospherePressure WRITE setAtmospherePressure NOTIFY atmospherePressureChanged)

public:

	//! Constructor
	StelSkyDrawer(StelCore* core);
	//! Destructor
	~StelSkyDrawer();

	//! Init parameters from config file
	void init();

	//! Update with respect to the time and StelProjector/StelToneReproducer state
	//! @param deltaTime the time increment in second since last call.
	void update(double deltaTime);

	//! Set the proper openGL state before making calls to drawPointSource
	//! @param p a pointer to a valid instance of a Painter. The instance must be valid until postDrawPointSource() is called
	void preDrawPointSource(StelPainter* p);

	//! Finalize the drawing of point sources
	void postDrawPointSource(StelPainter* sPainter);

	//! Draw a point source halo.
	//! @param sPainter the StelPainter to use for drawing.
	//! @param v the 3d position of the source in J2000 reference frame
	//! @param rcMag the radius and luminance of the source as computed by computeRCMag()
	//! @param bV the source B-V index
	//! @param checkInScreen whether source in screen should be checked to avoid unnecessary drawing.
	//! @param twinkleFactor allows height-dependent twinkling. Recommended value: min(1,1-0.9*sin(altitude)). Allowed values [0..1]
	//! @return true if the source was actually visible and drawn
	bool drawPointSource(StelPainter* sPainter, const Vec3f& v, const RCMag &rcMag, unsigned int bV, bool checkInScreen=false, float twinkleFactor=1.0f)
	{
		return drawPointSource(sPainter, v, rcMag, colorTable[bV], checkInScreen, twinkleFactor);
	}

	bool drawPointSource(StelPainter* sPainter, const Vec3f& v, const RCMag &rcMag, const Vec3f& bcolor, bool checkInScreen=false, float twinkleFactor=1.0f);

	void drawSunCorona(StelPainter* painter, const Vec3f& v, float radius, const Vec3f& color, const float alpha);

	//! Terminate drawing of a 3D model, draw the halo
	//! @param p the StelPainter instance to use for this drawing operation
	//! @param v the 3d position of the source in J2000 reference frame
	//! @param illuminatedArea the illuminated area in arcmin^2
	//! @param mag the source integrated magnitude
	//! @param color the object halo RGB color
	void postDrawSky3dModel(StelPainter* p, const Vec3f& v, float illuminatedArea, float mag, const Vec3f& color = Vec3f(1.f,1.f,1.f));

	//! Compute RMag and CMag from magnitude.
	//! @param mag the object integrated V magnitude
	//! @param rcMag array of 2 floats containing the radius and luminance
	//! @return false if the object is too faint to be displayed
	bool computeRCMag(float mag, RCMag*) const;

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
	static float surfaceBrightnessToLuminance(float sb);
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
	void setRelativeStarScale(double b=1.0) {if(b!=starRelativeScale){ starRelativeScale=b; emit relativeStarScaleChanged(b);}}
	//! Get the way brighter stars will look bigger as the fainter ones
	double getRelativeStarScale() const {return starRelativeScale;}

	//! Set the absolute star brightness scale
	void setAbsoluteStarScale(double b=1.0) {if(b!=starAbsoluteScaleF){ starAbsoluteScaleF=b; emit absoluteStarScaleChanged(b);}}
	//! Get the absolute star brightness scale
	double getAbsoluteStarScale() const {return starAbsoluteScaleF;}

	//! Set source twinkle amount.
	void setTwinkleAmount(double b) {if(b!=twinkleAmount){ twinkleAmount=b; emit twinkleAmountChanged(b);}}
	//! Get source twinkle amount.
	double getTwinkleAmount() const {return twinkleAmount;}

	//! Set flag for source twinkling.
	void setFlagTwinkle(bool b) {if(b!=flagStarTwinkle){ flagStarTwinkle=b; emit flagTwinkleChanged(b);}}
	//! Get flag for source twinkling.
	bool getFlagTwinkle() const {return flagStarTwinkle;}

	//! Set flag for enable twinkling of stars without atmosphere.
	//! @note option for planetariums
	void setFlagForcedTwinkle(bool b) {if(b!=flagForcedTwinkle){ flagForcedTwinkle=b;}}
	//! Get flag for enable twinkling of stars without atmosphere.
	//! @note option for planetariums
	bool getFlagForcedTwinkle() const {return flagForcedTwinkle;}

	//! Set the parameters so that the stars disappear at about the limit given by the bortle scale
	//! The limit is valid only at a given zoom level (around 60 deg)
	//! @see https://en.wikipedia.org/wiki/Bortle_scale
	void setBortleScaleIndex(int index);
	//! Get the current Bortle scale index
	//! @see https://en.wikipedia.org/wiki/Bortle_scale
	int getBortleScaleIndex() const {return bortleScaleIndex;}
	//! Get the average NELM for current Bortle scale index
	float getNELMFromBortleScale() const;

	//! Set flag for drawing a halo around bright stars.
	void setFlagDrawBigStarHalo(bool b) {if(b!=flagDrawBigStarHalo){ flagDrawBigStarHalo=b; emit flagDrawBigStarHaloChanged(b);}}
	//! Get flag for drawing a halo around bright stars.
	bool getFlagDrawBigStarHalo() const {return flagDrawBigStarHalo;}

	//! Get the magnitude of the currently faintest visible point source
	//! It depends on the zoom level, on the eye adapation and on the point source rendering parameters
	//! @return the limit V mag at which a point source will be displayed
	float getLimitMagnitude() const {return limitMagnitude;}

	//! Toggle the application of user-defined star magnitude limit.
	//! If enabled, stars fainter than the magnitude set with
	//! setCustomStarMagnitudeLimit() will not be displayed.
	// FIXME: Exposed to scripts - make sure it synchs with the GUI. --BM
	void setFlagStarMagnitudeLimit(bool b) {if(b!=flagStarMagnitudeLimit){ flagStarMagnitudeLimit = b; emit flagStarMagnitudeLimitChanged(b);}}
	//! @return true if the user-defined star magnitude limit is in force.
	bool getFlagStarMagnitudeLimit() const {return flagStarMagnitudeLimit;}
	//! Toggle the application of user-defined deep-sky object magnitude limit.
	//! If enabled, deep-sky objects fainter than the magnitude set with
	//! setCustomNebulaMagnitudeLimit() will not be displayed.
	// FIXME: Exposed to scripts - make sure it synchs with the GUI. --BM
	void setFlagNebulaMagnitudeLimit(bool b) {if(b!=flagNebulaMagnitudeLimit){ flagNebulaMagnitudeLimit = b; emit flagNebulaMagnitudeLimitChanged(b);}}
	//! @return true if the user-defined nebula magnitude limit is in force.
	bool getFlagNebulaMagnitudeLimit() const {return flagNebulaMagnitudeLimit;}
	//! Toggle the application of user-defined solar system object magnitude limit.
	//! If enabled, planets, planetary moons, asteroids (KBO, ...) and comets fainter than the magnitude set with
	//! setCustomPlanetMagnitudeLimit() will not be displayed.
	// FIXME: Exposed to scripts - make sure it synchs with the GUI. --BM  --- GZ: this was copy/paste. Track down BM's changes!!!
	void setFlagPlanetMagnitudeLimit(bool b) {if(b!=flagPlanetMagnitudeLimit){ flagPlanetMagnitudeLimit = b; emit flagPlanetMagnitudeLimitChanged(b);}}
	//! @return true if the user-defined nebula magnitude limit is in force.
	bool getFlagPlanetMagnitudeLimit() const {return flagPlanetMagnitudeLimit;}

	//! Get the value used for forced star magnitude limiting.
	double getCustomStarMagnitudeLimit() const {return customStarMagLimit;}
	//! Sets a lower limit for star magnitudes (anything fainter is ignored).
	//! In force only if flagStarMagnitudeLimit is set.
	void setCustomStarMagnitudeLimit(double limit) {if(limit!=customStarMagLimit){ customStarMagLimit=limit; emit customStarMagLimitChanged(limit);}}
	//! Get the value used for forced nebula magnitude limiting.
	double getCustomNebulaMagnitudeLimit() const {return customNebulaMagLimit;}
	//! Sets a lower limit for nebula magnitudes (anything fainter is ignored).
	//! In force only if flagNebulaMagnitudeLimit is set.
	void setCustomNebulaMagnitudeLimit(double limit) {if(limit!=customNebulaMagLimit){ customNebulaMagLimit=limit; emit customNebulaMagLimitChanged(limit);}}
	//! Get the value used for forced solar system object magnitude limiting.
	double getCustomPlanetMagnitudeLimit() const {return customPlanetMagLimit;}
	//! Sets a lower limit for solar system object magnitudes (anything fainter is ignored).
	//! In force only if flagPlanetMagnitudeLimit is set.
	void setCustomPlanetMagnitudeLimit(double limit) {if(limit!=customPlanetMagLimit){ customPlanetMagLimit=limit; emit customPlanetMagLimitChanged(limit);}}

	//! Get the luminance of the faintest visible object (e.g. RGB<0.05)
	//! It depends on the zoom level, on the eye adapation and on the point source rendering parameters
	//! @return the limit V luminance at which an object will be visible
	float getLimitLuminance() const {return limitLuminance;}

	//! Set the value of the eye adaptation flag
	void setFlagLuminanceAdaptation(bool b) {if(b!=flagLuminanceAdaptation){ flagLuminanceAdaptation=b; emit flagLuminanceAdaptationChanged(b);}}
	//! Get the current value of eye adaptation flag
	bool getFlagLuminanceAdaptation() const {return flagLuminanceAdaptation;}

	//! Informing the drawer whether atmosphere is displayed.
	//! This is used to avoid twinkling/simulate extinction/refraction.
	void setFlagHasAtmosphere(bool b) {flagHasAtmosphere=b;}
	//! This is used to decide whether to apply refraction/extinction before rendering point sources et al.
	bool getFlagHasAtmosphere() const {return flagHasAtmosphere;}

	//! Set extinction coefficient, mag/airmass (for extinction).
	void setExtinctionCoefficient(double extCoeff) {if(extCoeff!=extinction.getExtinctionCoefficient()){ extinction.setExtinctionCoefficient(extCoeff); emit extinctionCoefficientChanged(extinction.getExtinctionCoefficient());}}
	//! Get extinction coefficient, mag/airmass (for extinction).
	double getExtinctionCoefficient() const {return extinction.getExtinctionCoefficient();}
	//! Set atmospheric (ground) temperature in deg celsius (for refraction).
	void setAtmosphereTemperature(double celsius) {if(celsius!=refraction.getTemperature()){refraction.setTemperature(celsius); emit atmosphereTemperatureChanged(refraction.getTemperature());}}
	//! Get atmospheric (ground) temperature in deg celsius (for refraction).
	double getAtmosphereTemperature() const {return refraction.getTemperature();}
	//! Set atmospheric (ground) pressure in mbar (for refraction).
	void setAtmospherePressure(double mbar) {if(mbar!=refraction.getPressure()){ refraction.setPressure(mbar); emit atmospherePressureChanged(refraction.getPressure());}}
	//! Get atmospheric (ground) pressure in mbar (for refraction).
	double getAtmospherePressure() const {return refraction.getPressure();}

	//! Get the current valid extinction computation object.
	const Extinction& getExtinction() const {return extinction;}
	//! Get the current valid refraction computation object.
	const Refraction& getRefraction() const {return refraction;}

	//! Get the radius of the big halo texture used when a 3d model is very bright.
	float getBig3dModelHaloRadius() const {return big3dModelHaloRadius;}
	//! Set the radius of the big halo texture used when a 3d model is very bright.
	void setBig3dModelHaloRadius(float r) {big3dModelHaloRadius=r;}
signals:
	//! Emitted whenever the relative star scale changed
	void relativeStarScaleChanged(double b);
	//! Emitted whenever the absolute star scale changed
	void absoluteStarScaleChanged(double b);
	//! Emitted whenever the twinkle amount changed
	void twinkleAmountChanged(double b);
	//! Emitted whenever the twinkle flag is toggled
	void flagTwinkleChanged(bool b);
	//! Emitted whenever the Bortle scale index changed
	void bortleScaleIndexChanged(int index);
	//! Emitted when flag to draw big halo around stars changed
	void flagDrawBigStarHaloChanged(bool b);

	//! Emitted whenever the star magnitude limit flag is toggled
	void flagStarMagnitudeLimitChanged(bool b);
	//! Emitted whenever the nebula magnitude limit flag is toggled
	void flagNebulaMagnitudeLimitChanged(bool b);
	//! Emitted whenever the planet magnitude limit flag is toggled
	void flagPlanetMagnitudeLimitChanged(bool b);

	//! Emitted whenever the star magnitude limit changed
	void customStarMagLimitChanged(double limit);
	//! Emitted whenever the nebula magnitude limit changed
	void customNebulaMagLimitChanged(double limit);
	//! Emitted whenever the planet magnitude limit changed
	void customPlanetMagLimitChanged(double limit);

	//! Emitted whenever the luminance adaptation flag is toggled
	void flagLuminanceAdaptationChanged(bool b);

	void extinctionCoefficientChanged(double coeff);
	void atmosphereTemperatureChanged(double celsius);
	void atmospherePressureChanged(double mbar);
	
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
	StelToneReproducer* eye;

	Extinction extinction;
	Refraction refraction;

	float maxAdaptFov, minAdaptFov, lnfovFactor;
	bool flagStarTwinkle;
	bool flagForcedTwinkle;
	double twinkleAmount;
	bool flagDrawBigStarHalo;

	//! Informing the drawer whether atmosphere is displayed.
	//! This is used to avoid twinkling/simulate extinction/refraction.
	bool flagHasAtmosphere;

	//! Controls the application of the user-defined star magnitude limit.
	//! @see customStarMagnitudeLimit
	bool flagStarMagnitudeLimit;
	//! Controls the application of the user-defined nebula magnitude limit.
	//! @see customNebulaMagnitudeLimit
	bool flagNebulaMagnitudeLimit;
	//! Controls the application of the user-defined planet magnitude limit.
	//! @see customPlanetMagnitudeLimit
	bool flagPlanetMagnitudeLimit;

	double starRelativeScale;
	double starAbsoluteScaleF;

	float starLinearScale;	// optimization variable

	//! Current magnitude limit for point sources
	float limitMagnitude;

	//! Current magnitude luminance
	float limitLuminance;

	//! User-defined magnitude limit for stars.
	//! Interpreted as a lower limit - stars fainter than this value will not
	//! be displayed.
	//! Used if flagStarMagnitudeLimit is true.
	double customStarMagLimit;
	//! User-defined magnitude limit for deep-sky objects.
	//! Interpreted as a lower limit - nebulae fainter than this value will not
	//! be displayed.
	//! Used if flagNebulaMagnitudeLimit is true.
	//! @todo Why the asterisks this is not in NebulaMgr? --BM
	//  GZ To explain: we have 3 limits for stars, nebulae, planets. It's easier to maintain the pretty similar code in 1 place.
	double customNebulaMagLimit;
	//! User-defined magnitude limit for solar system objects.
	//! Interpreted as a lower limit - planets fainter than this value will not
	//! be displayed.
	//! Used if flagPlanetMagnitudeLimit is true.
	double customPlanetMagLimit;

	//! Little halo texture
	StelTextureSP texHalo;

	//! Load B-V conversion parameters from config file
	void initColorTableFromConfigFile(class QSettings* conf);

	//! Contains the list of colors matching a given B-V index
	static Vec3f colorTable[128];

	//! The current Bortle Scale index
	int bortleScaleIndex;

	//! The scaling applied to input luminance before they are converted by the StelToneReproducer
	float inScale;

	// Variables used for GL optimization when displaying point sources
	//! Vertex format for a point source.
	//! Texture pos is stored in another separately.
	struct StarVertex {
		Vec2f pos;
		unsigned char color[4];
	};
	static_assert(sizeof(StarVertex) == 12, "Size of StarVertex must be 12 bytes");
	
	//! Buffer for storing the vertex array data
	StarVertex* vertexArray;

	//! Buffer for storing the texture coordinate array data.
	unsigned char* textureCoordArray;
	
	class QOpenGLShaderProgram* starShaderProgram;
	struct StarShaderVars {
		int projectionMatrix;
		int texCoord;
		int pos;
		int color;
		int texture;
	};
	StarShaderVars starShaderVars;
	
	//! Current number of sources stored in the buffers (still to display)
	unsigned int nbPointSources;
	//! Maximum number of sources which can be stored in the buffers
	unsigned int maxPointSources;

	//! The maximum transformed luminance to apply at the next update
	float maxLum;
	//! The previously used world luminance
	float oldLum;

	//! Big halo texture
	StelTextureSP texBigHalo;
	StelTextureSP texSunHalo;
	StelTextureSP texSunCorona;

	bool flagLuminanceAdaptation;

	float big3dModelHaloRadius;
};

#endif // _STELSKYDRAWER_HPP_
