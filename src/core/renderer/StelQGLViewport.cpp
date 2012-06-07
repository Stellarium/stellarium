#include "StelQGLRenderer.hpp"
#include "StelQGLTextureBackend.hpp"
#include "StelQGLViewport.hpp"

StelTextureBackend* StelQGLViewport::getViewportTextureBackend(StelQGLRenderer* renderer)
{
	return useFBO() 
		? StelQGLTextureBackend::fromFBO(renderer, frontBuffer)
		: StelQGLTextureBackend::fromViewport(renderer, getViewportSize(), 
		                                      renderer->getGLContext()->format());
}
