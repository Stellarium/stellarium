#ifndef _STELGLPROVIDER_HPP_
#define _STELGLPROVIDER_HPP_

#include <QImage>
#include <QPainter>


//! Provides functionality to set OpenGL up on a platform.
//!
//! @note This is an interface. It should have only functions, no data members,
//!       as it is used in multiple inheritance.
class StelGLProvider
{
public:
	//! Initialize GL context. Must be called before any other methods.
	virtual void init() = 0;
	
	//TODO in future, this should be done by Renderer implementation; not Provider.
	//! Get the framebuffer as an image (used for screenshots).
	virtual QImage grabFrameBuffer() = 0;
	
	//! Enable painting with specified painter, or default initialized painter if NULL.
	//!
	//! Painting must be disabled when calling this.
	virtual void enablePainting(QPainter* painter) = 0;
	
	//! Disable painting.
	//!
	//! Painting must be enabled when calling this.
	virtual void disablePainting() = 0;
};


#endif // _STELGLPROVIDERHPP_

