#ifndef _STELQGL2RENDERER_HPP_
#define _STELQGL2RENDERER_HPP_


#include "StelQGLRenderer.hpp"


//! Renderer backend using OpenGL2 with Qt.
class StelQGL2Renderer : public StelQGLRenderer
{
public:
	StelQGL2Renderer(QGraphicsView* parent)
		: StelQGLRenderer(parent)
		, initialized(false)
	{
	}
	
    virtual ~StelQGL2Renderer()
	{
		invariant();
		initialized = false;
	}
	
	virtual void init()
	{
		Q_ASSERT_X(!initialized, "StelQGL2Renderer::init()", 
		           "StelQGLR2enderer is already initialized");
		initialized = true;
		invariant();
		
		StelQGLRenderer::init();
	}
	
protected:
	virtual void invariant()
	{
		Q_ASSERT_X(initialized, "StelQGL2Renderer::invariant()", 
		           "uninitialized StelQGLRenderer");
		StelQGLRenderer::invariant();
	}
	
private:
	//! Is the renderer initialized?
	bool initialized;
};

#endif // _STELQGL2RENDERER_HPP_
