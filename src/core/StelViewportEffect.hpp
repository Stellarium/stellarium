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

#ifndef STELVIEWPORTEFFECT_HPP
#define STELVIEWPORTEFFECT_HPP

#include "VecMath.hpp"
#include "StelProjector.hpp"

class QOpenGLFramebufferObject;

//! @class StelViewportEffect
//! Allow to apply visual effects on the whole Stellarium viewport.
class StelViewportEffect
{
public:
	StelViewportEffect() {;}
	virtual ~StelViewportEffect() {;}
	virtual QString getName() const {return "framebufferOnly";}
	//! Alter the GL frame buffer, this method must not display anything.
	//! The default implementation does nothing.
	virtual void alterBuffer(QOpenGLFramebufferObject*) const {;}
	//! Draw the viewport on the screen.
	//! @param buf the GL frame buffer containing the Stellarium viewport alreay drawn.
	//! The default implementation paints the buffer on the fullscreen.
	virtual void paintViewportBuffer(const QOpenGLFramebufferObject* buf) const;
	//! Distort an x,y position according to the distortion.
	//! The default implementation does nothing.
	virtual void distortXY(qreal& x, qreal& y) const {Q_UNUSED(x); Q_UNUSED(y);}
};


class StelViewportDistorterFisheyeToSphericMirror : public StelViewportEffect
{
public:
	StelViewportDistorterFisheyeToSphericMirror(int screen_w,int screen_h);
	~StelViewportDistorterFisheyeToSphericMirror();
	virtual QString getName() const {return "sphericMirrorDistorter";}
	virtual void paintViewportBuffer(const QOpenGLFramebufferObject* buf) const;
	virtual void distortXY(qreal& x, qreal& y) const;
private:
	const int screen_w;
	const int screen_h;
	const StelProjector::StelProjectorParams originalProjectorParams;
	StelProjector::StelProjectorParams newProjectorParams;
	int viewport_texture_offset[2];

	Vec2f *texture_point_array;
	int max_x,max_y;
	float step_x,step_y;

	QVector<Vec2f> displayVertexList;
	QVector<Vec4f> displayColorList;
	QVector<Vec2f> displayTexCoordList;
};

#endif // STELVIEWPORTEFFECT_HPP

