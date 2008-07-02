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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _LANDSCAPE_HPP_
#define _LANDSCAPE_HPP_

#include <QMap>
#include "vecmath.h"
#include "ToneReproducer.hpp"
#include "Projector.hpp"
#include "Navigator.hpp"
#include "Fader.hpp"
#include "StelUtils.hpp"
#include "STextureTypes.hpp"

class QSettings;

//! Store and manages the displaying of the Landscape
class Landscape
{
public:
	Landscape(float _radius = 2.);
	virtual ~Landscape();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId) = 0;
	virtual void draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav) = 0;
	void update(double deltaTime)
	{
		landFader.update((int)(deltaTime*1000));
		fogFader.update((int)(deltaTime*1000));
	}

	//! Set the brightness of the landscape
	void setBrightness(float b) {skyBrightness = b;}
	
	//! Set whether landscape is displayed (does not concern fog)
	void setFlagShow(bool b) {landFader=b;}
	//! Get whether landscape is displayed (does not concern fog)
	bool getFlagShow() const {return (bool)landFader;}
	//! Set whether fog is displayed
	void setFlagShowFog(bool b) {fogFader=b;}
	//! Get whether fog is displayed
	bool getFlagShowFog() const {return (bool)fogFader;}
	//! Get landscape name
	QString getName() const {return name;}
	//! Get landscape author name
	QString getAuthorName() const {return author;}
	//! Get landscape description
	QString getDescription() const {return description;}
	
	//! Return the (English) planet name for the landscape
	QString getPlanetName(void) { return planet; }	
	//! Return the latitude for the landscape in degree
	double getLatitude(void) { return latitude; }
	//! Return the longitude for the landscape in degree
	double getLongitude(void) { return longitude; }
	//! Return the altitude for the landscape in m
	double getAltitude(void) { return altitude; }

protected:
	//! Load attributes common to all landscapes
	//! @param A reference to an existant QSettings object which describes the landscape
	//! @param The name of the directory for the landscape files (e.g. "ocean")
	void loadCommon(const QSettings& landscapeIni, const QString& landscapeId);
	
	//! search for a texture in landscape directory, else global textures directory
	//! @param basename The name of a texture file, e.g. "fog.png"
	//! @param landscapeId The landscape ID (directory name) to which the texture belongs
	//! @exception misc possibility of throwing boost:filesystem or "file not found" exceptions
	const QString getTexturePath(const QString& basename, const QString& landscapeId);
	float radius;
	QString name;
	float skyBrightness;
	bool validLandscape;   // was a landscape loaded properly?
	LinearFader landFader;
	LinearFader fogFader;
	QString author;
	QString description;
	QString planet;
	double latitude;
	double longitude;
	double altitude;
	
	typedef struct
	{
		STextureSP tex;
		float texCoords[4];
	} landscapeTexCoord;
};


class LandscapeOldStyle : public Landscape
{
public:
	LandscapeOldStyle(float _radius = 2.);
    virtual ~LandscapeOldStyle();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav);
	void create(bool _fullpath, QMap<QString, QString> param);
private:
	void drawFog(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const;
	void drawDecor(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const;
	void drawGround(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const;
	STextureSP* sideTexs;
	int nbSideTexs;
	int nbSide;
	landscapeTexCoord* sides;
	STextureSP fogTex;
	landscapeTexCoord fogTexCoord;
	STextureSP groundTex;
	landscapeTexCoord groundTexCoord;
	int nbDecorRepeat;
	float fogAltAngle;
	float fogAngleShift;
	float decorAltAngle;
	float decorAngleShift;
	float decorAngleRotatez;
	float groundAngleShift;
	float groundAngleRotatez;
	int drawGroundFirst;
	bool tanMode;	// Whether the angles should be converted using tan instead of sin 
};

class LandscapeFisheye : public Landscape
{
public:
	LandscapeFisheye(float _radius = 1.);
	virtual ~LandscapeFisheye();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav);
	void create(const QString _name, bool _fullpath, const QString& _maptex,
	            double _texturefov, double angleRotatez);
private:

	STextureSP mapTex;
	float texFov;
	float angleRotatez;
};


class LandscapeSpherical : public Landscape
{
public:
	LandscapeSpherical(float _radius = 1.);
	virtual ~LandscapeSpherical();
	virtual void load(const QSettings& landscapeIni, const QString& landscapeId);
	virtual void draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav);
	void create(const QString _name, bool _fullpath,
	            const QString& _maptex, double angleRotatez);
private:

	STextureSP mapTex;
	float angleRotatez;
};

#endif // _LANDSCAPE_HPP_
