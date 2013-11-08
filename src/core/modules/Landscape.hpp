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

#ifndef _LANDSCAPE_HPP_
#define _LANDSCAPE_HPP_

#include <QMap>
#include <QImage>
#include "VecMath.hpp"
#include "StelToneReproducer.hpp"
#include "StelProjector.hpp"

#include "StelFader.hpp"
#include "StelUtils.hpp"
#include "StelTextureTypes.hpp"
#include "StelLocation.hpp"

class QSettings;
class StelLocation;
class StelCore;
class StelPainter;

//! @class Landscape
//! Store and manages the displaying of the Landscape.
//! Don't use this class directly, use the LandscapeMgr.
//! A landscape's most important element is a photo panorama.
//! Optional components include:
//!  - A fog texture that is displayed with the Fog [F] command.
//!  - A location. It is possible to auto-move to the location when loading.
//!  - Atmospheric conditions: temperature/pressure/extinction coefficient.
//!  - Light pollution information (Bortle index)
//!  - A night texture that gets blended over the dimmed daylight panorama. (Currently for Spherical and Fisheye only)
//!  - You can set a minimum brightness level to prevent too dark landscape.
//! We discern:
//!   @param LandscapeId: The directory name of the landscape.
//!   @param name: The landscape name as specified in the LandscapeIni (may contain spaces, UTF8, ...) GZ:VERIFY!
//! TODO: 1) Add a way to "ask" a landscape if a certain point is transparent. Useful for accurate rise/set predictions!
//!       2) Implement LandscapePolygonal
class Landscape
{
public:
	Landscape(float _radius = 2.f);
	virtual ~Landscape();
	//! Load landscape.
	//! @param landscapeIni A reference to an existing QSettings object which describes the landscape
	//! @param landscapeId The name of the directory for the landscape files (e.g. "ocean")
	//         TBD: Is this really only last part of directory? Or fullPath?
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId) = 0;
	virtual void draw(StelCore* core) = 0;
	void update(double deltaTime)
	{
		landFader.update((int)(deltaTime*1000));
		fogFader.update((int)(deltaTime*1000));
	}

	//! Set the brightness of the landscape plus brightness of optional add-on night lightscape.
	//! This is called in each draw().
	void setBrightness(const float b, const float pollutionBrightness=0.0f) {skyBrightness = b; lightScapeBrightness=pollutionBrightness; }

	//! Set whether landscape is displayed (does not concern fog)
	void setFlagShow(const bool b) {landFader=b;}
	//! Get whether landscape is displayed (does not concern fog)
	bool getFlagShow() const {return (bool)landFader;}
	//! Set whether fog is displayed
	void setFlagShowFog(const bool b) {fogFader=b;}
	//! Get whether fog is displayed
	bool getFlagShowFog() const {return (bool)fogFader;}
	//! Get landscape name
	QString getName() const {return name;}
	//! Get landscape author name
	QString getAuthorName() const {return author;}
	//! Get landscape description
	QString getDescription() const {return description;}

	//! Return the associated location or NULL
	const StelLocation& getLocation() const {return location;}
  	//! Return default Bortle index (light pollution value) or -1 (unknown/no change)
	int getDefaultBortleIndex() const {return defaultBortleIndex;}
	//! Return default fog setting (0/1) or -1 (no change)
	int getDefaultFogSetting() const {return defaultFogSetting;}
  	//! Return default atmosperic extinction, mag/airmass, or -1 (no change)
	float getDefaultAtmosphericExtinction() const {return defaultExtinctionCoefficient;}
	//! Return configured atmospheric temperature, for refraction computation, or -1000 for "unknown/no change".
	float getDefaultAtmosphericTemperature() const {return defaultTemperature;}
	//! Return configured atmospheric pressure, for refraction computation.
	//! returns -1 to signal "standard conditions" [compute from altitude], or -2 for "unknown/invalid/no change"
	float getDefaultAtmosphericPressure() const {return defaultPressure;}
	//! Return default brightness for landscape
	//! returns -1 to signal "standard conditions" (use default value from config.ini)
	float getLandscapeNightBrightness() const {return defaultBrightness;}
	// GZ: RENAME TO getLandscapeMinBrightness()?

	//! Set an additional z-axis (azimuth) rotation after landscape has been loaded.
	//! This is intended for special uses such as when the landscape consists of
	//! a vehicle which might change orientation over time (e.g. a ship). It is called
	//! e.g. by the LandscapeMgr. Contrary to that, the purpose of the azimuth rotation
	//! (landscape/[decor_]angle_rotatez) in landscape.ini is to orient the pano.
	//! @param d the rotation angle in degrees.
	void setZRotation(float d) {angleRotateZOffset = d * M_PI/180.0f;}

	//! Get whether the landscape is currently fully visible (i.e. opaque).
	bool getIsFullyVisible() const {return landFader.getInterstate() >= 0.999f;}

	// GZ: NEW FUNCTION: USEFUL IF ALL SUBCLASSES IMPLEMENT THIS TEXTURE LOOKUP.
	//! can be used to find sunrise or visibility questions on the real-world landscape horizon.
	//! Default implementation indicated the horizon never blocks view, i.e. sunrise etc.
	//! can be computed on the math horizon without interfering.
	virtual float getOpacity(const Vec3d azalt) const {Q_UNUSED(azalt); return 0.0f; }
	
