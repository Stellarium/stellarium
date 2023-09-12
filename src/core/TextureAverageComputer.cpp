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

#include "TextureAverageComputer.hpp"
#include "StelUtils.hpp"

#if !QT_CONFIG(opengles2) // This class uses glGetTexImage(), which is not supported in GLES2

#include <QOpenGLFunctions_3_3_Core>

Vec4f TextureAverageComputer::getTextureAverageSimple(const GLuint texture, const int width, const int height)
{
	// Get average value of the pixels as the value of the deepest mipmap level
	gl.glActiveTexture(GL_TEXTURE0);
	gl.glBindTexture(GL_TEXTURE_2D, texture);
	gl.glGenerateMipmap(GL_TEXTURE_2D);

	using namespace std;
	// Formula from the glspec, "Mipmapping" subsection in section 3.8.11 Texture Minification
	const auto totalMipmapLevels = 1+floor(log2(max(width,height)));
	const auto deepestLevel=totalMipmapLevels-1;

#ifndef NDEBUG
	// Sanity check
	int deepestMipmapLevelWidth=-1, deepestMipmapLevelHeight=-1;
	gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, deepestLevel, GL_TEXTURE_WIDTH, &deepestMipmapLevelWidth);
	gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, deepestLevel, GL_TEXTURE_HEIGHT, &deepestMipmapLevelHeight);
	assert(deepestMipmapLevelWidth==1);
	assert(deepestMipmapLevelHeight==1);
#endif

	Vec4f pixel;
	gl.glGetTexImage(GL_TEXTURE_2D, deepestLevel, GL_RGBA, GL_FLOAT, &pixel[0]);
	return pixel;
}

// Clobbers:
// GL_VERTEX_ARRAY_BINDING, GL_ACTIVE_TEXTURE, GL_TEXTURE_BINDING_2D, GL_CURRENT_PROGRAM,
// input texture's minification filter
Vec4f TextureAverageComputer::getTextureAverageWithWorkaround(const GLuint texture)
{
	// Play it safe: we don't want to make the GPU struggle with very large textures
	// if we happen to make them ~4 times larger. Instead round the dimensions down.
	const auto potWidth  = StelUtils::getSmallerPowerOfTwo(npotWidth);
	const auto potHeight = StelUtils::getSmallerPowerOfTwo(npotHeight);

	gl.glActiveTexture(GL_TEXTURE0);
	gl.glBindTexture(GL_TEXTURE_2D, texture);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	blitTexProgram->bind();

	GLint oldViewport[4];
	gl.glGetIntegerv(GL_VIEWPORT, oldViewport);
	GLint oldFBO=-1;
	gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

	gl.glBindFramebuffer(GL_FRAMEBUFFER, potFBO);
	gl.glViewport(0,0,potWidth,potHeight);

	gl.glBindVertexArray(vao);
	gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	gl.glBindVertexArray(0);

	gl.glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
	gl.glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);

	blitTexProgram->release();

	return getTextureAverageSimple(potTex, potWidth, potHeight);
}

Vec4f TextureAverageComputer::getTextureAverage(const GLuint texture)
{
	if(workaroundNeeded)
		return getTextureAverageWithWorkaround(texture);
	return getTextureAverageSimple(texture, npotWidth, npotHeight);
}

