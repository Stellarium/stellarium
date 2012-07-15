#include "StelQGLGLSLShader.hpp"
#include "StelQGL2Renderer.hpp"

StelQGLGLSLShader::StelQGLGLSLShader(StelQGL2Renderer* renderer) 
	: StelGLSLShader()
	, renderer(renderer)
	, program(renderer->getGLContext())
	, bound(false)
	, built(false)
{
}

void StelQGLGLSLShader::bind()
{
	Q_ASSERT_X(built, Q_FUNC_INFO, "Trying to bind a non-built shader");
	Q_ASSERT_X(!bound, Q_FUNC_INFO, "Must release() a shader before calling bind() again");
	renderer->bindCustomShader(this);
	bound = true;
}

void StelQGLGLSLShader::release()
{
	Q_ASSERT_X(built, Q_FUNC_INFO, "Trying to release a non-built shader");
	Q_ASSERT_X(bound, Q_FUNC_INFO, "Trying to release() a shader that is not bound");
	bound = false;
	// The reference is passed to ensure the custom shader used is really this one.
	renderer->releaseCustomShader(this);
}