protected:
	//! Load attributes common to all landscapes
	//! @param landscapeIni A reference to an existing QSettings object which describes the landscape
	//! @param landscapeId The name of the directory for the landscape files (e.g. "ocean")
	void loadCommon(const QSettings& landscapeIni, const QString& landscapeId);

	//! search for a texture in landscape directory, else global textures directory
	//! @param basename The name of a texture file, e.g. "fog.png"
	//! @param landscapeId The landscape ID (directory name) to which the texture belongs
	//! @exception misc possibility of throwing "file not found" exceptions
	const QString getTexturePath(const QString& basename, const QString& landscapeId);
	float radius;
	QString name;          //! GZ:  Given in landscape.ini:[landscape]name TBD: VERIFY
	QString author;        //! GZ:  Given in landscape.ini:[landscape]author TBD: VERIFY
	QString description;   //! GZ:  Given in landscape.ini:[landscape]description TBD: VERIFY
	float skyBrightness;
	float nightBrightness;
	float defaultBrightness; //! GZ: some small brightness, allows minimum visibility that cannot be underpowered. VERIFY
	float lightScapeBrightness; //! can be used to draw nightscape texture (e.g. city light pollution), if available.
	bool validLandscape;   //! was a landscape loaded properly?
	LinearFader landFader; //! Used to slowly fade in/out landscape painting.
	LinearFader fogFader;  //! Used to slowly fade in/out fog painting.
	// GZ patched, these can now be set in landscape.ini:
	int rows; //! horizontal rows.  May be given in landscape.ini. More indicates higher accuracy, but is slower.
	int cols; //! vertical columns. May be given in landscape.ini. More indicates higher accuracy, but is slower.
	int defaultBortleIndex; // light pollution from landscape.ini, or -1(no change). Default: -1
	int defaultFogSetting; // fog flag setting from landscape.ini: -1(no change), 0(off), 1(on). Default: -1
	double defaultExtinctionCoefficient; // atmospheric_extinction_coefficient from landscape.ini or -1 (no change; default).
	double defaultTemperature; // atmospheric_temperature from landscape.ini or -1000.0 (no change; default)
	double defaultPressure; // atmospheric_pressure from landscape.ini or -1.0 (compute from location.altitude). VERIFY!

	typedef struct
	{
		StelTextureSP tex;
		float texCoords[4];
	} landscapeTexCoord;
	// GZ: Maybe this can be used to configure upper border, and a texture CLAMP setting?

	StelLocation location; //! OPTIONAL. If present, can be used to set location.
	float angleRotateZ;    //! [radians] if pano does not have its left border in the east, rotate in azimuth. Configured in landscape.ini[landscape]rotate
	float angleRotateZOffset; //! [radians] This is a rotation changeable at runtime via setZRotation (called by LandscapeMgr::setZRotation).
							  //! Not in landscape.ini: Used in special cases where the horizon may rotate, e.g. on a ship.
	// GZ NEW: Optional elements which, if present, describe a horizon polygon. They can be used to render a line or a filled region, esp. in LandscapePolygonal
	QString horizonFileName; //! GZ NEW: optional filename (within landscape dir) with a definition of the horizon line.
	SphericalPolygon *horizonPolygon; //! Optional element describing the horizon line.
									  //! Data shall be read from the file given as landscape.ini[landscape]horizon_list
									  //! For LandscapePolygonal, this is the only horizon data item.
	Vec3f horizonLineColor ;    //! for all horizon types, the horizonPolygon line, if specified, will be drawn in this color
								//! specified in landscape.ini[landscape]horizon_line_color. TBD: Default?
};

