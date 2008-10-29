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
#include "Projector.hpp"
#include "Constellation.hpp"
#include "StarMgr.hpp"
#include "Navigator.hpp"
#include "STexture.hpp"
#include "SFont.hpp"

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

	for(unsigned int ii=0;ii<numberOfSegments*2;++ii)
	{
		XYZname+= asterism[ii]->getObsJ2000Pos(0);
	}
	XYZname*=1./(numberOfSegments*2);

	return true;
}

void Constellation::drawOptim(Projector* prj) const
{
	if(!lineFader.getInterstate()) return;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

	glColor4f(lineColor[0], lineColor[1], lineColor[2], lineFader.getInterstate());
	
	Vec3d star1;
	Vec3d star2;
	for (unsigned int i=0;i<numberOfSegments;++i)
	{
		if (prj->projectLineCheck(asterism[2*i]->getObsJ2000Pos(0),star1,asterism[2*i+1]->getObsJ2000Pos(0),star2))
		{
			glBegin(GL_LINES);
			glVertex2f(star1[0],star1[1]);
			glVertex2f(star2[0],star2[1]);
			glEnd();
		}
	}
}

void Constellation::drawName(SFont *constfont, Projector* prj) const
{
	if(!nameFader.getInterstate()) return;
	glColor4f(labelColor[0], labelColor[1], labelColor[2], nameFader.getInterstate());
	prj->drawText(constfont, XYname[0], XYname[1], nameI18, 
	              0., -constfont->getStrLen(nameI18)/2, 0, false);
}

void Constellation::drawArtOptim(Projector* prj, const Navigator* nav) const
{
	float intensity = artFader.getInterstate(); 
	if (artTexture && intensity) 
	{
		glColor3f(intensity,intensity,intensity);

		Vec3d v0, v1, v2, v3, v4, v5, v6, v7, v8;
		bool b0, b1, b2, b3, b4, b5, b6, b7, b8; 

		// If one of the point is in the screen
		b0 = prj->projectCheck(artVertex[0],v0) || (nav->getJ2000EquVisionDirection().dot(artVertex[0])>0.9);
		b1 = prj->projectCheck(artVertex[1],v1) || (nav->getJ2000EquVisionDirection().dot(artVertex[1])>0.9);
		b2 = prj->projectCheck(artVertex[2],v2) || (nav->getJ2000EquVisionDirection().dot(artVertex[2])>0.9);
		b3 = prj->projectCheck(artVertex[3],v3) || (nav->getJ2000EquVisionDirection().dot(artVertex[3])>0.9);
		b4 = prj->projectCheck(artVertex[4],v4) || (nav->getJ2000EquVisionDirection().dot(artVertex[4])>0.9);
		b5 = prj->projectCheck(artVertex[5],v5) || (nav->getJ2000EquVisionDirection().dot(artVertex[5])>0.9);
		b6 = prj->projectCheck(artVertex[6],v6) || (nav->getJ2000EquVisionDirection().dot(artVertex[6])>0.9);
		b7 = prj->projectCheck(artVertex[7],v7) || (nav->getJ2000EquVisionDirection().dot(artVertex[7])>0.9);
		b8 = prj->projectCheck(artVertex[8],v8) || (nav->getJ2000EquVisionDirection().dot(artVertex[8])>0.9);
			
		if (b0 || b1 || b2 || b3 || b4 || b5 || b6 || b7 || b8)
		{
			// The texture is not fully loaded
			if (artTexture->bind()==false)
				return;
		
			if ((b0 || b1 || b2 || b3) && (v0[2]<1 && v1[2]<1 && v2[2]<1 && v3[2]<1))
			{	
				glBegin(GL_QUADS);
					glTexCoord2f(0,0); 		glVertex2f(v0[0],v0[1]);
					glTexCoord2f(0.5,0); 	glVertex2f(v1[0],v1[1]);
					glTexCoord2f(0.5,0.5); 	glVertex2f(v2[0],v2[1]);
					glTexCoord2f(0,0.5); 	glVertex2f(v3[0],v3[1]);
				glEnd();
			}
			if ((b1 || b4 || b5 || b2) && (v1[2]<1 && v4[2]<1 && v5[2]<1 && v2[2]<1))
			{
				glBegin(GL_QUADS);
					glTexCoord2f(0.5,0); glVertex2f(v1[0],v1[1]);
					glTexCoord2f(1,0); glVertex2f(v4[0],v4[1]);
					glTexCoord2f(1,0.5); glVertex2f(v5[0],v5[1]);
					glTexCoord2f(0.5,0.5); glVertex2f(v2[0],v2[1]);
				glEnd();
			}
			if ((b2 || b5 || b6 || b7) && (v2[2]<1 && v5[2]<1 && v6[2]<1 && v7[2]<1))
			{
				glBegin(GL_QUADS);
					glTexCoord2f(0.5,0.5); 	glVertex2f(v2[0],v2[1]);
					glTexCoord2f(1,0.5); 	glVertex2f(v5[0],v5[1]);
					glTexCoord2f(1,1); 		glVertex2f(v6[0],v6[1]);
					glTexCoord2f(0.5,1);	glVertex2f(v7[0],v7[1]);
				glEnd();
			}
			if ((b3 || b2 || b7 || b8) && (v3[2]<1 && v2[2]<1 && v7[2]<1 && v8[2]<1))
			{
				glBegin(GL_QUADS);
					glTexCoord2f(0,0.5); 	glVertex2f(v3[0],v3[1]);
					glTexCoord2f(0.5,0.5); 	glVertex2f(v2[0],v2[1]);
					glTexCoord2f(0.5,1); 	glVertex2f(v7[0],v7[1]);
					glTexCoord2f(0,1);		glVertex2f(v8[0],v8[1]);
				glEnd();
			}
		}
	}
}

// Draw the art texture
void Constellation::drawArt(Projector* prj, const Navigator* nav) const
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	drawArtOptim(prj, nav);

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

void Constellation::drawBoundaryOptim(Projector* prj) const
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
