/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
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

#include <algorithm>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QFontMetrics>

#include "StelProjector.hpp"
#include "Constellation.hpp"
#include "StarMgr.hpp"

#include "renderer/StelCircleArcRenderer.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"
#include "renderer/StelIndexBuffer.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

Vec3f Constellation::lineColor = Vec3f(0.4,0.4,0.8);
Vec3f Constellation::labelColor = Vec3f(0.4,0.4,0.8);
Vec3f Constellation::boundaryColor = Vec3f(0.8,0.3,0.3);
bool Constellation::singleSelected = false;

Constellation::Constellation() 
	: asterism(NULL)
	, artTexture(NULL)
	, artVertices(NULL)
	, artIndices(NULL)
{
}

Constellation::~Constellation()
{
	if (NULL != asterism)    {delete[] asterism;}
	if (NULL != artTexture)  {delete artTexture;}
	if (NULL != artVertices) {delete artVertices;}
	if (NULL != artIndices)  {delete artIndices;}
	asterism    = NULL;
	artTexture  = NULL;
	artVertices = NULL;
	artIndices  = NULL;
}

void Constellation::generateArtVertices(StelRenderer* renderer, const int resolution)
{
	// Texture not yet loaded.
	if (NULL == artTexture || !artTexture->getStatus() == TextureStatus_Loaded)
	{
		// artVertices will be still NULL, and it will be generated in a future 
		// call once the texture is loaded
		return;
	}

	const QSize texSize = artTexture->getDimensions();

	artVertices = renderer->createVertexBuffer<Vertex>(PrimitiveType_Triangles);
	artIndices  = renderer->createIndexBuffer(IndexType_U16);

	const float mult = 1.0f / resolution;

	// Create the vertex grid.
	for (int y = 0; y <= resolution; ++y)
	{
		for (int x = 0; x <= resolution; ++x)
		{
			const float texX = x * mult;
			const float texY = y * mult;
			artVertices->addVertex
				(Vertex(texCoordTo3D * Vec3f(texX * texSize.width(), texY * texSize.height(), 0.0f),
				        Vec2f(texX, texY)));
		}
	}
	// Use indices to form a triangle pair for every cell of the grid.
	for (int y = 0; y < resolution; ++y)
	{
		for (int x = 0; x < resolution; ++x)
		{
			const uint sw = x       * (resolution + 1) + y;
			const uint se = (x + 1) * (resolution + 1) + y;
			const uint nw = x       * (resolution + 1) + y + 1;
			const uint ne = (x + 1) * (resolution + 1) + y + 1;
			artIndices->addIndex(nw);
			artIndices->addIndex(se);
			artIndices->addIndex(sw);
			artIndices->addIndex(nw);
			artIndices->addIndex(ne);
			artIndices->addIndex(se);
		}
	}

	artVertices->lock();
	artIndices->lock();
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
		XYZname+= asterism[ii]->getJ2000EquatorialPos(StelApp::getInstance().getCore());
	}
	XYZname.normalize();

	return true;
}

void Constellation::drawOptim(StelRenderer* renderer, StelProjectorP projector, const StelCore* core, const SphericalCap& viewportHalfspace) const
{
	// Avoid drawing when not visible
	if (lineFader.getInterstate() <= 0.001f)
	{
		return;
	}

	renderer->setGlobalColor(lineColor[0], lineColor[1], 
	                         lineColor[2], lineFader.getInterstate());

	Vec3d star1;
	Vec3d star2;
	for (unsigned int i = 0; i < numberOfSegments; ++i)
	{
		star1 = asterism[2 * i]->getJ2000EquatorialPos(core);
		star2 = asterism[2 * i + 1]->getJ2000EquatorialPos(core);
		star1.normalize();
		star2.normalize();
		StelCircleArcRenderer(renderer, projector)
			.drawGreatCircleArc(star1, star2, &viewportHalfspace);
	}
}

void Constellation::drawName(StelRenderer* renderer, QFont& font) const
{
	if (nameFader.getInterstate() <= 0.001f)
	{
		return;
	}

	renderer->setFont(font);
	renderer->setGlobalColor(labelColor[0], labelColor[1], 
	                         labelColor[2], nameFader.getInterstate());
	renderer->drawText(TextParams(XYname[0], XYname[1], nameI18)
	                   .shift(-QFontMetrics(font).width(nameI18) * 0.5, 0.0f)
	                   .useGravity());
}

void Constellation::drawArtOptim
	(StelRenderer* renderer, StelProjectorP projector, const SphericalRegion& region) const
{
	const float intensity = artFader.getInterstate();
	// Art polygon not yet generated (only generated once the texture is loaded)
	if (NULL == artVertices) {return;}
	Q_ASSERT_X(NULL != artIndices, Q_FUNC_INFO, 
	           "Vertex buffer was generated but index buffer was not");

	renderer->setCulledFaces(CullFace_Back);
	// Don't draw if outside viewport.
	if (intensity > 0.001f && region.intersects(boundingCap))
	{
		artTexture->bind();
		if (StelApp::getInstance().getVisionModeNight())
			renderer->setGlobalColor(intensity, 0.0, 0.0);
		else
			renderer->setGlobalColor(intensity,intensity,intensity);
		renderer->drawVertexBuffer(artVertices, artIndices, projector);
	}
	renderer->setCulledFaces(CullFace_None);
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

void Constellation::drawBoundaryOptim(StelRenderer* renderer, StelProjectorP projector) const
{
	if (boundaryFader.getInterstate() < 0.001)
	{
		return;
	}

	renderer->setBlendMode(BlendMode_Alpha);
	renderer->setGlobalColor(boundaryColor[0], boundaryColor[1], 
	                         boundaryColor[2], boundaryFader.getInterstate());

	int size = singleSelected ? isolatedBoundarySegments.size() 
	                          : sharedBoundarySegments.size();

	const SphericalCap& viewportHalfspace = projector->getBoundingCap();

	for (int i = 0; i < size; i++)
	{
		std::vector<Vec3f>* points = singleSelected ? isolatedBoundarySegments[i] 
		                                            : sharedBoundarySegments[i];

		for (int j = 0; j < static_cast<int>(points->size()) - 1; j++)
		{
			const Vec3f pt1 = points->at(j) ;
			const Vec3f pt2 = points->at(j +1);
			if (pt1 * pt2 > 0.9999999f)
			{
				continue;
			}
			const Vec3d ptd1(pt1[0], pt1[1], pt1[2]);
			const Vec3d ptd2(pt2[0], pt2[1], pt2[2]);
			StelCircleArcRenderer(renderer, projector)
				.drawGreatCircleArc(ptd1, ptd2, &viewportHalfspace);
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
		const float Mag = asterism[i]->getVMagnitude(0, false);
		if (Mag < maxMag)
		{
			brightest = asterism[i];
			maxMag = Mag;
		}
	}
	return brightest;
}
