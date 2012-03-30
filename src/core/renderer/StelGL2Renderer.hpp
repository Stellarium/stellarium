#ifndef _STELGL2RENDERER_HPP_
#define _STELGL2RENDERER_HPP_

#include <QImage>
#include <QPainter>

#include "StelGLProvider.hpp"
#include "StelRenderer.hpp"


//! Renderer based on OpenGL 2.x .
class StelGL2Renderer : public StelRenderer
{
public:
	//! Construct a StelGL2Renderer using specified provider to initialize GL context.
	StelGL2Renderer(StelGLProvider* provider)
	    :provider(provider)
	{
	}
	
	virtual void init()
	{
		provider->init();
	}
	
	virtual QImage screenshot()
	{
		return provider->grabFrameBuffer();
	}
	
	virtual void enablePainting(QPainter* painter)
	{
		provider->enablePainting(painter);
	}
	
	virtual void disablePainting()
	{
		provider->disablePainting();
	}
	
private:
	//! Provides GL context and platform (e.g. Qt) specific functionality.
	StelGLProvider* provider;
};

#endif // _STELGL2RENDERER_HPP_