void TextureAverageComputer::init()
{
	GLuint texture = -1;
	gl.glGenTextures(1, &texture);
	Q_ASSERT(texture>0);
	gl.glActiveTexture(GL_TEXTURE0);
	gl.glBindTexture(GL_TEXTURE_2D, texture);

	std::vector<Vec4f> data;
	for(int n=0; n<10; ++n)
		data.emplace_back(1,1,1,1);
	for(int n=0; n<10; ++n)
		data.emplace_back(1,1,1,0);
	for(int n=0; n<10; ++n)
		data.emplace_back(1,1,0,0);
	for(int n=0; n<10; ++n)
		data.emplace_back(1,0,0,0);

	constexpr int width = 63;
	for(int n=data.size(); n<width; ++n)
		data.emplace_back(0,0,0,0);

	gl.glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,data.size(),1,0,GL_RGBA,GL_FLOAT,&data[0][0]);
	const auto mipmapAverage = getTextureAverageSimple(texture, width, 1);

	const auto sum = std::accumulate(data.begin(), data.end(), Vec4f(0,0,0,0));
	const auto trueAverage = sum / float(data.size());
	qDebug().nospace() << "Test texture true average: "
					   << trueAverage[0] << ", "
					   << trueAverage[1] << ", "
					   << trueAverage[2] << ", "
					   << trueAverage[3];
	qDebug().nospace() << "Test texture mipmap average: "
					   << mipmapAverage[0] << ", "
					   << mipmapAverage[1] << ", "
					   << mipmapAverage[2] << ", "
					   << mipmapAverage[3];

	const auto diff = mipmapAverage - trueAverage;
	using std::abs;
	const auto maxDiff = std::max({abs(diff[0]),abs(diff[1]),abs(diff[2]),abs(diff[3])});
	workaroundNeeded = maxDiff >= 2./255.;

	if(workaroundNeeded)
	{
		qDebug() << "Mipmap average is unusable, will resize textures to "
					"power-of-two size when average value is required.";
	}
	else
	{
		qDebug() << "Mipmap average works correctly";
	}

	gl.glBindTexture(GL_TEXTURE_2D, 0);
	gl.glDeleteTextures(1, &texture);

	inited = true;
}

// Clobbers: GL_TEXTURE_BINDING_2D, GL_VERTEX_ARRAY_BINDING, GL_ARRAY_BUFFER_BINDING
TextureAverageComputer::TextureAverageComputer(QOpenGLFunctions_3_3_Core& gl, const int texWidth, const int texHeight, const GLenum internalFormat)
	: gl(gl)
	, npotWidth(texWidth)
	, npotHeight(texHeight)
{
	if(!inited) init();
	if(!workaroundNeeded) return;

	GLint oldFBO=-1;
	gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

	gl.glGenFramebuffers(1, &potFBO);
	gl.glGenTextures(1, &potTex);
	gl.glBindTexture(GL_TEXTURE_2D, potTex);
	const auto potWidth  = StelUtils::getSmallerPowerOfTwo(npotWidth);
	const auto potHeight = StelUtils::getSmallerPowerOfTwo(npotHeight);
	gl.glTexImage2D(GL_TEXTURE_2D,0,internalFormat,potWidth,potHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
	gl.glBindTexture(GL_TEXTURE_2D,0);
	gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER,potFBO);
	gl.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,potTex,0);
	[[maybe_unused]] const auto status=gl.glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	Q_ASSERT(status==GL_FRAMEBUFFER_COMPLETE);
	gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);

	gl.glGenVertexArrays(1, &vao);
	gl.glBindVertexArray(vao);
	gl.glGenBuffers(1, &vbo);
	gl.glBindBuffer(GL_ARRAY_BUFFER, vbo);
	const GLfloat vertices[]=
	{
		-1, -1,
		 1, -1,
		-1,  1,
		 1,  1,
	};
	gl.glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
	constexpr GLuint attribIndex=0;
	constexpr int coordsPerVertex=2;
	gl.glVertexAttribPointer(attribIndex, coordsPerVertex, GL_FLOAT, false, 0, 0);
	gl.glEnableVertexAttribArray(attribIndex);
	gl.glBindVertexArray(0);

	blitTexProgram.reset(new QOpenGLShaderProgram);
    blitTexProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, 1+R"(
#version 330
layout(location=0) in vec4 vertex;
out vec2 texcoord;
void main()
{
    gl_Position = vertex;
    texcoord = vertex.st*0.5+vec2(0.5);
}
)");
	blitTexProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, 1+R"(
#version 330
in vec2 texcoord;
out vec4 color;
uniform sampler2D tex;
void main()
{
    color = texture(tex, texcoord);
}
)");
	blitTexProgram->link();
	blitTexProgram->bind();
	blitTexProgram->setUniformValue("tex", 0);
	blitTexProgram->release();

	gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldFBO);
}

TextureAverageComputer::~TextureAverageComputer()
{
	gl.glDeleteTextures(1, &potTex);
	gl.glDeleteFramebuffers(1, &potFBO);
	gl.glDeleteVertexArrays(1, &vao);
	gl.glDeleteBuffers(1, &vbo);
}

#endif
