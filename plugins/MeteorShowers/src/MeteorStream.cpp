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

#include <cstdlib>

#include "MeteorShowers.hpp"
#include "MeteorStream.hpp"
#include "StelCore.hpp"
#include "StelToneReproducer.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"
#include "StelTexture.hpp"

StelTextureSP MeteorStream::bolideTexture;

MeteorStream::MeteorStream(const StelCore* core,
			   double speed,
			   double radiantAlpha,
			   double radiantDelta,
			   float pidx,
			   QList<MeteorShower::colorPair> colors)
	: m_speed(speed)
	, m_segments(10)
{
	// if speed is zero, use a random value
	if (!speed)
	{
		m_speed = 11+(double)rand()/((double)RAND_MAX+1)*61;  // abs range 11-72 km/s
	}

	// view matrix of meteor model
	m_viewMatrix = Mat4d::zrotation(radiantAlpha) * Mat4d::yrotation(M_PI_2 - radiantDelta);

	// building meteor model
	m_alive = Meteor::initMeteorModel(core, m_segments, m_viewMatrix, meteor);
	if (!m_alive)
	{
		return;
	}

	// implements the population index
	float oneMag = -0.2; // negative, working in different scale ( 0 to 1 - where 1 is brighter)
	if (pidx) // is not zero
	{
		if (rand()%100 < 100.f/pidx) // probability
		{
			meteor.mag += oneMag;  // (m+1)
		}
	}

	// determine the meteor color
	if (colors.isEmpty()) {
		colors.push_back(MeteorShower::colorPair("white", 100));
	} else {
		// handle cases when the total intensity is less than 100
		int totalIntensity = 0;
		int indexWhite = -1;
		for (int i=0; i<colors.size(); i++) {
			totalIntensity += colors.at(i).second;
			if (colors.at(i).first == "white") {
				indexWhite = i;
			}
		}

		int increaseWhite = 0;
		if (totalIntensity > 100) {
			qDebug() << "MeteorShowers plugin (showers.json): Total intensity must be less than 100";
			colors.clear();
			colors.push_back(MeteorShower::colorPair("white", 100));
		} else {
			increaseWhite = 100 - totalIntensity;
		}

		if (increaseWhite > 0) {
			if (indexWhite == -1) {
				colors.push_back(MeteorShower::colorPair("white", increaseWhite));
			} else {
				colors[indexWhite].second = increaseWhite;
			}
		}
	}

	// building lineColorArray and trainColorArray
	Meteor::buildColorArrays(m_segments, colors, m_lineColorArray, m_trainColorArray);
}

MeteorStream::~MeteorStream()
{
}

// returns true if alive
bool MeteorStream::update(double deltaTime)
{
	if (!m_alive)
	{
		return false;
	}

	m_alive = Meteor::updateMeteorModel(deltaTime, m_speed, meteor);

	return m_alive;
}

void MeteorStream::insertVertex(const StelCore* core, QVector<Vec3d> &vertexArray, Vec3d vertex) {
	// convert to equ
	vertex.transfo4d(m_viewMatrix);
	// convert to local and correct for earth radius
	//[since equ and local coordinates in stellarium use same 0 point!]
	vertex = core->j2000ToAltAz(vertex);
	vertex[2] -= EARTH_RADIUS;
	// 1216 is to scale down under 1 for desktop version
	vertex/=1216;

	vertexArray.push_back(vertex);
}

// returns true if visible
// Assumes that we are in local frame
void MeteorStream::draw(const StelCore* core, StelPainter& sPainter)
{
	if (!m_alive)
	{
		return;
	}

	double maxFOV = core->getMovementMgr()->getMaxFov();
	double FOV = core->getMovementMgr()->getCurrentFov();
	double thickness = 2*log(FOV + 0.25)/(1.2*maxFOV - (FOV + 0.25)) + 0.01;
	if (FOV <= 0.5)
	{
		thickness = 0.013 * FOV; // decreasing faster
	}
	double bolideSize = thickness*3;

	// train (triangular prism)
	//
	QVector<Vec3d> vertexArrayLine;
	QVector<Vec3d> vertexArrayL;
	QVector<Vec3d> vertexArrayR;
	QVector<Vec3d> vertexArrayTop;

	Vec3d posTrainB = meteor.posTrain;
	posTrainB[0] += thickness*0.7;
	posTrainB[1] += thickness*0.7;
	Vec3d posTrainL = meteor.posTrain;
	posTrainL[1] -= thickness;
	Vec3d posTrainR = meteor.posTrain;
	posTrainR[0] -= thickness;

	for (int i=0; i<m_segments; i++) {
		double mag = meteor.mag * i/(3* (m_segments-1));
		if (i > meteor.firstBrightSegment) {
			mag *= 12/5;
		}

		double height = meteor.posTrain[2] + i*(meteor.position[2] - meteor.posTrain[2])/(m_segments-1);
		Vec3d posi;

		posi = meteor.posTrain;
		posi[2] = height;
		insertVertex(core, vertexArrayLine, posi);

		posi = posTrainB;
		posi[2] = height;
		insertVertex(core, vertexArrayL, posi);
		insertVertex(core, vertexArrayR, posi);

		posi = posTrainL;
		posi[2] = height;
		insertVertex(core, vertexArrayL, posi);
		insertVertex(core, vertexArrayTop, posi);

		posi = posTrainR;
		posi[2] = height;
		insertVertex(core, vertexArrayR, posi);
		insertVertex(core, vertexArrayTop, posi);

		m_lineColorArray[i][3] = mag;
		m_trainColorArray[i*2][3] = mag;
		m_trainColorArray[i*2+1][3] = mag;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter.enableClientStates(true, false, true);
	sPainter.setColorPointer(4, GL_FLOAT, m_trainColorArray.toVector().constData());
	if (thickness) {
		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayL.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayL.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayR.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayR.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayTop.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayTop.size(), 0, true);
	}
	sPainter.setColorPointer(4, GL_FLOAT, m_lineColorArray.toVector().constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayLine.constData());
	sPainter.drawFromArray(StelPainter::LineStrip, vertexArrayLine.size(), 0, true);

	// bolide
	//
	QVector<Vec3d> vertexArrayBolide;
	QVector<Vec4f> colorArrayBolide;
	Vec4f bolideColor = Vec4f(1,1,1,meteor.mag);

	Vec3d topLeft = meteor.position;
	topLeft[1] -= bolideSize;
	insertVertex(core, vertexArrayBolide, topLeft);
	colorArrayBolide.push_back(bolideColor);

	Vec3d topRight = meteor.position;
	topRight[0] -= bolideSize;
	insertVertex(core, vertexArrayBolide, topRight);
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomRight = meteor.position;
	bottomRight[1] += bolideSize;
	insertVertex(core, vertexArrayBolide, bottomRight);
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomLeft = meteor.position;
	bottomLeft[0] += bolideSize;
	insertVertex(core, vertexArrayBolide, bottomLeft);
	colorArrayBolide.push_back(bolideColor);

	glBlendFunc(GL_ONE, GL_ONE);

	sPainter.enableClientStates(true, true, true);
	MeteorStream::bolideTexture->bind();

	static const float texCoordData[] = {1.,0., 0.,0., 0.,1., 1.,1.};
	sPainter.setTexCoordPointer(2, GL_FLOAT, texCoordData);
	sPainter.setColorPointer(4, GL_FLOAT, colorArrayBolide.constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayBolide.constData());
	sPainter.drawFromArray(StelPainter::TriangleFan, vertexArrayBolide.size(), 0, true);

	glDisable(GL_BLEND);
	sPainter.enableClientStates(false);
}
