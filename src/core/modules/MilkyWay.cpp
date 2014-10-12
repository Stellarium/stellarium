/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include "MilkyWay.hpp"
#include "StelFader.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelFileMgr.hpp"

#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPainter.hpp"

#include <QDebug>
#include <QSettings>

// Class which manages the displaying of the Milky Way
MilkyWay::MilkyWay()
	: color(1.f, 1.f, 1.f)
	, intensity(1.)
	, vertexArray()
	, debugOne(true)
{
	setObjectName("MilkyWay");
	fader = new LinearFader();
}

MilkyWay::~MilkyWay()
{
	delete fader;
	fader = NULL;
	
	delete vertexArray;
	vertexArray = NULL;
}

void MilkyWay::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	tex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/milkyway.png");
	setFlagShow(conf->value("astro/flag_milky_way").toBool());
	setIntensity(conf->value("astro/milky_way_intensity",1.f).toFloat());

	vertexArray = new StelVertexArray(StelPainter::computeSphereNoLight(1.f,1.f,20,20,1));
	vertexArray->colors.resize(vertexArray->vertex.length());
	vertexArray->colors.fill(Vec3f(1.0, 0.3, 0.9));
}


void MilkyWay::update(double deltaTime)
{
	fader->update((int)(deltaTime*1000));

	// TODO: Fill colors array with extincted versions of 1/1/1.
	// i.e. project Milky Way, read all vertices' horizon-relative z values, adjust colors.
}

void MilkyWay::setFlagShow(bool b){*fader = b;}
bool MilkyWay::getFlagShow() const {return *fader;}

void MilkyWay::draw(StelCore* core)
{
	if (!getFlagShow())
		return;

	StelProjector::ModelViewTranformP transfo = core->getJ2000ModelViewTransform();
	transfo->combine(Mat4d::xrotation(M_PI/180.*23.)*
					 Mat4d::yrotation(M_PI/180.*120.)*
					 Mat4d::zrotation(M_PI/180.*7.));

	const StelProjectorP prj = core->getProjection(transfo);
	StelToneReproducer* eye = core->getToneReproducer();

	Q_ASSERT(tex);	// A texture must be loaded before calling this

	// This RGB color corresponds to the night blue scotopic color = 0.25, 0.25 in xyY mode.
	// since milky way is always seen white RGB value in the texture (1.0,1.0,1.0)
	Vec3f c = Vec3f(0.34165f, 0.429666f, 0.63586f);

	float lum = core->getSkyDrawer()->surfacebrightnessToLuminance(13.5f);

	// Get the luminance scaled between 0 and 1
	float aLum =eye->adaptLuminanceScaled(lum*fader->getInterstate());

	// Bound a maximum luminance
	aLum = qMin(0.38f, aLum*2.f);

	// intensity of 1.0 is "proper", but allow boost for dim screens
	c*=aLum*intensity;

	if (c[0]<0) c[0]=0;
	if (c[1]<0) c[1]=0;
	if (c[2]<0) c[2]=0;

	const bool withExtinction=(core->getSkyDrawer()->getFlagHasAtmosphere() && core->getSkyDrawer()->getExtinction().getExtinctionCoefficient()>=0.01f);

	if (withExtinction)
	{
		// GZ: We must process the rotated vertices to find elevation.
		QVector<Vec3d> rotatedMilkywayVertices(vertexArray->vertex);


		// TODO: Rotate a copy of the milky way vertices so that vertex coordinates reflect azimuthal positions.
		// We can use this fast version. The difference causes by refraction in this case is negligible.
		// core->j2000ToAltAzInPlaceNoRefraction(transfo-> * rotatedMilkywayVertices.data());
		// Alternative: something along the lines of:
		// prj->forward(rotatedMilkywayVertices); // ???

		// mag vector is used for fast extinction. We set the vertex color array afterwards.
//		QVector<float> extinctionMag;
//		extinctionMag.resize(rotatedMilkywayVertices.length());
//		extinctionMag.fill(0.0f);
//		core->getSkyDrawer()->getExtinction().forward(&(rotatedMilkywayVertices.constData()), extinctionMag.data());
//		// Now we have an array of mag difference values for each vertex of the Milky Way vertexArray.
//		// The globally derived color must be modulated by the extinction magnitude and brought to every vertex color.

//		//QVector<float>::const_iterator magIter;
//		vertexArray->colors.clear();
//		//for (magIter=extinctionMag.begin(); magIter != extinctionMag.end(); ++magIter)
//		float oneMag;
//		foreach(oneMag, extinctionMag)
//		{
//			// compute a simple factor from magnitude loss.
//			//float extinctionFactor=std::pow(0.4f , magIter.value()); // drop of one magnitude: factor 2.5 or 40%
//			float extinctionFactor=std::pow(0.4f , oneMag); // drop of one magnitude: factor 2.5 or 40%
//			Vec3f thisColor=Vec3f(c[0]*extinctionFactor, c[1]*extinctionFactor, c[2]*extinctionFactor);
//			vertexArray->colors.append(thisColor);
//		}
		Extinction extinction=core->getSkyDrawer()->getExtinction();
		vertexArray->colors.clear();
		QVector<Vec3d>::const_iterator iter;
		for (iter=rotatedMilkywayVertices.begin(); iter !=rotatedMilkywayVertices.end(); ++iter)
		{
			float oneMag=0.0f;
			Vec3d tmp(iter->v[0], iter->v[1], iter->v[2]);
			extinction.forward(tmp, &oneMag);
			if (debugOne)
				qDebug() << "Vector: " << iter->v[0] << "/" << iter->v[1] << "/" << iter->v[2] << ":" << oneMag;
			float extinctionFactor=std::pow(0.4f , oneMag); // drop of one magnitude: factor 2.5 or 40%
			Vec3f thisColor=Vec3f(3.0f*c[0], c[1]*8.0f*extinctionFactor, c[2]*extinctionFactor);
			vertexArray->colors.append(thisColor);

		}
		debugOne=false; // stop log dump after one frame.
	}
	else
		vertexArray->colors.fill(Vec3f(c[0], c[1], c[2]));
	StelPainter sPainter(prj);
	//sPainter.setColor(c[0],c[1],c[2]);
	glEnable(GL_CULL_FACE);
	sPainter.enableTexture2d(true);
	glDisable(GL_BLEND);
	tex->bind();
	sPainter.drawStelVertexArray(*vertexArray);
	glDisable(GL_CULL_FACE);
}
