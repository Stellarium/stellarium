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

#ifndef _STELVIEWPORTEFFECT_HPP_
#define _STELVIEWPORTEFFECT_HPP_

#include "VecMath.hpp"
#include "StelProjector.hpp"

class QGLFramebufferObject;

//! @class StelViewportEffect
//! Allow to apply visual effects on the whole Stellarium viewport.
class StelViewportEffect
{
public:
	StelViewportEffect() {;}
	virtual ~StelViewportEffect() {;}
	virtual QString getName() {return "framebufferOnly";}
	//! Alter the GL frame buffer, this method must not display anything.
	//! The default implementation does nothing.
	virtual void alterBuffer(QGLFramebufferObject*) const {;}
	//! Draw the viewport on the screen.
	//! @param buf the GL frame buffer containing the Stellarium viewport alreay drawn.
	//! The default implementation paints the buffer on the fullscreen.
	virtual void paintViewportBuffer(const QGLFramebufferObject* buf) const;
	//! Distort an x,y position according to the distortion.
	//! The default implementation does nothing.
	virtual void distortXY(float& x, float& y) const {Q_UNUSED(x); Q_UNUSED(y);}
};


class StelViewportDistorterFisheyeToSphericMirror : public StelViewportEffect
{
public:
	StelViewportDistorterFisheyeToSphericMirror(int screenWidth,int screenHeight);
	~StelViewportDistorterFisheyeToSphericMirror();
	virtual QString getName() {return "sphericMirrorDistorter";}
	virtual void paintViewportBuffer(const QGLFramebufferObject* buf) const;
	virtual void distortXY(float& x, float& y) const;
private:
	const int screenWidth;
	const int screenHeight;
	const StelProjector::StelProjectorParams originalProjectorParams;
	StelProjector::StelProjectorParams newProjectorParams;
	int viewportTextureOffset[2];
	int texture_wh;

	Vec2f *texturePointGrid;
	int maxGridX,maxGridY;
	double stepX,stepY;

	QVector<Vec2f> displayVertexList;
	QVector<Vec4f> displayColorList;
	QVector<Vec2f> displayTexCoordList;
	
	void constructVertexBuffer(const class VertexPoint * const vertexGrid);
	void generateDistortion(const class QSettings& conf, const StelProjectorP& proj, 
	                        const double distorterMaxFOV);
	bool loadDistortionFromFile(const QString & fileName);
};

#endif // _STELVIEWPORTEFFECT_HPP_

