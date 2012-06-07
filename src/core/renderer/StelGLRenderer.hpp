#ifndef _STELGLRENDERER_HPP_
#define _STELGLRENDERER_HPP_

#include <QSize>

#include "StelGLUtilityFunctions.hpp"
#include "StelRenderer.hpp"


//! Base class for OpenGL based renderers.
//!
class StelGLRenderer : public StelRenderer
{
public:
	//! Construct a StelGLRenderer.
	StelGLRenderer()
		: viewportSize(QSize())
		, drawing(false)
	{
	}
	
	virtual QSize getViewportSize() const {return viewportSize;}
	
protected:
	//! Check for any OpenGL errors. Useful for detecting incorrect GL code.
	void checkGLErrors() const
	{
		const GLenum glError = glGetError();
		if(glError == GL_NO_ERROR) {return;}

		qWarning() << "OpenGL error detected: " << glErrorToString(glError);
	}

protected:
	//! Graphics scene size.
	QSize viewportSize;
	
	//! Are we in the middle of drawing?
	bool drawing;
};

#endif // _STELGLRENDERER_HPP_
