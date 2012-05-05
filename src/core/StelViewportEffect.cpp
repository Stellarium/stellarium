/*
 * Stellarium
 * Copyright (C) 2010 Fabien Chereau
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

#include "StelViewportEffect.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "SphericMirrorCalculator.hpp"
#include "StelFileMgr.hpp"
#include "StelMovementMgr.hpp"

#include <QGLFramebufferObject>
#include <QSettings>
#include <QFile>

void StelViewportEffect::paintViewportBuffer(const QGLFramebufferObject* buf) const
{
	StelPainter sPainter(StelApp::getInstance().getCore()->getProjection2d());
	sPainter.setColor(1,1,1);
	sPainter.enableTexture2d(true);
	glBindTexture(GL_TEXTURE_2D, buf->texture());
	sPainter.drawRect2d(0, 0, buf->size().width(), buf->size().height());
}

struct VertexPoint
{
	Vec2f ver_xy;
	Vec4f color;
	double h;
};

StelViewportDistorterFisheyeToSphericMirror::StelViewportDistorterFisheyeToSphericMirror
	(int screenWidth,int screenHeight) 
	: screenWidth(screenWidth)
	, screenHeight(screenHeight)
	, originalProjectorParams(StelApp::getInstance().getCore()->
	                          getCurrentStelProjectorParams())
	, texturePointGrid(NULL)
{
	QSettings& conf = *StelApp::getInstance().getSettings();
	StelCore* core = StelApp::getInstance().getCore();

	// initialize viewport parameters and texture size:

	// maximum FOV value of the not yet distorted image
	double distorterMaxFOV = conf.value("spheric_mirror/distorter_max_fov",175.f).toFloat();
	if (distorterMaxFOV > 240.f)
		distorterMaxFOV = 240.f;
	else if (distorterMaxFOV < 120.f)
		distorterMaxFOV = 120.f;
	if (distorterMaxFOV > core->getMovementMgr()->getMaxFov())
		distorterMaxFOV = core->getMovementMgr()->getMaxFov();

	StelProjectorP proj = core->getProjection(StelCore::FrameJ2000);
	core->getMovementMgr()->setMaxFov(distorterMaxFOV);

	// width of the not yet distorted image
	newProjectorParams.viewportXywh[2] =
	    conf.value("spheric_mirror/newProjectorParams.viewportXywh[2]idth",
	               originalProjectorParams.viewportXywh[2]).toInt();
	if (newProjectorParams.viewportXywh[2] <= 0)
	{
		newProjectorParams.viewportXywh[2] = originalProjectorParams.viewportXywh[2];
	}
	else if (newProjectorParams.viewportXywh[2] > screenWidth)
	{
		newProjectorParams.viewportXywh[2] = screenWidth;
	}

	// height of the not yet distorted image
	newProjectorParams.viewportXywh[3] =
	    conf.value("spheric_mirror/newProjectorParams.viewportXywh[3]eight",
	    originalProjectorParams.viewportXywh[3]).toInt();
	if (newProjectorParams.viewportXywh[3] <= 0)
	{
		newProjectorParams.viewportXywh[3] = originalProjectorParams.viewportXywh[3];
	}
	else if (newProjectorParams.viewportXywh[3] > screenHeight)
	{
		newProjectorParams.viewportXywh[3] = screenHeight;
	}

	// center of the FOV-disk in the not yet distorted image
	newProjectorParams.viewportCenter[0] = 
	    conf.value("spheric_mirror/viewportCenterX",
	               0.5*newProjectorParams.viewportXywh[2]).toFloat();
	newProjectorParams.viewportCenter[1] = 
	    conf.value("spheric_mirror/viewportCenterY",
	               0.5*newProjectorParams.viewportXywh[3]).toFloat();

	// diameter of the FOV-disk in pixels
	newProjectorParams.viewportFovDiameter = 
	    conf.value("spheric_mirror/viewport_fov_diameter",
	               qMin(newProjectorParams.viewportXywh[2],
	                    newProjectorParams.viewportXywh[3])).toFloat();

	texture_wh = 1;
	while (texture_wh < newProjectorParams.viewportXywh[2] || 
	       texture_wh < newProjectorParams.viewportXywh[3])
	{
		texture_wh <<= 1;
	}
	
	viewportTextureOffset[0] = (texture_wh-newProjectorParams.viewportXywh[2])>>1;
	viewportTextureOffset[1] = (texture_wh-newProjectorParams.viewportXywh[3])>>1;

	newProjectorParams.viewportXywh[0] = (screenWidth-newProjectorParams.viewportXywh[2]) >> 1;
	newProjectorParams.viewportXywh[1] = (screenHeight-newProjectorParams.viewportXywh[3]) >> 1;

	StelApp::getInstance().getCore()->setCurrentStelProjectorParams(newProjectorParams);

	const QString customDistortionFileName = 
	    conf.value("spheric_mirror/custom_distortion_file","").toString();
	
	if (customDistortionFileName.isEmpty())
	{
		generateDistortion(conf, proj, distorterMaxFOV);
	}
	else if (!loadDistortionFromFile(customDistortionFileName))
	{
		qDebug() << "Falling back to generated distortion";
		generateDistortion(conf, proj, distorterMaxFOV);
	}
}


StelViewportDistorterFisheyeToSphericMirror::~StelViewportDistorterFisheyeToSphericMirror(void)
{
	if (texturePointGrid)
		delete[] texturePointGrid;

	// TODO repair
	// prj->setMaxFov(original_max_fov);
	//	prj->setViewport(original_viewport[0],original_viewport[1],
	// 	                 original_viewport[2],original_viewport[3],
	// 	                 original_viewportCenter[0],original_viewportCenter[1],
	// 	                 original_viewportFovDiameter);
}


void StelViewportDistorterFisheyeToSphericMirror::generateDistortion
	(const QSettings& conf, const StelProjectorP& proj, const double distorterMaxFOV)
{
	// Load generation parameters.
	float textureTriangleBaseLength =
	    conf.value("spheric_mirror/texture_triangle_base_length",16.f).toFloat();
	if (textureTriangleBaseLength > 256.f)
	{
		qDebug() << "spheric_mirror/projector_gamma too high : setting to 256.0";
		textureTriangleBaseLength = 256.f;
	}
	else if (textureTriangleBaseLength < 2.f)
	{
		qDebug() << "spheric_mirror/projector_gamma too low : setting to 2.0";
		textureTriangleBaseLength = 2.f;
	}
	maxGridX = (int)trunc(0.5 + screenWidth / textureTriangleBaseLength);
	stepX = screenWidth / (double)(maxGridX - 0.5);
	maxGridY = (int)trunc(screenHeight / (textureTriangleBaseLength * 0.5 * sqrt(3.0)));
	stepY = screenHeight / (double)maxGridY;

	double gamma = conf.value("spheric_mirror/projector_gamma",0.45).toDouble();
	if (gamma < 0.0)
	{
		qDebug() << "Negative spheric_mirror/projector_gamma : setting to zero.";
		gamma = 0.0;
	}

	const int cols = maxGridX + 1;
	const int rows = maxGridY + 1;
	
	const float viewScalingFactor = 0.5 * newProjectorParams.viewportFovDiameter /
	                                proj->fovToViewScalingFactor(distorterMaxFOV*(M_PI/360.0));
	texturePointGrid = new Vec2f[cols*rows];
	VertexPoint *const vertexGrid = new VertexPoint[cols*rows];
	double maxH = 0;
	SphericMirrorCalculator calc(conf);
	
	// Generate grid vertices/texcoords.
	for (int row = 0; row <= maxGridY; row++)
	{
		for (int col = 0; col <= maxGridX; col++)
		{
			VertexPoint &vertexPoint(vertexGrid[row * cols + col]);
			Vec2f &texturePoint(texturePointGrid[row * cols + col]);
			
			// Clamp to screen extents.
			vertexPoint.ver_xy[0] = ((col == 0)        ? 0.f : 
			                         (col == maxGridX) ? screenWidth : 
			                                             (col - 0.5f * (row & 1)) * stepX);
			vertexPoint.ver_xy[1] = row * stepY;
			Vec3f v,vX,vY;
			bool rc = calc.retransform((vertexPoint.ver_xy[0]-0.5f*screenWidth) / screenHeight,
			                           (vertexPoint.ver_xy[1]-0.5f*screenHeight) / screenHeight,
			                           v,vX,vY);

			rc &= proj->forward(v);
			const float x = newProjectorParams.viewportCenter[0] + v[0] * viewScalingFactor;
			const float y = newProjectorParams.viewportCenter[1] + v[1] * viewScalingFactor;
			vertexPoint.h = rc ? (vX^vY).length() : 0.0;

			// sharp image up to the border of the fisheye image, at the cost of
			// accepting clamping artefacts. You can get rid of the clamping
			// artefacts by specifying a viewport size a little less then
			// (1<<n)*(1<<n), for instance 1022*1022. With a viewport size
			// of 512*512 and viewportFovDiameter=512 you will get clamping
			// artefacts in the 3 otherwise black hills on the bottom of the image.

			//      if (x < 0.f) {x=0.f;vertexPoint.h=0;}
			//      else if (x > newProjectorParams.viewportXywh[2])
			//          {x=newProjectorParams.viewportXywh[2];vertexPoint.h=0;}
			//      if (y < 0.f) {y=0.f;vertexPoint.h=0;}
			//      else if (y > newProjectorParams.viewportXywh[3])
			//          {y=newProjectorParams.viewportXywh[3];vertexPoint.h=0;}

			texturePoint[0] = (viewportTextureOffset[0]+x)/texture_wh;
			texturePoint[1] = (viewportTextureOffset[1]+y)/texture_wh;

			if (vertexPoint.h > maxH) maxH = vertexPoint.h;
		}
	}
	
	// Generate grid colors.
	for (int row = 0; row <= maxGridY; row++)
	{
		for (int col = 0; col <= maxGridX; col++)
		{
			VertexPoint &vertexPoint(vertexGrid[(row * (maxGridX + 1) + col)]);
			Vec4f &color(vertexPoint.color);
			color[0] = color[1] = color[2] = (vertexPoint.h <= 0.0) 
			                               ? 0.0 
			                               : exp(gamma * log(vertexPoint.h / maxH));
			color[3] = 1.0f;
		}
	}

	constructVertexBuffer(vertexGrid);
	
	delete[] vertexGrid;
}

bool StelViewportDistorterFisheyeToSphericMirror::loadDistortionFromFile
	(const QString& fileName)
{
	// Open file.
	QFile file;
	QTextStream in;
	try
	{
		file.setFileName(StelFileMgr::findFile(fileName));
		file.open(QIODevice::ReadOnly);
		if (file.error() != QFile::NoError)
			throw("failed to open file");
		in.setDevice(&file);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: could not open custom_distortion_file:" << fileName << e.what();
		return false;
	}
	Q_ASSERT(file.error() != QFile::NoError);
	
	in >> maxGridX >> maxGridY;
	Q_ASSERT(in.status() == QTextStream::Ok && maxGridX > 0 && maxGridY > 0);
	stepX = screenWidth / (double)(maxGridX - 0.5);
	stepY = screenHeight / (double)maxGridY;
	
	const int cols = maxGridX + 1;
	const int rows = maxGridY + 1;
	texturePointGrid = new Vec2f[cols * rows];
	
	// Load the grid.
	VertexPoint *const vertexGrid = new VertexPoint[cols *rows];
	for (int row = 0; row <= maxGridY; row++)
	{
		for (int col = 0; col <= maxGridX; col++)
		{
			VertexPoint &vertexPoint(vertexGrid[(row * cols + col)]);
			Vec2f &texturePoint(texturePointGrid[(row * cols + col)]);
			// Clamp to screen extents.
			vertexPoint.ver_xy[0] = ((col == 0)        ? 0.f : 
			                         (col == maxGridX) ? screenWidth :
			                                            (col - 0.5f * (row & 1)) * stepX);
			vertexPoint.ver_xy[1] = row * stepY;
			float x, y;
			in >> x >> y 
			   >> vertexPoint.color[0] 
			   >> vertexPoint.color[1] 
			   >> vertexPoint.color[2];
			vertexPoint.color[3] = 1.0f;
			Q_ASSERT(in.status() != QTextStream::Ok);
			texturePoint[0] = (viewportTextureOffset[0] + x) / texture_wh;
			texturePoint[1] = (viewportTextureOffset[1] + y) / texture_wh;
		}
	}
	
	constructVertexBuffer(vertexGrid);
	
	delete[] vertexGrid;
	
	return true;
}

void StelViewportDistorterFisheyeToSphericMirror::constructVertexBuffer
	(const VertexPoint *const vertexGrid)
{
	const int cols = maxGridX + 1;
	
	// Each row is a triangle strip.
	for (int row = 0; row < maxGridY; row++)
	{
		// Two rows of vertices make up one row of the grid.
		const Vec2f *t0 = texturePointGrid + row * cols;
		const Vec2f *t1 = t0;
		const VertexPoint *v0 = vertexGrid + row * cols;
		const VertexPoint *v1 = v0;
		
		// Alternating between the "first" and the "second" vertex row.
		if (row & 1)
		{
			t1 += cols;
			v1 += cols;
		}
		else
		{
			t0 += cols;
			v0 += cols;
		}
		
		for (int col = 0; col < cols; col++,t0++,t1++,v0++,v1++)
		{
			displayColorList    << v0->color  << v1->color;
			displayTexCoordList << *t0        << *t1;
			displayVertexList   << v0->ver_xy << v1->ver_xy;
		}
	}
}



void StelViewportDistorterFisheyeToSphericMirror::distortXY(float& x, float& y) const
{
	float texture_x,texture_y;

	// find the triangle and interpolate accordingly:
	float dy = y / stepY;
	const int j = (int)floorf(dy);
	dy -= j;
	if (j&1)
	{
		float dx = x / stepX + 0.5f*(1.f-dy);
		const int i = (int)floorf(dx);
		dx -= i;
		const Vec2f *const t = texturePointGrid + (j*(maxGridX+1)+i);
		if (dx + dy <= 1.f)
		{
			if (i == 0)
			{
				dx -= 0.5f*(1.f-dy);
				dx *= 2.f;
			}
			texture_x = t[0][0] + dx * (t[1][0]-t[0][0]) + dy * (t[maxGridX+1][0]-t[0][0]);
			texture_y = t[0][1] + dx * (t[1][1]-t[0][1]) + dy * (t[maxGridX+1][1]-t[0][1]);
		}
		else
		{
			if (i == maxGridX-1)
			{
				dx -= 0.5f*(1.f-dy);
				dx *= 2.f;
			}
			texture_x = t[maxGridX+2][0] + (1.f-dy) * (t[1][0]-t[maxGridX+2][0]) + (1.f-dx) *
			                                          (t[maxGridX+1][0]-t[maxGridX+2][0]);
			texture_y = t[maxGridX+2][1] + (1.f-dy) * (t[1][1]-t[maxGridX+2][1]) + (1.f-dx) *
			                                          (t[maxGridX+1][1]-t[maxGridX+2][1]);
		}
	}
	else
	{
		float dx = x / stepX + 0.5f*dy;
		const int i = (int)floorf(dx);
		dx -= i;
		const Vec2f *const t = texturePointGrid + (j*(maxGridX+1)+i);
		if (dx >= dy)
		{
			if (i == maxGridX-1)
			{
				dx -= 0.5f*dy;
				dx *= 2.f;
			}
			texture_x = t[1][0] + (1.f-dx) * (t[0][0]-t[1][0]) + dy *
			                                 (t[maxGridX+2][0]-t[1][0]);
			texture_y = t[1][1] + (1.f-dx) * (t[0][1]-t[1][1]) + dy *
			                                 (t[maxGridX+2][1]-t[1][1]);
		}
		else
		{
			if (i == 0)
			{
				dx -= 0.5f*dy;
				dx *= 2.f;
			}
			texture_x = t[maxGridX+1][0] + (1.f-dy) * (t[0][0]-t[maxGridX+1][0]) + dx *
			                                          (t[maxGridX+2][0]-t[maxGridX+1][0]);
			texture_y = t[maxGridX+1][1] + (1.f-dy) * (t[0][1]-t[maxGridX+1][1]) + dx *
			                                          (t[maxGridX+2][1]-t[maxGridX+1][1]);
		}
	}

	x = texture_wh*texture_x - viewportTextureOffset[0] + newProjectorParams.viewportXywh[0];
	y = texture_wh*texture_y - viewportTextureOffset[1] + newProjectorParams.viewportXywh[1];
}


void StelViewportDistorterFisheyeToSphericMirror::paintViewportBuffer
    (const QGLFramebufferObject* buf) const
{
	StelPainter sPainter(StelApp::getInstance().getCore()->getProjection2d());
	sPainter.enableTexture2d(true);
	glBindTexture(GL_TEXTURE_2D, buf->texture());
	glDisable(GL_BLEND);

	sPainter.enableClientStates(true, true, true);
	sPainter.setColorPointer(4, GL_FLOAT, displayColorList.constData());
	sPainter.setVertexPointer(2, GL_FLOAT, displayVertexList.constData());
	sPainter.setTexCoordPointer(2, GL_FLOAT, displayTexCoordList.constData());
	for (int row = 0; row < maxGridY; row++)
	{
		sPainter.drawFromArray(StelPainter::TriangleStrip, 
		                       (maxGridX + 1) * 2, row * (maxGridX + 1) * 2,
		                       false);
	}
	sPainter.enableClientStates(false);
}

