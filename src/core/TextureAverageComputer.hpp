/*
 * Stellarium
 * Copyright (C) 2023 Ruslan Kabatsayev
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

#ifndef INCLUDE_ONCE_D638566A_098E_4855_B0B5_C8BB5940DA10
#define INCLUDE_ONCE_D638566A_098E_4855_B0B5_C8BB5940DA10

#include <string>
#include <memory>
#include "VecMath.hpp"
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLVertexArrayObject>
#include "StelOpenGL.hpp"

class TextureAverageComputer
{
	QOpenGLExtraFunctions& gl;
	StelOpenGL::HighGraphicsFunctions* hiGL;
	std::unique_ptr<QOpenGLShaderProgram> blitTexProgram;
	//! This FBO is used for getting the deepest mipmap levels of textures in GLES mode.
	std::unique_ptr<QOpenGLFramebufferObject> glesFBO;
	GLuint potFBO = 0;
	GLuint potTex = 0;
	GLuint vbo = 0;
	QOpenGLVertexArrayObject vao;
	GLint npotWidth, npotHeight;
	bool textureIsFloat = false;
	bool isGLES = false;
	static inline bool needForWorkaroundChecked = false;
	static inline bool npotWorkaroundNeeded = false;

	void bindVAO();
	void releaseVAO();
	void setupCurrentVAO();
	void checkNeedForWorkaround();
	Vec4f getTextureAverageWithWorkaround(GLuint texture);
	Vec4f getCurrentTextureDeepestMipLevelPixelGL(const int width, const int height);
	Vec4f getCurrentTextureDeepestMipLevelPixelGLES(const int width, const int height);
public:
	//!< The function to use for arbitrary NPOT textures
	Vec4f getTextureAverage(GLuint texture);
	//!< Can be used for power-of-two textures
	Vec4f getTextureAverageSimple(GLuint texture, int width, int height);
	TextureAverageComputer(StelOpenGL::HighGraphicsFunctions*, int texW, int texH,
	                       GLenum internalFormat, bool textureIsFloat);
	~TextureAverageComputer();
};

#endif
