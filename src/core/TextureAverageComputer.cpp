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
#include "StelMainView.hpp"
#include "StelUtils.hpp"
#include <array>
#include <QOpenGLExtraFunctions>

namespace
{
class FBORestorer
{
	QOpenGLExtraFunctions& gl;
	GLint oldFBO=-1;
public:
	FBORestorer(QOpenGLExtraFunctions& gl)
		: gl(gl)
	{
	}
	void capture()
	{
		GL(gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO));
	}
	~FBORestorer()
	{
		if (oldFBO >= 0)
			GL(gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldFBO));
	}
};
}

Vec4f TextureAverageComputer::getCurrentTextureDeepestMipLevelPixelGLES(const int width, const int height) const
{
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));

	blitTexProgram->bind();

	GLint oldViewport[4];
	GL(gl.glGetIntegerv(GL_VIEWPORT, oldViewport));
	GLint oldFBO=-1;
	GL(gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO));

	glesFBO->bind();
	GL(gl.glViewport(0,0,1,1));

	GL(gl.glBindVertexArray(vao));
	GL(gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
	GL(gl.glBindVertexArray(0));

	blitTexProgram->release();
	Vec4f pixel(NAN,NAN,NAN,NAN);
	if (textureIsFloat)
	{
		GL(gl.glReadPixels(0,0,1,1,GL_RGBA,GL_FLOAT,&pixel[0]));
	}
	else
	{
		std::array<uint8_t, 4> data{};
		GL(gl.glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,&data[0]));
		pixel = {data[0] / 255.f, data[1] / 255.f, data[2] / 255.f, data[3] / 255.f};
	}

	GL(gl.glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]));
	GL(gl.glBindFramebuffer(GL_FRAMEBUFFER, oldFBO));

	return pixel;
}

Vec4f TextureAverageComputer::getCurrentTextureDeepestMipLevelPixelGL(const int width, const int height) const
{
#if !QT_CONFIG(opengles2)
	using namespace std;
	// Formula from the glspec, "Mipmapping" subsection in section 3.8.11 Texture Minification
	const auto totalMipmapLevels = 1+std::floor(log2(max(width,height)));
	const auto deepestLevel=totalMipmapLevels-1;

#ifndef NDEBUG
	// Sanity check
	int deepestMipmapLevelWidth=-1, deepestMipmapLevelHeight=-1;
	GL(gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, deepestLevel, GL_TEXTURE_WIDTH, &deepestMipmapLevelWidth));
	GL(gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, deepestLevel, GL_TEXTURE_HEIGHT, &deepestMipmapLevelHeight));
	assert(deepestMipmapLevelWidth==1);
	assert(deepestMipmapLevelHeight==1);
#endif

	Vec4f pixel;
	GL(hiGL->glGetTexImage(GL_TEXTURE_2D, deepestLevel, GL_RGBA, GL_FLOAT, &pixel[0]));
	return pixel;
#else
	return {NAN, NAN, NAN, NAN};
#endif
}

Vec4f TextureAverageComputer::getTextureAverageSimple(const GLuint texture, const int width, const int height)
{
	// Get average value of the pixels as the value of the deepest mipmap level
	GL(gl.glActiveTexture(GL_TEXTURE0));
	GL(gl.glBindTexture(GL_TEXTURE_2D, texture));
	GL(gl.glGenerateMipmap(GL_TEXTURE_2D));

	if (isGLES)
		return getCurrentTextureDeepestMipLevelPixelGLES(width, height);
	else
		return getCurrentTextureDeepestMipLevelPixelGL(width, height);
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

	GL(gl.glActiveTexture(GL_TEXTURE0));
	GL(gl.glBindTexture(GL_TEXTURE_2D, texture));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

	blitTexProgram->bind();

	GLint oldViewport[4];
	GL(gl.glGetIntegerv(GL_VIEWPORT, oldViewport));
	GLint oldFBO=-1;
	GL(gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO));

	GL(gl.glBindFramebuffer(GL_FRAMEBUFFER, potFBO));
	GL(gl.glViewport(0,0,potWidth,potHeight));

	GL(gl.glBindVertexArray(vao));
	GL(gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
	GL(gl.glBindVertexArray(0));

	GL(gl.glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]));
	GL(gl.glBindFramebuffer(GL_FRAMEBUFFER, oldFBO));

	blitTexProgram->release();

	return getTextureAverageSimple(potTex, potWidth, potHeight);
}

Vec4f TextureAverageComputer::getTextureAverage(const GLuint texture)
{
	if(npotWorkaroundNeeded)
		return getTextureAverageWithWorkaround(texture);
	return getTextureAverageSimple(texture, npotWidth, npotHeight);
}

