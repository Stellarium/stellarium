#ifndef _STELRENDERER_HPP_
#define _STELRENDERER_HPP_

#include <QImage>
#include <QPainter>
#include <QSize>

#include "StelViewportEffect.hpp"


//Development notes:
//
//enable/disablePainting are temporary and will be removed
//    once painting is done through StelRenderer.
//
//FPS balancing, currently in StelAppGraphicsWidget, might be moved into StelRenderer
//    implementation.
//
//VisualEffects now always exist. Renderer implementation decides whether to apply them or not.
//    This is because things such as FBOs are implementation details of Renderer,
//    not seen by outside code.
//    If this is not desired, it a visualEffectsSupported() method could be added to Renderer.

//TODO If Renderer implementation now decides whether or not to use VisualEffects,
//     "framebufferOnly" makes no sense, and should be removed.

//! Handles all graphics related functionality.
//! 
//! @note This is an interface. It should have only functions, no data members,
//!       as it might be used in multiple inheritance.
class StelRenderer
{
public:
	//! Initialize the renderer. 
	//! 
	//! Must be called before any other methods.
	//!
	//! @return true on success, false if there was an error.
	virtual bool init() = 0;
	
	//! Take a screenshot and return it.
	virtual QImage screenshot() = 0;
	
	//! Enable painting.
	//!
	//! Painting must be disabled when calling this.
	//!
	//! This method is temporary and will be removed once painting is handled through Renderer.
	virtual void enablePainting() = 0;
	
	//! Disable painting.
	//!
	//! Painting must be enabled when calling this.
	//!
	//! This method is temporary and will be removed once painting is handled through Renderer.
	virtual void disablePainting() = 0;
	
	//! Set default QPainter to use for Qt-based painting commands.
	virtual void setDefaultPainter(QPainter* painter) = 0;
	
	//! Must be called once at startup and on every GL viewport resize, specifying new size.
	virtual void viewportHasBeenResized(QSize size) = 0;
	
	//! Start using drawing calls.
	virtual void startDrawing() = 0;
	
	//! Suspend drawing, not showing the result on the screen. Drawing can be continued later.
	virtual void suspendDrawing() = 0;
	
	//! Finish drawing, showing the result on screen.
	virtual void finishDrawing() = 0;
	
	//! Draw the result of drawing commands to the window, applying given effect if possible.
	virtual void drawWindow(StelViewportEffect* effect) = 0;
};

#endif // _STELRENDERER_HPP_