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
//! Don't use this class direcly, use the LandscapeMgr.
class Landscape
{
public:
	Landscape(float _radius = 2.f);
	virtual ~Landscape();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId) = 0;
	virtual void draw(StelCore* core) = 0;
	void update(double deltaTime)
	{
		landFader.update((int)(deltaTime*1000));
		fogFader.update((int)(deltaTime*1000));
	}

	//! Set the brightness of the landscape
	void setBrightness(const float b) {skyBrightness = b;}

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
	//! Return default atmospheric temperature, for refraction computation, or -1000 for "unknown/no change".
	float getDefaultAtmosphericTemperature() const {return defaultTemperature;}
	//! Return default atmospheric temperature, for refraction computation.
	//! returns -1 to signal "standard conditions", or -2 for "unknown/invalid/no change"
	float getDefaultAtmosphericPressure() const {return defaultPressure;}

	//! Set the z-axis rotation (offset from original value when rotated
	void setZRotation(float d) {angleRotateZOffset = d;}

protected:
	//! Load attributes common to all landscapes
	//! @param landscapeIni A reference to an existant QSettings object which describes the landscape
	//! @param landscapeId The name of the directory for the landscape files (e.g. "ocean")
	void loadCommon(const QSettings& landscapeIni, const QString& landscapeId);

	//! search for a texture in landscape directory, else global textures directory
	//! @param basename The name of a texture file, e.g. "fog.png"
	//! @param landscapeId The landscape ID (directory name) to which the texture belongs
	//! @exception misc possibility of throwing "file not found" exceptions
	const QString getTexturePath(const QString& basename, const QString& landscapeId);
	float radius;
	QString name;
	float skyBrightness;
	float nightBrightness;
	bool validLandscape;   // was a landscape loaded properly?
	LinearFader landFader;
	LinearFader fogFader;
	QString author;
	QString description;
	// GZ patched, these can now be set in landscape.ini:
	int rows; // horizontal rows
	int cols; // vertical columns
        int defaultBortleIndex; // light pollution from landscape.ini, or -1(no change)
        int defaultFogSetting; // fog flag setting from landscape.ini: -1(no change), 0(off), 1(on)
        double defaultExtinctionCoefficient; // atmospheric_extinction_coefficient from landscape.ini or -1
        double defaultTemperature; // atmospheric_temperature from landscape.ini or -1000.0
        double defaultPressure; // atmospheric_pressure from landscape.ini or -1.0

	typedef struct
	{
		StelTextureSP tex;
		float texCoords[4];
	} landscapeTexCoord;

	StelLocation location;
	float angleRotateZ;
	float angleRotateZOffset;
};


class LandscapeOldStyle : public Landscape
{
public:
	LandscapeOldStyle(float radius = 2.f);
	virtual ~LandscapeOldStyle();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(StelCore* core);
	void create(bool _fullpath, QMap<QString, QString> param);
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
	float decorAltAngle;
	float decorAngleShift;
	float groundAngleShift;
	float groundAngleRotateZ;
	int drawGroundFirst;
	bool tanMode;		// Whether the angles should be converted using tan instead of sin
	bool calibrated;	// if true, the documented altitudes are inded correct (the original code is buggy!)
	struct LOSSide
	{
		StelVertexArray arr;
		StelTextureSP tex;
	};

	QList<LOSSide> precomputedSides;
};

class LandscapeFisheye : public Landscape
{
public:
	LandscapeFisheye(float radius = 1.f);
	virtual ~LandscapeFisheye();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(StelCore* core);
	void create(const QString name, const QString& maptex, float texturefov, float angleRotateZ);
private:

	StelTextureSP mapTex;
	float texFov;
};


class LandscapeSpherical : public Landscape
{
public:
	LandscapeSpherical(float radius = 1.f);
	virtual ~LandscapeSpherical();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(StelCore* core);
	void create(const QString name, const QString& maptex, float angleRotateZ);
private:

	StelTextureSP mapTex;
};

#endif // _LANDSCAPE_HPP_
