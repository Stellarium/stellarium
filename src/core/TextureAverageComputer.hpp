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

#if !QT_CONFIG(opengles2) // This class uses glGetTexImage(), which is not supported in GLES2

class QOpenGLFunctions_3_3_Core;
class TextureAverageComputer
{
	QOpenGLFunctions_3_3_Core& gl;
	std::unique_ptr<QOpenGLShaderProgram> blitTexProgram;
	GLuint potFBO = 0;
	GLuint potTex = 0;
	GLuint vbo = 0, vao = 0;
	GLint npotWidth, npotHeight;
	static inline bool inited = false;
	static inline bool workaroundNeeded = false;

    void init();
	Vec4f getTextureAverageSimple(GLuint texture, int width, int height);
	Vec4f getTextureAverageWithWorkaround(GLuint texture);
public:
	Vec4f getTextureAverage(GLuint texture);
    TextureAverageComputer(QOpenGLFunctions_3_3_Core&, int texW, int texH, GLenum internalFormat);
	~TextureAverageComputer();
};

#endif

#endif
