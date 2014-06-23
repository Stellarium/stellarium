/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013 Marcos Cardinot
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

#ifndef _METEORSTREAM_HPP_
#define _METEORSTREAM_HPP_

#include "Meteor.hpp"
#include "VecMath.hpp"

class StelCore;
class StelPainter;

// all in km - altitudes make up meteor range
#define EARTH_RADIUS 6369.f
#define VISIBLE_RADIUS 457.8f
#define HIGH_ALTITUDE 115.f
#define LOW_ALTITUDE 70.f

//! @class MeteorStream
//! Models a single meteor.
//! Control of the meteor rate is performed in the MeteorShowers class.  Once
//! created, a meteor object only lasts for some amount of time, and then
//! "dies", after which, the update() member returns false.  The live/dead
//! status of a meteor may also be determined using the isAlive member.
class MeteorStream
{
	friend class MeteorShowers;

public:
	//! Create a Meteor object.
	//! @param the speed of the meteor in km/s.
	//! @param rAlpha the radiant alpha in rad
	//! @param rDelta the radiant delta in rad
	//! @param pidx population index
	MeteorStream(const StelCore*,
		     double speed,
		     double radiantAlpha,
		     double radiantDelta,
		     float pidx,
		     QList<MeteorShower::colorPair> colors);

	virtual ~MeteorStream();

	//! Updates the position of the meteor, and expires it if necessary.
	//! @return true of the meteor is still alive, else false.
	bool update(double deltaTime);

	//! Draws the meteor.
	void draw(const StelCore* core, StelPainter& sPainter);

private:
	void insertVertex(const StelCore* core, QVector<Vec3d> &vertexArray, Vec3d vertex);
	Vec4f getColor(QString colorName);

	bool m_alive;             //! Indicate if the meteor it still visible

	double m_speed;                 //! Velocity of meteor in km/s
	Mat4d m_viewMatrix;         //! View Matrix
	Meteor::MeteorModel meteor; //! Parameters of meteor model

	QList<Vec4f> m_trainColorArray;
	QList<Vec4f> m_lineColorArray;
	int m_segments;                 //! Number of segments along the train
};

#endif // _METEORSTREAM_HPP_
