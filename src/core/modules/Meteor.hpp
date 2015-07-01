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

#ifndef _METEOR_HPP_
#define _METEOR_HPP_

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
#define BURN_ALTITUDE 45.f           //! meteors usually disintegrate at altitudes greater than 45km
#define VISIBLE_RADIUS 457.8f        //! max visible distance in km

//! @class Meteor 
//! Models a single meteor.
//! Once created, a meteor object only lasts for some amount of time,
//! and then "dies", after which, the update() member returns false.
//! @author Marcos Cardinot <mcardinot@gmail.com>
class Meteor
{
public:
	//! <colorName, intensity>
	typedef QPair<QString, int> colorPair;

	//! Create a Meteor object.
	Meteor(const StelCore* core, const StelTextureSP &bolideTexture);
	virtual ~Meteor();

	//! Initialize meteor
	void init(const float& radiantAlpha, const float& radiantDelta,
		  const float& speed, const QList<colorPair> colors);

	//! Updates the position of the meteor, and expires it if necessary.
	//! @return true of the meteor is still alive, else false.
	virtual bool update(double deltaTime);
	
	//! Draws the meteor.
	virtual void draw(const StelCore* core, StelPainter& sPainter);

	//! Indicate if the meteor it still visible.
	inline bool isAlive() { return m_alive; }
	//! Set meteor apparent magnitude.
	inline void setMag(float mag) { m_mag = mag; }
	//! Get meteor apparent magnitude.
	inline float mag() { return m_mag; }

private:
	//! Determine color arrays of line and prism used to draw meteor train.
	void buildColorArrays(const QList<colorPair> colors);

	//! get RGB from color name
	Vec4f getColorFromName(QString colorName);

	//! Calculates the train thickness and bolide size.
	void calculateThickness(const StelCore* core, float &thickness, float &bolideSize);

	//! Draws the meteor bolide.
	void drawBolide(const StelCore* core, StelPainter &sPainter, const float &bolideSize);

	//! Draws the meteor train.
	void drawTrain(const StelCore* core, StelPainter& sPainter, const float &thickness);

	//! Calculating meteor distance as a function of meteor zenith angle
	float meteorDistance(float zenithAngle, float altitude);

	//! find meteor position in horizontal coordinate system
	Vec3d radiantToAltAz(Vec3d position);

	//! find meteor position in radiant coordinate system
	Vec3d altAzToRadiant(Vec3d position);

	const StelCore* m_core;         //! The associated StelCore instance.

	bool m_alive;                   //! Indicate if the meteor it still visible.
	float m_speed;                  //! Velocity of meteor in km/s.
	Mat4d m_matAltAzToRadiant;      //! Rotation matrix to convert from horizontal to radiant coordinate system.
	Vec3d m_position;               //! Meteor position in radiant coordinate system.
	Vec3d m_posTrain;               //! End of train in radiant coordinate system.
	float m_xyDist;                 //! Distance in XY plane (orthogonal to radiant) from observer to meteor
	float m_initialDist;            //! Initial distance from observer to meteor.
	float m_finalDist;              //! Final distance from observer to meteor.
	float m_mag;                    //! Apparent magnitude at head, 0-1
	int m_firstBrightSegment;       //! First bright segment of the train

	StelTextureSP m_bolideTexture;  //! Meteor bolide texture
	const int m_segments;           //! Number of segments along the train (useful to curve along projection distortions)
	QList<Vec4f> m_trainColorArray;
	QList<Vec4f> m_lineColorArray;
};

#endif // _METEOR_HPP_
