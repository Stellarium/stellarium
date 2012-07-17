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

#include "renderer/StelVertexAttribute.hpp"
#include "renderer/StelVertexBuffer.hpp"
#include "StelProjector.hpp"
#include "VecMath.hpp"
class StelCore;

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
	void draw(const StelCore* core, StelProjectorP projector, class StelRenderer* renderer);
	
	//! Determine if a meteor is alive or has burned out.
	//! @return true if alive, else false.
	bool isAlive(void);
	
private:
	//! Vertex with a 3D position and a color.
	struct Vertex
	{
		Vec3f position;
		Vec4f color;
		Vertex(const Vec3d& pos, const Vec4f& color) 
			: position(Vec3f(pos[0], pos[1], pos[2])), color(color) {}
		VERTEX_ATTRIBUTES(Vec3f Position, Vec4f Color);
	};

	//! Vertex buffer used for drawing.
	StelVertexBuffer<Vertex>* vertexBuffer;

	Mat4d mmat; // tranformation matrix to align radiant with earth direction of travel
	Vec3d obs;  // observer position in meteor coord. system
	Vec3d position;  // equatorial coordinate position
	Vec3d posInternal;  // middle of train
	Vec3d posTrain;  // end of train
	bool train;      // point or train visible?
	double startH;  // start height above center of earth
	double endH;    // end height
	double velocity; // km/s
	bool alive;      // is it still visible?
	float mag;	   // Apparent magnitude at head, 0-1
	float maxMag;  // 0-1
	float absMag;  // absolute magnitude
	float visMag;  // visual magnitude at observer
	double xydistance; // distance in XY plane (orthogonal to meteor path) from observer to meteor
	double initDist;  // initial distance from observer
	double minDist;  // nearest point to observer along path
	double distMultiplier;  // scale magnitude due to changes in distance 
};


#endif // _METEOR_HPP_
