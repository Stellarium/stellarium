#ifndef _STELRENDERER_HPP_
#define _STELRENDERER_HPP_

#include <QImage>
#include <QPainter>

//! Handles all graphics related functionality.
//! 
//! @note This is an interface. It should have only functions, no data members,
//!       as it is used in multiple inheritance.
class StelRenderer
{
public:
	//! Initialize the renderer. Must be called before any other methods.
	virtual void init() = 0;
	
	//! Take a screenshot and return it.
	virtual QImage screenshot() = 0;
	
	//! Enable painting with specified painter, or default initialized painter if NULL.
	//!
	//! Painting must be disabled when calling this.
	virtual void enablePainting(QPainter* painter = NULL) = 0;
	
	//! Disable painting.
	//!
	//! Painting must be enabled when calling this.
	virtual void disablePainting() = 0;
};

#endif // _STELRENDERER_HPP_