void TextureAverageComputer::checkNeedForWorkaround()
{
	GLuint texture = -1;
	GL(gl.glGenTextures(1, &texture));
	Q_ASSERT(texture>0);
	GL(gl.glActiveTexture(GL_TEXTURE0));
	GL(gl.glBindTexture(GL_TEXTURE_2D, texture));

	std::vector<Vector4<uint8_t>> data;
	constexpr uint8_t M = 255;
	for(int n=0; n<10; ++n)
		data.emplace_back(M,M,M,M);
	for(int n=0; n<10; ++n)
		data.emplace_back(M,M,M,0);
	for(int n=0; n<10; ++n)
		data.emplace_back(M,M,0,0);
	for(int n=0; n<10; ++n)
		data.emplace_back(M,0,0,0);

	constexpr int width = 63;
	for(int n=data.size(); n<width; ++n)
		data.emplace_back(0,0,0,0);

	GL(gl.glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,data.size(),1,0,GL_RGBA,GL_UNSIGNED_BYTE,&data[0][0]));
	const auto mipmapAverage = getTextureAverageSimple(texture, width, 1);

	const auto sum = std::accumulate(data.begin(), data.end(), Vec4f(0,0,0,0),
	                                 [](const Vec4f& left, const Vector4<uint8_t>& right) { return left + Vec4f(right); });
	const auto trueAverage = Vec4f(sum) / float(data.size()) / M;
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
	npotWorkaroundNeeded = maxDiff >= 2./255.;

	if(npotWorkaroundNeeded)
	{
		qDebug() << "Mipmap average is unusable, will resize textures to "
					"power-of-two size when average value is required.";
	}
	else
	{
		qDebug() << "Mipmap average works correctly";
	}

	GL(gl.glBindTexture(GL_TEXTURE_2D, 0));
	GL(gl.glDeleteTextures(1, &texture));

	needForWorkaroundChecked = true;
}

// Clobbers: GL_TEXTURE_BINDING_2D, GL_VERTEX_ARRAY_BINDING, GL_ARRAY_BUFFER_BINDING
TextureAverageComputer::TextureAverageComputer(StelOpenGL::HighGraphicsFunctions* hiGL, const int texWidth, const int texHeight, const GLenum internalFormat, const bool textureIsFloat)
	: gl(*QOpenGLContext::currentContext()->extraFunctions())
	, hiGL(hiGL)
	, npotWidth(texWidth)
	, npotHeight(texHeight)
	, textureIsFloat(textureIsFloat)
{
	isGLES = StelMainView::getInstance().getGLInformation().isGLES || !hiGL;

	GL(gl.glGenVertexArrays(1, &vao));
	GL(gl.glBindVertexArray(vao));
	GL(gl.glGenBuffers(1, &vbo));
	GL(gl.glBindBuffer(GL_ARRAY_BUFFER, vbo));
	const GLfloat vertices[]=
	{
		-1, -1,
		 1, -1,
		-1,  1,
		 1,  1,
	};
	GL(gl.glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW));
	constexpr GLuint attribIndex=0;
	constexpr int coordsPerVertex=2;
	GL(gl.glVertexAttribPointer(attribIndex, coordsPerVertex, GL_FLOAT, false, 0, 0));
	GL(gl.glEnableVertexAttribArray(attribIndex));
	GL(gl.glBindVertexArray(0));

	blitTexProgram.reset(new QOpenGLShaderProgram);
	blitTexProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
		StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) +
		R"(
ATTRIBUTE vec4 vertex;
VARYING vec2 texcoord;
void main()
{
    gl_Position = vertex;
    texcoord = vertex.st*0.5+vec2(0.5);
}
)");
	blitTexProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
		StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) +
		R"(
VARYING vec2 texcoord;
uniform sampler2D tex;
void main()
{
    FRAG_COLOR = texture2D(tex, texcoord);
}
)");
	blitTexProgram->bindAttributeLocation("vertex", 0);
	blitTexProgram->link();
	blitTexProgram->bind();
	blitTexProgram->setUniformValue("tex", 0);
	blitTexProgram->release();

	FBORestorer fboRestorer(gl);
	if (isGLES)
	{
		fboRestorer.capture();
		glesFBO.reset(new QOpenGLFramebufferObject(1, 1, QOpenGLFramebufferObject::NoAttachment,
		                                           GL_TEXTURE_2D, internalFormat));
	}

	if(!needForWorkaroundChecked) checkNeedForWorkaround();

	if(npotWorkaroundNeeded)
	{
		fboRestorer.capture();

		GL(gl.glGenFramebuffers(1, &potFBO));
		GL(gl.glGenTextures(1, &potTex));
		GL(gl.glBindTexture(GL_TEXTURE_2D, potTex));
		const auto potWidth  = StelUtils::getSmallerPowerOfTwo(npotWidth);
		const auto potHeight = StelUtils::getSmallerPowerOfTwo(npotHeight);
		GL(gl.glTexImage2D(GL_TEXTURE_2D,0,internalFormat,potWidth,potHeight,0,GL_RGBA,
		                   textureIsFloat ? GL_FLOAT : GL_UNSIGNED_BYTE,nullptr));
		GL(gl.glBindTexture(GL_TEXTURE_2D,0));
		GL(gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER,potFBO));
		GL(gl.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,potTex,0));
		[[maybe_unused]] const auto status=gl.glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		Q_ASSERT(status==GL_FRAMEBUFFER_COMPLETE);
		GL(gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0));
	}
}

TextureAverageComputer::~TextureAverageComputer()
{
	GL(gl.glDeleteTextures(1, &potTex));
	GL(gl.glDeleteFramebuffers(1, &potFBO));
	GL(gl.glDeleteVertexArrays(1, &vao));
	GL(gl.glDeleteBuffers(1, &vbo));
}
