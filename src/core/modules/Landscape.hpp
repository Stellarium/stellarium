/*
 * Stellarium
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

#ifndef LANDSCAPE_HPP
#define LANDSCAPE_HPP

#include "VecMath.hpp"
#include "StelToneReproducer.hpp"
#include "StelProjector.hpp"

#include "StelFader.hpp"
#include "StelUtils.hpp"
#include "StelTextureTypes.hpp"
#include "StelLocation.hpp"
#include "StelSphereGeometry.hpp"

#include <QMap>
#include <QImage>
#include <QList>
#include <QFont>

class QSettings;
class StelLocation;
class StelCore;
class StelPainter;

//! @class Landscape
//! Store and manages the displaying of the Landscape.
//! Don't use this class directly, use the LandscapeMgr.
//! A landscape's most important element is a photo panorama, or at least a polygon that describes the true horizon visible from your observing location.
//! Optional components include:
//!  - A fog texture that is displayed with the Fog [F] command (displayed only if atmosphere is on!).
//!  - A location. It is possible to auto-move to the location when loading.
//!  - Atmospheric conditions: temperature/pressure/extinction coefficients. These influence refraction and extinction.
//!  - Light pollution information (Bortle index)
//!  - A night texture that gets blended over the dimmed daylight panorama. (All landscapes except Polygonal.)
//!  - A polygonal horizon line (required for PolygonalLandscape). If present, defines a measured horizon line, which can be plotted or queried for rise/set predictions.
//!  - You can set a minimum brightness level to prevent too dark landscape. There is
//!    a global activation setting (config.ini[landscape]flag_minimal_brightness),
//!    a global value (config.ini[landscape]minimal_brightness),
//!    and, if config.ini[landscape]flag_landscape_sets_minimal_brightness=true,
//!    optional individual values given in landscape.ini[landscape]minimal_brightness are used.
//!
//! We discern:
//!   @param LandscapeId: The directory name of the landscape.
//!   @param name: The landscape name as specified in the LandscapeIni (may contain spaces, translatable, UTF8, ...)
class Landscape
{
public:
	typedef struct
	{
		QString name;
		Vec3d featurePoint; // start of the line: mountain peak, building, ...
		Vec3d labelPoint;   // end of the line, where the centered label best fits.
	} LandscapeLabel;

	Landscape(float _radius = 2.f);
	virtual ~Landscape();
	//! Load landscape.
	//! @param landscapeIni A reference to an existing QSettings object which describes the landscape
	//! @param landscapeId The name of the directory for the landscape files (e.g. "ocean")
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId) = 0;

	//! Return approximate memory footprint in bytes (required for cache cost estimate in LandscapeMgr)
	//! The returned value is only approximate, content of QStrings and other small containers like the horizon polygon are not put in in detail.
	//! However, texture image sizes must be computed and added in subclasses.
	//! The value returned is a sum of RAM and texture memory requirements.
	unsigned int getMemorySize() const {return memorySize;}

	//! Draw the landscape. If onlyPolygon, only draw the landscape polygon, if one is defined. If no polygon is defined, the
	virtual void draw(StelCore* core, bool onlyPolygon) = 0;
	void update(double deltaTime)
	{
		landFader.update(static_cast<int>(deltaTime*1000));
		fogFader.update(static_cast<int>(deltaTime*1000));
		illumFader.update(static_cast<int>(deltaTime*1000));
		labelFader.update(static_cast<int>(deltaTime*1000));
	}

	//! Set the brightness of the landscape plus brightness of optional add-on night lightscape.
	//! This is called in each draw().
	void setBrightness(const double b, const double pollutionBrightness=0.0) {landscapeBrightness = static_cast<float>(b); lightScapeBrightness=static_cast<float>(pollutionBrightness); }

	//! Returns the current brightness level
	double getBrightness() const { return static_cast<double>(landscapeBrightness); }
	//! Returns the lightscape brightness
	double getLightscapeBrightness() const { return static_cast<double>(lightScapeBrightness); }
	//! Returns the lightscape brighness modulated with the fader's target state (i.e. binary on/off)
	double getTargetLightscapeBrightness() const { return static_cast<double>(lightScapeBrightness * illumFader); }
	//! Gets the currently effective lightscape brightness (modulated by the fader)
	double getEffectiveLightscapeBrightness() const { return static_cast<double>(lightScapeBrightness * illumFader.getInterstate()); }

	//! Set whether landscape is displayed (does not concern fog)
	void setFlagShow(const bool b) {landFader=b;}
	//! Get whether landscape is displayed (does not concern fog)
	bool getFlagShow() const {return static_cast<bool>(landFader);}
	//! Returns the currently effective land fade value
	float getEffectiveLandFadeValue() const { return landFader.getInterstate(); }
	//! Set whether fog is displayed
	void setFlagShowFog(const bool b) {fogFader=b;}
	//! Get whether fog is displayed
	bool getFlagShowFog() const {return static_cast<bool>(fogFader);}
	//! Set whether illumination is displayed
	void setFlagShowIllumination(const bool b) {illumFader=b;}
	//! Get whether illumination is displayed
	bool getFlagShowIllumination() const {return static_cast<bool>(illumFader);}
	//! Set whether labels are displayed
	void setFlagShowLabels(const bool b) {labelFader=b;}
	//! Get whether labels are displayed
	bool getFlagShowLabels() const {return static_cast<bool>(labelFader);}
	//! change font and fontsize for landscape labels
	void setLabelFontSize(const int size){fontSize=size;}

	//! Get landscape name
	QString getName() const {return name;}
	//! Get landscape author name
	QString getAuthorName() const {return author;}
	//! Get landscape description
	QString getDescription() const {return description;}
	//! Get landscape id. This is the landscape directory name, used for cache handling.
	QString getId() const {return id;}

	//! Return the associated location (may be empty!)
	const StelLocation& getLocation() const {return location;}
	//! Return if the location is valid (a valid location has a valid planetName!)
	bool hasLocation() const {return (!(location.planetName.isEmpty()));}
  	//! Return default Bortle index (light pollution value) or -1 (unknown/no change)
	int getDefaultBortleIndex() const {return defaultBortleIndex;}
	//! Return default fog setting (0/1) or -1 (no change)
	int getDefaultFogSetting() const {return defaultFogSetting;}
	//! Return default atmosperic extinction [mag/airmass], or -1 (no change)
	double getDefaultAtmosphericExtinction() const {return defaultExtinctionCoefficient;}
	//! Return configured atmospheric temperature [degrees Celsius], for refraction computation, or -1000 for "unknown/no change".
	double getDefaultAtmosphericTemperature() const {return defaultTemperature;}
	//! Return configured atmospheric pressure [mbar], for refraction computation.
	//! returns -1 to signal "standard conditions" [compute from altitude], or -2 for "unknown/invalid/no change"
	double getDefaultAtmosphericPressure() const {return defaultPressure;}
	//! Return minimal brightness for landscape
	//! returns -1 to signal "standard conditions" (use default value from config.ini)
	double getLandscapeMinimalBrightness() const {return static_cast<double>(minBrightness);}

	//! Set an additional z-axis (azimuth) rotation after landscape has been loaded.
	//! This is intended for special uses such as when the landscape consists of
	//! a vehicle which might change orientation over time (e.g. a ship). It is called
	//! e.g. by the LandscapeMgr. Contrary to that, the purpose of the azimuth rotation
	//! (landscape/[decor_]angle_rotatez) in landscape.ini is to orient the pano.
	//! @param d the rotation angle in degrees.
	void setZRotation(float d) {angleRotateZOffset = d * static_cast<float>(M_PI)/180.0f;}

	//! Get whether the landscape is currently fully visible (i.e. opaque).
	bool getIsFullyVisible() const {return landFader.getInterstate() >= 0.999f;}
	//! Get the sine of the limiting altitude (can be used to short-cut drawing below horizon, like star fields). There is no set here, value is only from landscape.ini
	double getSinMinAltitudeLimit() const {return sinMinAltitudeLimit;}

	//! Find opacity in a certain direction. (New in V0.13 series)
	//! can be used to find sunrise or visibility questions on the real-world landscape horizon.
	//! Default implementation indicates the horizon equals math horizon.
	// TBD: Maybe change this to azalt[2]<sinMinAltitudeLimit ? (But never called in practice, reimplemented by the subclasses...)
	virtual float getOpacity(Vec3d azalt) const { Q_ASSERT(0); return (azalt[2]<0 ? 1.0f : 0.0f); }
	//! The list of azimuths (counted from True North towards East) and altitudes can come in various formats. We read the first two elements, which can be of formats:
	enum horizonListMode {
		invalid        =-1,
		azDeg_altDeg   = 0, //! azimuth[degrees] altitude[degrees]
		azDeg_zdDeg    = 1, //! azimuth[degrees] zenithDistance[degrees]
		azRad_altRad   = 2, //! azimuth[radians] altitude[radians]
		azRad_zdRad    = 3, //! azimuth[radians] zenithDistance[radians]
		azGrad_altGrad = 4, //! azimuth[new_degrees] altitude[new_degrees] (may be found on theodolites)
		azGrad_zdGrad  = 5  //! azimuth[new_degrees] zenithDistance[new_degrees] (may be found on theodolites)
	};
	
	//! Load descriptive labels from optional file gazetteer.LANG.utf8.
	void loadLabels(const QString& landscapeId);
	bool hasLandscapePolygon() const {return !horizonPolygon.isNull();}

protected:
	//! Load attributes common to all landscapes
	//! @param landscapeIni A reference to an existing QSettings object which describes the landscape
	//! @param landscapeId The name of the directory for the landscape files (e.g. "ocean")
	void loadCommon(const QSettings& landscapeIni, const QString& landscapeId);

	//! Draw optional labels on the landscape
	void drawLabels(StelCore *core, StelPainter *painter);


	//! Create a StelSphericalPolygon that describes a measured horizon line. If present, this can be used to draw a horizon line
	//! or simplify the functionality to discern if an object is below the horizon.
	//! @param lineFileName A text file with lines that are either empty or comment lines starting with # or azimuth altitude [degrees]
	//! @param polyAngleRotateZ possibility to set some final calibration offset like meridian convergence correction.
	//! @param listMode keys which indicate angular units for the angles
	void createPolygonalHorizon(const QString& lineFileName, const float polyAngleRotateZ=0.0f, const QString &listMode="azDeg_altDeg");

	//! search for a texture in landscape directory, else global textures directory
	//! @param basename The name of a texture file, e.g. "fog.png"
	//! @param landscapeId The landscape ID (directory name) to which the texture belongs
	//! @note returns an empty string if file not found.
	static const QString getTexturePath(const QString& basename, const QString& landscapeId);
	double radius;
	QString name;          //! Read from landscape.ini:[landscape]name
	QString author;        //! Read from landscape.ini:[landscape]author
	QString description;   //! Read from landscape.ini:[landscape]description
	QString id;            //! Set during load. Required for consistent caching.

	float minBrightness;   //! Read from landscape.ini:[landscape]minimal_brightness. Allows minimum visibility that cannot be underpowered.
	float landscapeBrightness;  //! brightness [0..1] to draw the landscape. Computed by the LandscapeMgr.
	float lightScapeBrightness; //! can be used to draw nightscape texture (e.g. city light pollution), if available. Computed by the LandscapeMgr.
	bool validLandscape;   //! was a landscape loaded properly?
	LinearFader landFader; //! Used to slowly fade in/out landscape painting.
	LinearFader fogFader;  //! Used to slowly fade in/out fog painting.
	LinearFader illumFader;//! Used to slowly fade in/out illumination painting.
	LinearFader labelFader;//! Used to slowly fade in/out landscape feature labels.
	unsigned int rows;     //! horizontal rows.  May be given in landscape.ini:[landscape]tesselate_rows. More indicates higher accuracy, but is slower.
	unsigned int cols;     //! vertical columns. May be given in landscape.ini:[landscape]tesselate_cols. More indicates higher accuracy, but is slower.
	float angleRotateZ;    //! [radians] if pano does not have its left border in the east, rotate in azimuth. Configured in landscape.ini[landscape]angle_rotatez (or decor_angle_rotatez for old_style landscapes)
	float angleRotateZOffset; //! [radians] This is a rotation changeable at runtime via setZRotation (called by LandscapeMgr::setZRotation).
				  //! Not in landscape.ini: Used in special cases where the horizon may rotate, e.g. on a ship.

	double sinMinAltitudeLimit; //! Minimal altitude of landscape cover. Can be used to construct bounding caps, so that e.g. no stars are drawn below this altitude. Default -0.035, i.e. sin(-2 degrees).

	StelLocation location; //! OPTIONAL. If present, can be used to set location.
	int defaultBortleIndex; //! May be given in landscape.ini:[location]light_pollution. Default: -1 (no change).
	int defaultFogSetting;  //! May be given in landscape.ini:[location]display_fog: -1(no change), 0(off), 1(on). Default: -1.
	double defaultExtinctionCoefficient; //! May be given in landscape.ini:[location]atmospheric_extinction_coefficient. Default -1 (no change).
	double defaultTemperature; //! [Celsius] May be given in landscape.ini:[location]atmospheric_temperature. default: -1000.0 (no change)
	double defaultPressure;    //! [mbar]    May be given in landscape.ini:[location]atmospheric_pressure. Default -1.0 (compute from [location]/altitude), use -2 to indicate "no change".

	// Optional elements which, if present, describe a horizon polygon. They can be used to render a line or a filled region, esp. in LandscapePolygonal
	SphericalRegionP horizonPolygon;   //! Optional element describing the horizon line.
					   //! Data shall be read from the file given as landscape.ini[landscape]polygonal_horizon_list
					   //! For LandscapePolygonal, this is the only horizon data item.
	Vec3f horizonPolygonLineColor;     //! for all horizon types, the horizonPolygon line, if specified, will be drawn in this color
					   //! specified in landscape.ini[landscape]horizon_line_color. Negative red (default) indicated "don't draw".
	// Optional element: labels for landscape features.
	QList<LandscapeLabel> landscapeLabels;
	int fontSize;     //! Used for landscape labels (optionally indicating landscape features)
	Vec3f labelColor; //! Color for the landscape labels.
	unsigned int memorySize;   //!< holds an approximate value of memory consumption (for cache cost estimate)
};

//! @class LandscapeOldStyle
//! This was the original landscape, introduced for decorative purposes. It segments the horizon in several tiles
//! (usually 4 or 8), therefore allowing very high resolution horizons also on limited hardware,
//! and closes the ground with a separate bottom piece. (You may want to configure a map with pointers to surrounding mountains or a compass rose instead!)
//! You can use panoramas created in equirectangular or cylindrical coordinates, for the latter case set
//! [landscape]tan_mode=true.
//! Until V0.10.5 there was an undetected bug involving vertical positioning. For historical reasons (many landscapes
//! were already configured and published), it was decided to keep this bug as feature, but a fix for new landscapes is
//! available: [landscape]calibrated=true.
//! As of 0.10.6, the fix is only valid for equirectangular panoramas.
//! As of V0.13, [landscape]calibrated=true and [landscape]tan_mode=true go together for cylindrical panoramas.
//! It is more involved to configure, but may still be preferred if you require the resolution, e.g. for alignment studies
//! for archaeoastronomy. In this case, don't forget to set calibrated=true in landscape.ini.
//! Since V0.13.1, also this landscape has a self-luminous (light pollution) option: Configure light<n> entries with textures overlaid the tex<n> textures. Only textures with light are necessary!
//! Can be easily made using layers with e.g. Photoshop or Gimp.
class LandscapeOldStyle : public Landscape
{
public:
	LandscapeOldStyle(float radius = 2.0f);
	virtual ~LandscapeOldStyle() Q_DECL_OVERRIDE;
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId) Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core, bool onlyPolygon) Q_DECL_OVERRIDE;
	//void create(bool _fullpath, QMap<QString, QString> param); // still not implemented
	virtual float getOpacity(Vec3d azalt) const Q_DECL_OVERRIDE;
protected:
	typedef struct
	{
		StelTextureSP tex;
		StelTextureSP tex_illum; // optional light texture.
		float texCoords[4];
	} landscapeTexCoord;

private:
	void drawFog(StelCore* core, StelPainter&) const;
	// drawLight==true for illumination layer, it then selects only the self-illuminating panels.
	void drawDecor(StelCore* core, StelPainter&, const bool drawLight=false) const;
	void drawGround(StelCore* core, StelPainter&) const;
	QVector<Vec3d> groundVertexArr;
	QVector<Vec2f> groundTexCoordArr;
	StelTextureSP* sideTexs;
	unsigned short int nbSideTexs;
	unsigned short int nbSide;
	landscapeTexCoord* sides;
	StelTextureSP fogTex;
	StelTextureSP groundTex;
	QVector<QImage*> sidesImages; // Required for opacity lookup
	unsigned short int nbDecorRepeat;
	float fogAltAngle;
	float fogAngleShift;
	float decorAltAngle; // vertical extent of the side panels
	float decorAngleShift;
	float groundAngleShift; //! [radians]: altitude of the bottom plane. Usually negative and equal to decorAngleShift
	double groundAngleRotateZ; //! [radians]: rotation to bring top of texture away from due east.
	bool drawGroundFirst;
	bool tanMode;		// Whether the angles should be converted using tan instead of sin, i.e., for a cylindrical pano
	bool calibrated;	// if true, the documented altitudes are indeed correct (the original code is buggy!)
	struct LOSSide
	{
		StelVertexArray arr;
		StelTextureSP tex;
		bool light; // true if texture is self-lighting.
	};

	QList<LOSSide> precomputedSides;
};

/////////////////////////////////////////////////////////
///
//! @class LandscapePolygonal
//! This uses the list of (usually measured) horizon altitudes to define the horizon.
//! Define it with the following names in landscape.ini:
//! @param landscape/ground_color use this color below horizon
//! @param landscape/polygonal_horizon_list filename containing azimuths/altitudes, compatible with Carte du Ciel.
//! @param landscape/polygonal_angle_rotatez offset for the polygonal measurement
//!        (different from landscape/angle_rotatez in photo panos, often photo and line are not aligned.)
class LandscapePolygonal : public Landscape
{
public:
	LandscapePolygonal(float radius = 1.f);
	virtual ~LandscapePolygonal() Q_DECL_OVERRIDE;
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId) Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core, bool onlyPolygon) Q_DECL_OVERRIDE;
	virtual float getOpacity(Vec3d azalt) const Q_DECL_OVERRIDE;
private:
	// we have inherited: horizonFileName, horizonPolygon, horizonPolygonLineColor
	Vec3f groundColor; //! specified in landscape.ini[landscape]ground_color.
};

///////////////////////////////////////////////////////////////
///
//! @class LandscapeFisheye
//! This uses a single image in fisheye projection. The image is typically square, ...
//! @param texFov:  field of view (opening angle) of the square texture, radians.
//! If @param angleRotateZ==0, the top image border is due south.
class LandscapeFisheye : public Landscape
{
public:
	LandscapeFisheye(float radius = 1.f);
	virtual ~LandscapeFisheye() Q_DECL_OVERRIDE;
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId) Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core, bool onlyPolygon) Q_DECL_OVERRIDE;
	//! Sample landscape texture for transparency/opacity. May be used for visibility, sunrise etc.
	//! @param azalt normalized direction in alt-az frame
	virtual float getOpacity(Vec3d azalt) const Q_DECL_OVERRIDE;
	//! create a fisheye landscape from basic parameters (no ini file needed).
	//! @param name Landscape name
	//! @param maptex the fisheye texture
	//! @param maptexIllum the fisheye texture that is overlaid in the night (streetlights, skyglow, ...)
	//! @param texturefov field of view for the photo, degrees
	//! @param angleRotateZ azimuth rotation angle, degrees
	void create(const QString name, const QString& maptex, float texturefov, float angleRotateZ);
	void create(const QString name, float texturefov, const QString& maptex, const QString &_maptexFog="", const QString& _maptexIllum="", const float angleRotateZ=0.0f);

private:
	StelTextureSP mapTex;      //!< The fisheye image, centered on the zenith.
	StelTextureSP mapTexFog;   //!< Optional panorama of identical size (create as layer over the mapTex image in your favorite image processor).
				   //!< can also be smaller, just the texture is again mapped onto the same geometry.
	StelTextureSP mapTexIllum; //!< Optional fisheye image of identical size (create as layer in your favorite image processor) or at least, proportions.
				   //!< To simulate light pollution (skyglow), street lights, light in windows, ... at night
	QImage *mapImage;          //!< The same image as mapTex, but stored in-mem for sampling.

	float texFov;
};

//////////////////////////////////////////////////////////////////////////
//! @class LandscapeSpherical
//! This uses a single panorama image in spherical (equirectangular) projection. A complete image is rectangular with the horizon forming a
//! horizontal line centered vertically, and vertical altitude angles linearly mapped in image height.
//! Since 0.13 and Qt5, large images of 8192x4096 pixels are available, but they still may not work on every hardware.
//! If @param angleRotateZ==0, the left/right image border is due east.
//! It is possible to remove empty top or bottom parts of the textures (main texture: only top part should meaningfully be cut away!)
//! The textures should still be power-of-two, so maybe 8192x1024 for the fog, or 8192x2048 for the light pollution.
//! (It's OK to stretch the textures. They just have to fit, geometrically!)
//! TODO: Allow a horizontal split for 2 or even 4 parts, i.e. super-large, super-accurate panos.
class LandscapeSpherical : public Landscape
{
public:
	LandscapeSpherical(float radius = 1.f);
	virtual ~LandscapeSpherical() Q_DECL_OVERRIDE;
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId) Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core, bool onlyPolygon) Q_DECL_OVERRIDE;
	//! Sample landscape texture for transparency/opacity. May be used for visibility, sunrise etc.
	//! @param azalt normalized direction in alt-az frame
	//! @retval alpha (0=fully transparent, 1=fully opaque. Trees, leaves, glass etc may have intermediate values.)
	virtual float getOpacity(Vec3d azalt) const Q_DECL_OVERRIDE;
	//! create a spherical landscape from basic parameters (no ini file needed).
	//! @param name Landscape name
	//! @param maptex the equirectangular texture
	//! @param maptexIllum the equirectangular texture that is overlaid in the night (streetlights, skyglow, ...)
	//! @param angleRotateZ azimuth rotation angle, degrees [0]
	//! @param _mapTexTop altitude angle of top edge of texture, degrees [90]
	//! @param _mapTexBottom altitude angle of bottom edge of texture, degrees [-90]
	//! @param _fogTexTop altitude angle of top edge of fog texture, degrees [90]
	//! @param _fogTexBottom altitude angle of bottom edge of fog texture, degrees [-90]
	//! @param _illumTexTop altitude angle of top edge of light pollution texture, degrees [90]
	//! @param _illumTexBottom altitude angle of bottom edge of light pollution texture, degrees [-90]
	//! @param _bottomCapColor RGB triplet for closing the hole around the nadir, if any. A color value of (-1/0/0) signals "no cap"
	void create(const QString name, const QString& maptex, const QString &_maptexFog="", const QString& _maptexIllum="", const float _angleRotateZ=0.0f,
				const float _mapTexTop=90.0f, const float _mapTexBottom=-90.0f,
				const float _fogTexTop=90.0f, const float _fogTexBottom=-90.0f,
				const float _illumTexTop=90.0f, const float _illumTexBottom=-90.0f, const Vec3f _bottomCapColor=Vec3f(-1.0f, 0.0f, 0.0f));

private:
	StelTextureSP mapTex;      //!< The equirectangular panorama texture
	StelTextureSP mapTexFog;   //!< Optional panorama of identical size (create as layer over the mapTex image in your favorite image processor).
				   //!< can also be smaller, just the texture is again mapped onto the same geometry.
	StelTextureSP mapTexIllum; //!< Optional panorama of identical size (create as layer over the mapTex image in your favorite image processor).
				   //!< To simulate light pollution (skyglow), street lights, light in windows, ... at night
	SphericalCap bottomCap;	   //!< Geometry to close the bottom with a monochrome color when mapTexBottom is given. (Avoid hole in Nadir!)
	// These vars are here to conserve texture memory. They must be allowed to be different: a landscape may have its highest elevations at 15°, fog may reach from -25 to +15°,
	// light pollution may cover -5° (street lamps slightly below) plus parts of or even the whole sky. All have default values to simplify life.
	float mapTexTop;           //!< zenithal top angle of the landscape texture, radians
	float mapTexBottom;	   //!< zenithal bottom angle of the landscape texture, radians
	float fogTexTop;	   //!< zenithal top angle of the fog texture, radians
	float fogTexBottom;	   //!< zenithal bottom angle of the fog texture, radians
	float illumTexTop;	   //!< zenithal top angle of the illumination texture, radians
	float illumTexBottom;	   //!< zenithal bottom angle of the illumination texture, radians
	QImage *mapImage;          //!< The same image as mapTex, but stored in-mem for opacity sampling.
	Vec3f bottomCapColor;      //!< The bottomCap, if specified, will be drawn in this color
};

#endif // LANDSCAPE_HPP
