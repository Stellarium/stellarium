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
#include "StelQGL1InterleavedArrayVertexBufferBackend.hpp"


//! Renderer backend using OpenGL 1.2 with Qt.
//!
//! GL 1.0 is not supported as it doesn't have vertex arrays,
//! and GL 1.1 doesn't have clamp to edge texture wrap mode.
//! That said, pretty much everything supports GL 1.2 (introduced in 1998).
//! The oldest GPUs supporting GL 1.2 are ATI Rage and GeForce 2
//! (GeForce 1 and TNT2 support GL 1.2 with updated drivers, although 
//! some functionality might be software emulated).
//!
//! Although later GL1 versions and extensions have many useful features 
//! (basic shaders, VBOs, etc.), these are not used - with the exception of 
//! multitexturing, which is still not mandatory - so compatibility is as high as 
//! possible.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
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
		statistics[VERTEX_BUFFERS_CREATED] += 1.0;
		return new StelQGL1InterleavedArrayVertexBufferBackend(primitiveType, attributes);
	}

	virtual void drawVertexBufferBackend(StelVertexBufferBackend* vertexBuffer, 
	                                     StelIndexBuffer* indexBuffer = NULL,
	                                     StelProjector* projector = NULL,
	                                     bool dontProject = false)
	{
		invariant();

		StelQGL1InterleavedArrayVertexBufferBackend* backend =
			dynamic_cast<StelQGL1InterleavedArrayVertexBufferBackend*>(vertexBuffer);
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
			if(indexBuffer->length() == 0)
			{
				statistics[EMPTY_BATCHES] += 1.0;
				return;
			}
		}
		else if(backend->length() == 0)
		{
			statistics[EMPTY_BATCHES] += 1.0;
			return;
		}

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

		// We need a shared pointer when we're getting the projector ourselves (the 2D case), 
		// to prevent destructor of returned shared pointer from destroying it 
		// before we can use it.
		StelProjectorP projector2DDummy = 
			(NULL != projector ? StelProjectorP(NULL)
			                   : StelApp::getInstance().getCore()->getProjection2d());
		
		projector = (NULL != projector ? projector : &(*(projector2DDummy)));
		// XXX: we should use a more generic way to test whether or not to do the projection.
		if(!dontProject && (NULL == dynamic_cast<StelProjector2d*>(projector)))
		{
			backend->projectVertices(projector, glIndexBuffer);
			statistics[BATCH_PROJECTIONS_CPU_TOTAL] += 1.0;
			statistics[BATCH_PROJECTIONS_CPU]       += 1.0;
		}
		else
		{
			statistics[BATCH_PROJECTIONS_NONE_TOTAL] += 1.0;
			statistics[BATCH_PROJECTIONS_NONE]       += 1.0;
		}

		// Instead of setting GL state when functions such as setDepthTest() or setCulledFaces()
		// are called, we only set it before drawing and reset after drawing to avoid 
		setupGLState(projector);


		// Set up viewport for the projector.
		const Vec4i viewXywh = projector->getViewportXywh();
		glViewport(viewXywh[0], viewXywh[1], viewXywh[2], viewXywh[3]);
		updateDrawStatistics(backend, glIndexBuffer);
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

