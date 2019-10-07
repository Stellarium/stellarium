/*
 * Stellarium
 * Copyright (C) 2014-2015 Marcos Cardinot
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

#ifndef METEOR_HPP
#define METEOR_HPP

#include "StelTextureTypes.hpp"
#include "VecMath.hpp"

#include <QList>
#include <QPair>

class StelCore;
class StelPainter;

#define EARTH_RADIUS 6378.f          //! earth_radius in km
#define EARTH_RADIUS2 40678884.f     //! earth_radius^2 in km
#define MAX_ALTITUDE 120.f           //! max meteor altitude in km
#define MIN_ALTITUDE 80.f            //! min meteor altitude in km

//! @class Meteor 
//! Models a single meteor.
//! Once created, a meteor object only lasts for some amount of time,
//! and then "dies", after which, the update() member returns false.
//! @author Marcos Cardinot <mcardinot@gmail.com>
class Meteor
{
public:
	//! <colorName, intensity>
	typedef QPair<QString, int> ColorPair;

	//! Create a Meteor object.
	Meteor(const StelCore* core, const StelTextureSP &bolideTexture);
	virtual ~Meteor();

	//! Initialize meteor
	void init(const float& radiantAlpha, const float& radiantDelta,
		  const float& speed, const QList<ColorPair> colors);

	//! Updates the position of the meteor, and expires it if necessary.
	//! @param deltaTime the time increment in seconds since the last call.
	//! @return true of the meteor is still alive, else false.
	virtual bool update(double deltaTime);
	
	//! Draws the meteor.
	virtual void draw(const StelCore* core, StelPainter& sPainter);

	//! Indicate if the meteor still visible.
	bool isAlive() const { return m_alive; }
	//! Set meteor absolute magnitude.
	void setAbsMag(float mag) { m_absMag = mag; }
	//! Get meteor absolute magnitude.
	float absMag() const { return m_absMag; }

private:
	//! Determine color vectors of line and prism used to draw meteor train.
	void buildColorVectors(const QList<ColorPair> colors);

	//! get RGB from color name
	static Vec4f getColorFromName(QString colorName);

	//! Calculates the train thickness and bolide size.
	static void calculateThickness(const StelCore* core, double &thickness, double &bolideSize);

	//! Draws the meteor bolide.
	void drawBolide(StelPainter &sPainter, const double &bolideSize);

	//! Draws the meteor train.
	void drawTrain(StelPainter& sPainter, const double &thickness);

	//! Calculates the z-component of a meteor as a function of meteor zenith angle
	static float meteorZ(float zenithAngle, float altitude);

	//! find meteor position in horizontal coordinate system
	Vec3d radiantToAltAz(Vec3d position);

	//! find meteor position in radiant coordinate system
	Vec3d altAzToRadiant(Vec3d position);

	const StelCore* m_core;         //! The associated StelCore instance.

	bool m_alive;                   //! Indicates if the meteor it still visible.
	double m_speed;                  //! Velocity of meteor in km/s.
	Mat4d m_matAltAzToRadiant;      //! Rotation matrix to convert from horizontal to radiant coordinate system.
	Vec3d m_position;               //! Meteor position in radiant coordinate system.
	Vec3d m_posTrain;               //! End of train in radiant coordinate system.
	float m_initialZ;               //! Initial z-component of the meteor in radiant coordinates.
	float m_finalZ;                 //! Final z-compoenent of the meteor in radiant coordinates.
	float m_minDist;                //! Shortest distance between meteor and observer.
	float m_absMag;                 //! Absolute magnitude [0, 1]
	float m_aptMag;                 //! Apparent magnitude [0, 1]

	StelTextureSP m_bolideTexture;  //! Meteor bolide texture
	const int m_segments;           //! Number of segments along the train (useful to curve along projection distortions)
	QVector<Vec4f> m_trainColorVector;
	QVector<Vec4f> m_lineColorVector;
};

#endif // METEOR_HPP
