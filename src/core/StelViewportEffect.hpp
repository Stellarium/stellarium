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
#include "renderer/StelVertexAttribute.hpp"
#include "renderer/StelVertexBuffer.hpp"

class QGLFramebufferObject;

//! @class StelViewportEffect
//! Allow to apply visual effects on the whole Stellarium viewport.
class StelViewportEffect
{
public:
	StelViewportEffect() {;}
	virtual ~StelViewportEffect() {;}
	virtual QString getName() {return "framebufferOnly";}
	//! Draw the viewport on the screen.
	//! @param buf the GL frame buffer containing the Stellarium viewport alreay drawn.
	//!TODO DOC
	//! The default implementation paints the buffer on the fullscreen.
	virtual void paintViewportBuffer(const QGLFramebufferObject* buf,
	                                 class StelRenderer* renderer) const;
	//! Distort an x,y position according to the distortion.
	//! The default implementation does nothing.
	virtual void distortXY(float& x, float& y) const {Q_UNUSED(x); Q_UNUSED(y);}
};


class StelViewportDistorterFisheyeToSphericMirror : public StelViewportEffect
{
public:
	StelViewportDistorterFisheyeToSphericMirror(int screenWidth, int screenHeight,
	                                            class StelRenderer* renderer);
	~StelViewportDistorterFisheyeToSphericMirror();
	virtual QString getName() {return "sphericMirrorDistorter";}
	virtual void paintViewportBuffer(const QGLFramebufferObject* buf,
	                                 StelRenderer* renderer) const;
	virtual void distortXY(float& x, float& y) const;
private:

	struct Vertex 
	{
		Vec2f position;
		Vec2f texCoord;
		Vec4f color;
		static const QVector<StelVertexAttribute> attributes;
	};
	const int screenWidth;
	const int screenHeight;
	const StelProjector::StelProjectorParams originalProjectorParams;
	StelProjector::StelProjectorParams newProjectorParams;
	int viewportTextureOffset[2];
	int texture_wh;

	//Vec2f *texturePointGrid;
	Vertex* vertexGrid;
	
	int maxGridX,maxGridY;
	double stepX,stepY;

	QVector<StelVertexBuffer<Vertex>*> vertexBuffers;
	
	void constructVertexBuffer(const class Vertex *const vertexGrid, StelRenderer* renderer);
	void generateDistortion(const class QSettings& conf, const StelProjectorP& proj, 
	                        const double distorterMaxFOV, class StelRenderer* renderer);

	//! Load parameters of distortion generation.
	//!
	//! Used by generateDistortion.
	//!
	//! @param conf Configuration to load the parameters from.
	//! @param gamma Gamma correction of the viewport effect will be output here.
	void loadGenerationParameters(const QSettings& conf, double& gamma);
	bool loadDistortionFromFile(const QString & fileName, class StelRenderer *renderer);
};
#endif // _STELVIEWPORTEFFECT_HPP_

