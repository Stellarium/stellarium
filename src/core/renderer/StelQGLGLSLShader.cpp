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

#include "StelQGLGLSLShader.hpp"
#include "StelQGL2Renderer.hpp"

StelQGLGLSLShader::StelQGLGLSLShader(StelQGL2Renderer* renderer) 
	: StelGLSLShader()
	, renderer(renderer)
	, program(renderer->getGLContext())
	, bound(false)
	, built(false)
	, useUnprojectedPosition_(false)
{
}

void StelQGLGLSLShader::bind()
{
	Q_ASSERT_X(built, Q_FUNC_INFO, "Trying to bind a non-built shader");
	Q_ASSERT_X(!bound, Q_FUNC_INFO, "Must release() a shader before calling bind() again");
	program.bind();
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
	program.release();
}
