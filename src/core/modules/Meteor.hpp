/*
 * Stellarium
 * Copyright (C) 2004 Robert Spearman
 * Copyright (C) 2014 Marcos Cardinot
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
#define VISIBLE_RADIUS 457.8f        //! max visible distance in km

//! @class Meteor 
//! Models a single meteor.
//! Control of the meteor rate is performed in the MeteorMgr class.  Once
//! created, a meteor object only lasts for some amount of time, and then
//! "dies", after which, the update() member returns false.
class Meteor
{
public:
	struct MeteorModel
	{
		Vec3d obs;              //! observer position
		Vec3d position;         //! equatorial coordinate position
		Vec3d posTrain;         //! end of train
		float xyDist;           //! Distance in XY plane (orthogonal to radiant) from observer to meteor
		float minDist;          //! Nearest point to observer along path
		float initialAlt;       //! Initial meteor altitude above the Earth surface.
		float finalAlt;         //! Final meteor altitude (end of burn point).
		float mag;              //! Apparent magnitude at head, 0-1
		int firstBrightSegment; //! First bright segment of the train
	};

	//! Create a Meteor object.
	//! @param v the velocity of the meteor in km/s.
	Meteor(const StelCore*, float v);
	virtual ~Meteor();
	
	//! Updates the position of the meteor, and expires it if necessary.
	//! @return true of the meteor is still alive, else false.
	bool update(double deltaTime);
	
	//! Draws the meteor.
	void draw(const StelCore* core, StelPainter& sPainter);

	//! Draws the meteor train. (useful to be reused in MeteorShowers plugin)
	static void drawTrain(const StelCore* core, StelPainter& sPainter, const MeteorModel& mm,
			      const Mat4d& viewMatrix, const float thickness, const int segments,
			      QList<Vec4f> lineColorArray, QList<Vec4f> trainColorArray);

	//! Draws the meteor bolide. (useful to be reused in MeteorShowers plugin)
	static void drawBolide(const StelCore* core, StelPainter &sPainter, const MeteorModel& mm,
			       const Mat4d& viewMatrix, const float bolideSize);

	//! Calculates the train thickness and bolide size.
	static void calculateThickness(const StelCore* core, float &thickness, float &bolideSize);

	//! <colorName, intensity>
	typedef QPair<QString, int> colorPair;

	//! Updates parameters of meteor model. (useful to be reused in MeteorShowers plugin)
	//! @return true of the meteor is still alive, else false.
	static bool updateMeteorModel(double deltaTime, double speed, MeteorModel &mm);

	//! Builds Meteor Model
	//! @return true if alive, else false.
	static bool initMeteorModel(const StelCore *core, const int segments, MeteorModel &mm);

	//! Determine color arrays of line and prism used to draw meteor train.
	static void buildColorArrays(const int segments,
				     const QList<colorPair> colors,
				     QList<Vec4f> &lineColorArray,
				     QList<Vec4f> &trainColorArray);

	static StelTextureSP bolideTexture;

private:
	static void insertVertex(const StelCore* core, const Mat4d& viewMatrix,
				 QVector<Vec3d> &vertexArray, Vec3d vertex);
	static Vec4f getColorFromName(QString colorName);
	QList<colorPair> getRandColor();

	bool m_alive;	//! Indicate if the meteor it still visible

	float m_speed;			//! Velocity of meteor in km/s
	Mat4d m_viewMatrix;		//! tranformation matrix to align radiant with earth direction of travel
	MeteorModel meteor;		//! Parameters of meteor model

	QList<Vec4f> m_trainColorArray;
	QList<Vec4f> m_lineColorArray;
	const int m_segments;	//! Number of segments along the train (useful to curve along projection distortions)
};


#endif // _METEOR_HPP_
