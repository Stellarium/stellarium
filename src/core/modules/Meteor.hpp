/*
 * Stellarium
 * This file Copyright (C) 2004 Robert Spearman
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

#include "MeteorMgr.hpp"
#include "VecMath.hpp"

class StelCore;
class StelPainter;

// all in km - altitudes make up meteor range
#define EARTH_RADIUS 6369.f
#define HIGH_ALTITUDE 115.f
#define LOW_ALTITUDE 70.f
#define VISIBLE_RADIUS 457.8f

//! @class Meteor 
//! Models a single meteor.
//! Control of the meteor rate is performed in the MeteorMgr class.  Once
//! created, a meteor object only lasts for some amount of time, and then
//! "dies", after which, the update() member returns false.  The live/dead
//! status of a meteor may also be determined using the isAlive member.
class Meteor
{
public:
	//! Create a Meteor object.
	//! @param v the velocity of the meteor in km/s.
	Meteor(const StelCore*, double v);
	virtual ~Meteor();
	
	//! Updates the position of the meteor, and expires it if necessary.
	//! @return true of the meteor is still alive, else false.
	bool update(double deltaTime);
	
	//! Draws the meteor.
	void draw(const StelCore* core, StelPainter& sPainter);
	
	//! Determine if a meteor is alive or has burned out.
	//! @return true if alive, else false.
	bool isAlive(void);
	
private:
	void insertVertex(const StelCore* core, QVector<Vec3d> &vertexArray, Vec3d vertex);
	Vec4f getColor(QString colorName);

	bool m_alive;       //! Indicate if the meteor it still visible

	Mat4d m_viewMatrix; //! tranformation matrix to align radiant with earth direction of travel
	Vec3d m_obs;        //! observer position
	Vec3d m_position;   //! equatorial coordinate position
	Vec3d m_posTrain;   //! end of train

	double m_speed;           //! Velocity of meteor in km/s
	double m_xydistance;      //! Distance in XY plane (orthogonal to meteor path) from observer to meteor
	double m_minDist;         //! Nearest point to observer along path
	double m_startH;          //! Start height above center of earth
	double m_endH;            //! End height
	float m_mag;	          //! Apparent magnitude at head, 0-1
	int m_segments;           //! Number of segments along the train
	int m_firstBrightSegment; //! First bright segment of the train
	double m_distMultiplier;  //! Scale magnitude due to changes in distance

	QList<MeteorMgr::colorPair> m_colors;
	QList<Vec4f> m_trainColorArray;
	QList<Vec4f> m_lineColorArray;
};


#endif // _METEOR_HPP_
