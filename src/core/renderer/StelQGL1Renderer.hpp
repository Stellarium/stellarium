/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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

#ifndef _STELQGL1RENDERER_HPP_
#define _STELQGL1RENDERER_HPP_


#include <QVector>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelQGLRenderer.hpp"
#include "StelProjector.hpp"
#include "StelProjectorClasses.hpp"
#include "StelQGL1ArrayVertexBufferBackend.hpp"


//! Renderer backend using OpenGL 1.1 with Qt.
//!
//! GL 1.0 is not supported as it doesn't have vertex arrays.
//! That said, pretty much all GPUs in history support 1.1. (introduced in 1997) - 
//! and I doubt even those are powerful enough to run Stellarium 
//! (e.g. ATI Rage GPUs?)
//!
//! Even though later GL1 versions and extensions have many more useful features 
//! (e.g. basic shaders, VBOs, etc.), these are not used - with the exception of 
//! multitexturing, which is still not required - so compatibility is as high as 
//! possible.
class StelQGL1Renderer : public StelQGLRenderer
{
public:
	//! Construct a StelQGL1Renderer.
	//!
	//! @param parent Parent widget for the renderer's GL widget.
	StelQGL1Renderer(QGraphicsView* parent)
		: StelQGLRenderer(parent, false)
		, initialized(false)
	{
	}
	
	virtual ~StelQGL1Renderer()
	{
		// Can't check the invariant here, as the renderer will be destroyed even 
		// if it's initialization has failed.

		initialized = false;
	}
	
	virtual bool init()
	{
		Q_ASSERT_X(!initialized, Q_FUNC_INFO, 
		           "StelQGL1Renderer is already initialized");
		
		// Using  this instead of makeGLContextCurrent() to avoid invariant 
		// as we're not in valid state (getGLContext() isn't public - it doesn't call invariant)
		getGLContext()->makeCurrent();

		if(!StelQGLRenderer::init())
		{
			qWarning() << "StelQGL1Renderer::init : parent init failed";
			return false;
		}
		
		initialized = true;
		invariant();
		return true;
	}

	virtual bool areFloatTexturesSupported() const {return false;}

	virtual bool areNonPowerOfTwoTexturesSupported() const {return false;}

	virtual StelGLSLShader* createGLSLShader()
	{
		// GL1 doesn't support shaders.
		return NULL;
	}

	virtual bool isGLSLSupported() const {return false;}

protected:
	virtual StelVertexBufferBackend* createVertexBufferBackend
		(const PrimitiveType primitiveType, const QVector<StelVertexAttribute>& attributes)
	{
		invariant();
		return new StelQGL1ArrayVertexBufferBackend(primitiveType, attributes);
	}

	virtual void drawVertexBufferBackend(StelVertexBufferBackend* vertexBuffer, 
	                                     StelIndexBuffer* indexBuffer = NULL,
	                                     StelProjector* projector = NULL,
	                                     bool dontProject = false)
	{
		invariant();

		// Save openGL projection state
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glDisable(GL_LIGHTING);
		glDisable(GL_MULTISAMPLE);
		glDisable(GL_DITHER);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_LINE_SMOOTH);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glShadeModel(GL_SMOOTH);

		StelQGL1ArrayVertexBufferBackend* backend =
			dynamic_cast<StelQGL1ArrayVertexBufferBackend*>(vertexBuffer);
		Q_ASSERT_X(backend != NULL, Q_FUNC_INFO,
		           "StelQGL1Renderer: Vertex buffer created by different renderer backend "
		           "or uninitialized");
		
		StelQGLIndexBuffer* glIndexBuffer = NULL;
		if(indexBuffer != NULL)
		{
			glIndexBuffer = dynamic_cast<StelQGLIndexBuffer*>(indexBuffer);
			Q_ASSERT_X(glIndexBuffer != NULL, Q_FUNC_INFO,
			           "StelQGL1Renderer: Index buffer created by different renderer " 
			           "backend or uninitialized");
		}

		if(NULL == projector)
		{
			projector = &(*(StelApp::getInstance().getCore()->getProjection2d()));
		}
		// XXX: we should use a more generic way to test whether or not to do the projection.
		else if(!dontProject && (NULL == dynamic_cast<StelProjector2d*>(projector)))
		{
			backend->projectVertices(projector, glIndexBuffer);
		}

		// Instead of setting GL state when functions such as setDepthTest() or setCulledFaces()
		// are called, we only set it before drawing and reset after drawing to avoid 
		setupGLState(projector);


		// Set up viewport for the projector.
		const Vec4i viewXywh = projector->getViewportXywh();
		glViewport(viewXywh[0], viewXywh[1], viewXywh[2], viewXywh[3]);
		backend->draw(*this, projector->getProjectionMatrix(), glIndexBuffer);

		// Restore default state to avoid interfering with Qt OpenGL drawing.
		restoreGLState(projector);

		// Restore openGL projection state for Qt drawings
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		invariant();
	}

	virtual int getTextureUnitCountBackend() 
	{
		// Called at initialization, so can't call invariant
		if(!gl.hasOpenGLFeature(QGLFunctions::Multitexture))
		{
			invariant();
			return 1;
		}
		int textureUnitCount;
		glGetIntegerv(GL_MAX_TEXTURE_UNITS, &textureUnitCount);
		return std::max(textureUnitCount, STELQGLRENDERER_MAX_TEXTURE_UNITS);
	}

#ifndef NDEBUG
	virtual void invariant() const
	{
		Q_ASSERT_X(initialized, Q_FUNC_INFO, "uninitialized StelQGL1Renderer");
		StelQGLRenderer::invariant();
	}
#endif
	
private:
	//! Is the renderer initialized?
	bool initialized;
};

#endif // _STELQGL1RENDERER_HPP_

