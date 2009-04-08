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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <algorithm>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include "StelProjector.hpp"
#include "Constellation.hpp"
#include "StarMgr.hpp"
#include "StelNavigator.hpp"
#include "StelTexture.hpp"
#include "StelFont.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

Vec3f Constellation::lineColor = Vec3f(0.4,0.4,0.8);
Vec3f Constellation::labelColor = Vec3f(0.4,0.4,0.8);
Vec3f Constellation::boundaryColor = Vec3f(0.8,0.3,0.3);
bool Constellation::singleSelected = false;

Constellation::Constellation() : asterism(NULL)
{
}

Constellation::~Constellation()
{   
	if (asterism) delete[] asterism;
	asterism = NULL;
}

bool Constellation::read(const QString& record, StarMgr *starMgr)
{   
	unsigned int HP;

	abbreviation.clear();
	numberOfSegments = 0;

	QString buf(record);
	QTextStream istr(&buf, QIODevice::ReadOnly);
	QString abb;
	istr >> abb >> numberOfSegments;
	if (istr.status()!=QTextStream::Ok) 
		return false;

	abbreviation = abb.toUpper();

	asterism = new StelObjectP[numberOfSegments*2];
	for (unsigned int i=0;i<numberOfSegments*2;++i)
	{
		HP = 0;
		istr >> HP;
		if(HP == 0)
		{
			// TODO: why is this delete commented?
			// delete[] asterism;
			return false;
		}

		asterism[i]=starMgr->searchHP(HP);
		if (!asterism[i])
		{
			qWarning() << "Error in Constellation " << abbreviation << " asterism : can't find star HP= " << HP;
			// TODO: why is this delete commented?
			// delete[] asterism;
			return false;
		}
	}

	XYZname.set(0.,0.,0.);
	for(unsigned int ii=0;ii<numberOfSegments*2;++ii)
	{
		XYZname+= asterism[ii]->getJ2000EquatorialPos(StelApp::getInstance().getCore()->getNavigator());
	}
	XYZname*=1./(numberOfSegments*2);

	return true;
}

void Constellation::drawOptim(const StelPainter& sPainter, const StelNavigator* nav) const
{
	if(!lineFader.getInterstate()) return;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

	glColor4f(lineColor[0], lineColor[1], lineColor[2], lineFader.getInterstate());
	
	Vec3d star1;
	Vec3d star2;
	Vec3d dummy;
	Vec3d v;
	const HalfSpace viewportHalfspace = sPainter.getProjector()->getBoundingHalfSpace();
	for (unsigned int i=0;i<numberOfSegments;++i)
	{
		star1=asterism[2*i]->getJ2000EquatorialPos(nav);
		star2=asterism[2*i+1]->getJ2000EquatorialPos(nav);
		v = star1^star2;
		v.normalize();
		if (viewportHalfspace.d<0. || planeIntersect2(viewportHalfspace, HalfSpace(v), dummy, dummy)!=false)
		{
			star1.normalize();
			star2.normalize();
			if (viewportHalfspace.contains(star1) || viewportHalfspace.contains(star2))
				sPainter.drawSmallCircleArc(star1, star2, Vec3d(0));
		}
	}
}

void Constellation::drawName(StelFont *constfont, const StelPainter& sPainter) const
{
	if(!nameFader.getInterstate())
		return;
	glColor4f(labelColor[0], labelColor[1], labelColor[2], nameFader.getInterstate());
	sPainter.drawText(constfont, XYname[0], XYname[1], nameI18, 0., -constfont->getStrLen(nameI18)/2, 0, false);
}

void Constellation::drawArtOptim(const StelPainter& sPainter, const SphericalRegion& region) const
{
	float intensity = artFader.getInterstate(); 
	if (artTexture && intensity && region.intersects(artPolygon)) 
	{
		glColor3f(intensity,intensity,intensity);

		// The texture is not fully loaded
		if (artTexture->bind()==false)
			return;
		
		sPainter.drawSphericalPolygon(&artPolygon, StelPainter::SphericalPolygonDrawModeTextureFill);
	}
}

// Draw the art texture
void Constellation::drawArt(const StelPainter& sPainter) const
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	SphericalRegionP region = sPainter.getProjector()->getViewportConvexPolygon();
	drawArtOptim(sPainter, *region);

	glDisable(GL_CULL_FACE);
}

const Constellation* Constellation::isStarIn(const StelObject* s) const
{
	for(unsigned int i=0;i<numberOfSegments*2;++i)
	{

		// asterism[i]==s test was not working
		if (asterism[i]->getEnglishName()==s->getEnglishName())
		{
			// qDebug() << "Const matched. " << getEnglishName();
			return this;
		}
	}
	return NULL;
}

void Constellation::update(int deltaTime)
{
	lineFader.update(deltaTime);
	nameFader.update(deltaTime);
	artFader.update(deltaTime);
	boundaryFader.update(deltaTime);
}

void Constellation::drawBoundaryOptim(const StelProjectorP& prj) const
{
	if(!boundaryFader.getInterstate()) return;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

	glColor4f(boundaryColor[0], boundaryColor[1], boundaryColor[2], boundaryFader.getInterstate());

	unsigned int i, j, size;
	Vec3d pt1, pt2;
	std::vector<Vec3f> *points;

	if (singleSelected) size = isolatedBoundarySegments.size();
	else size = sharedBoundarySegments.size();
	
	for (i=0;i<size;i++)
	{
		if (singleSelected) points = isolatedBoundarySegments[i];
		else points = sharedBoundarySegments[i];

		for (j=0;j<points->size()-1;j++)
		{
			if(prj->projectLineCheck(points->at(j),pt1,points->at(j+1),pt2)) 
			{
				glBegin(GL_LINES);
					glVertex2f(pt1[0],pt1[1]);
					glVertex2f(pt2[0],pt2[1]);
				glEnd();
			}
		}			
	}
}

StelObjectP Constellation::getBrightestStarInConstellation(void) const
{
	float maxMag = 99.f;
	StelObjectP brightest;
	// maybe the brightest star has always odd index,
	// so check all segment endpoints:
	for (int i=2*numberOfSegments-1;i>=0;i--)
	{
		const float Mag = asterism[i]->getVMagnitude(0);
		if (Mag < maxMag)
		{
			brightest = asterism[i];
			maxMag = Mag;
		}
	}
	return brightest;
}