//! @class LandscapeOldStyle
//! This was the original landscape, introduced for decorative purposes. It segments the horizon in several tiles
//! (usually 4 or 8), therefore allowing very high resolution horizons also on limited hardware,
//! and closes the ground with a separate bottom piece. (You may want to configure a map with pointers or a compass rose!)
//! You can use panoramas created in equirectangular or cylindrical coordinates, for the latter case set
//! [landscape]tan_mode=true.
//! Until V0.10.5 there was an undetected bug involving vertical positioning. For historical reasons (many landscapes
//! were already configured and published), it was decided to keep this bug as feature, but a fix for new landscapes is
//! available: [landscape]calibrated=true. With this, vertical positioning is accurate.
//! As of 0.10.6, the fix is only valid for equirectangular panoramas.
//! TODO (still as of 0.12.*): implement [landscape]calibrated=true for [landscape]tan_mode=true
//! It is more involved to configure, but may be required if you require the resolution, e.g. for alignment studies
//! for archaeoastronomy. In this case, don't forget to set calibrated=true in landscape.ini.
class LandscapeOldStyle : public Landscape
{
public:
	LandscapeOldStyle(float radius = 2.f);
	virtual ~LandscapeOldStyle();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(StelCore* core);
	void create(bool _fullpath, QMap<QString, QString> param);
	virtual float getOpacity(const Vec3d azalt) const;
private:
	void drawFog(StelCore* core, StelPainter&) const;
	void drawDecor(StelCore* core, StelPainter&) const;
	void drawGround(StelCore* core, StelPainter&) const;
	QVector<double> groundVertexArr;
	QVector<float> groundTexCoordArr;
	StelTextureSP* sideTexs;
	int nbSideTexs;
	int nbSide;
	landscapeTexCoord* sides;
	StelTextureSP fogTex;
	landscapeTexCoord fogTexCoord;
	StelTextureSP groundTex;
	landscapeTexCoord groundTexCoord;
	int nbDecorRepeat;
	float fogAltAngle;
	float fogAngleShift;
	float decorAltAngle; // vertical extent of the side panels
	float decorAngleShift;
	float groundAngleShift; //! [radians]: altitude of the bottom plane. Usually negative and equal to decorAngleShift
	float groundAngleRotateZ; //! [radians]
	int drawGroundFirst;
	bool tanMode;		// Whether the angles should be converted using tan instead of sin, i.e., for a cylindrical pano
	bool calibrated;	// if true, the documented altitudes are indeed correct (the original code is buggy!)
	struct LOSSide
	{
		StelVertexArray arr;
		StelTextureSP tex;
	};

	QList<LOSSide> precomputedSides;
};

/////////////////////////////////////////////////////////
///
//! @class LandscapePolygonal
//! This uses the list of (usually measured) horizon altitudes to define the horizon. It will be colored
// GZ TODO: Implement...
class LandscapePolygonal : public Landscape
{
public:
	LandscapePolygonal(float radius = 1.f);
	virtual ~LandscapePolygonal();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(StelCore* core);
	void create(const QString _name, const QString& _lineFileName, const float _angleRotateZ=0.0f);
	virtual float getOpacity(const Vec3d azalt) const;
private:
	// we have inherited: horizonFileName, horizonPolygon, horizonPolygonColor
	Vec3f groundColor; //! specified in landscape.ini[landscape]ground_color.
};

///////////////////////////////////////////////////////////////
///
//! @class LandscapeFisheye
//! This uses a single image in fisheye projection. The image is typically square, ...
//! @param texFov:  field of view (opening angle) of the square texture, radians.
//! If @param angleRotateZ==0, the top image border is due south.
// GZ TODO: find out opening angle or details on projection?
class LandscapeFisheye : public Landscape
{
public:
	LandscapeFisheye(float radius = 1.f);
	virtual ~LandscapeFisheye();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(StelCore* core);
	//! Sample landscape texture for transparency/opacity. May be used for visibility, sunrise etc.
	//! @param azalt normalized direction in alt-az frame
	virtual float getOpacity(const Vec3d azalt) const;
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
	virtual ~LandscapeSpherical();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(StelCore* core);
	//! Sample landscape texture for transparency/opacity. May be used for visibility, sunrise etc.
	//! @param azalt normalized direction in alt-az frame
	//! @retval alpha (0=fully transparent, 1=fully opaque. Trees, leaves, glass etc may have intermediate values.)
	virtual float getOpacity(const Vec3d azalt) const;
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
	void create(const QString name, const QString& maptex, const QString &_maptexFog="", const QString& _maptexIllum="", const float _angleRotateZ=0.0f,
				const float _mapTexTop=90.0f, const float _mapTexBottom=-90.0f,
				const float _fogTexTop=90.0f, const float _fogTexBottom=-90.0f,
				const float _illumTexTop=90.0f, const float _illumTexBottom=-90.0f);
private:

	StelTextureSP mapTex;      //!< The equirectangular panorama texture
	StelTextureSP mapTexFog;   //!< Optional panorama of identical size (create as layer over the mapTex image in your favorite image processor).
							   //!< can also be smaller, just the texture is again mapped onto the same geometry.
	StelTextureSP mapTexIllum; //!< Optional panorama of identical size (create as layer over the mapTex image in your favorite image processor).
							   //!< To simulate light pollution (skyglow), street lights, light in windows, ... at night
	// These vars are here to conserve texture memory. They must be allowed to be different: a landscape may have its highest elevations at 15°, fog may reach from -25 to +15°,
	// light pollution may cover -5° (street lamps slightly below) plus parts of or even the whole sky. All have default values to simplify life.
	float mapTexTop;           //!< zenithal top angle of the landscape texture, radians
	float mapTexBottom;		   //!< zenithal bottom angle of the landscape texture, radians
	float fogTexTop;		   //!< zenithal top angle of the fog texture, radians
	float fogTexBottom;		   //!< zenithal bottom angle of the fog texture, radians
	float illumTexTop;		   //!< zenithal top angle of the illumination texture, radians
	float illumTexBottom;	   //!< zenithal bottom angle of the illumination texture, radians
	QImage *mapImage;          //!< The same image as mapTex, but stored in-mem for opacity sampling.
};

#endif // _LANDSCAPE_HPP_
