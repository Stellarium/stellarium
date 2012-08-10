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
		, textureUnitCount(-1)
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
	                                     StelProjectorP projector = NULL,
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

		// Instead of setting GL state when functions such as setDepthTest() or setCulledFaces()
		// are called, we only set it before drawing and reset after drawing to avoid 
		// conflicts with e.g. Qt OpenGL backend, or any other GL code that might be running.

		// GL setup before drawing.
		// Fix some problem when using Qt OpenGL2 engine
		glStencilMask(0x11111111);
		
		switch(depthTest)
		{
			case DepthTest_Disabled:
				glDisable(GL_DEPTH_TEST);
				break;
			case DepthTest_ReadOnly:
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				break;
			case DepthTest_ReadWrite:
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown depth test mode");
		}

		switch(stencilTest)
		{
			case StencilTest_Disabled:
				glDisable(GL_STENCIL_TEST);
				break;
			case StencilTest_Write_1:
				glStencilFunc(GL_ALWAYS, 0x1, 0x1);
				glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);
				glEnable(GL_STENCIL_TEST);
				break;
			case StencilTest_DrawIf_1:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_EQUAL, 0x1, 0x1);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown stencil test mode");
		}

		switch(culledFaces)
		{
			case CullFace_None:
				glDisable(GL_CULL_FACE);
				break;
			case CullFace_Front:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;
			case CullFace_Back:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;
			default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown cull face type");
		}

		if(NULL == projector)
		{
			projector = StelApp::getInstance().getCore()->getProjection2d();
		}
		// XXX: we should use a more generic way to test whether or not to do the projection.
		else if(!dontProject && (NULL == dynamic_cast<StelProjector2d*>(projector.data())))
		{
			backend->projectVertices(projector, glIndexBuffer);
		}

		glFrontFace(projector->flipFrontBackFace() ? GL_CW : GL_CCW);

		// Set up viewport for the projector.
		const Vec4i viewXywh = projector->getViewportXywh();
		glViewport(viewXywh[0], viewXywh[1], viewXywh[2], viewXywh[3]);
		backend->draw(*this, projector->getProjectionMatrix(), glIndexBuffer);

		// Restore default state to avoid interfering with Qt OpenGL drawing.
		glFrontFace(projector->flipFrontBackFace() ? GL_CCW : GL_CW);
		glCullFace(GL_BACK);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);

		// Restore openGL projection state for Qt drawings
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		invariant();
	}

	virtual int getTextureUnitCount() 
	{
		invariant();
		if(textureUnitCount < 0)
		{
			if(!gl.hasOpenGLFeature(QGLFunctions::Multitexture))
			{
				textureUnitCount = 1;
			}
			else
			{
				glGetIntegerv(GL_MAX_TEXTURE_UNITS, &textureUnitCount);
				textureUnitCount = std::max(textureUnitCount, STELQGLRENDERER_MAX_TEXTURE_UNITS);
			}
		}
		invariant();
		return textureUnitCount;
	}

	virtual void invariant(const char* const caller = Q_FUNC_INFO) const
	{
#ifndef NDEBUG
		Q_ASSERT_X(initialized, caller, "uninitialized StelQGL1Renderer");
		StelQGLRenderer::invariant(caller);
#endif
	}
	
private:
	//! Is the renderer initialized?
	bool initialized;
	
	//! Number of texture units. Lazily initialized by getTextureUnitCount().
	GLint textureUnitCount;
};

#endif // _STELQGL1RENDERER_HPP_

