/*
 * Stellarium
 * Copyright (C) 2014 Fabien Chereau
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

#ifndef _STELOPENGL_HPP_
#define _STELOPENGL_HPP_

#define GL_GLEXT_PROTOTYPES
#include <qopengl.h>

#include <QOpenGLFunctions>

// On windows without ANGLE support, we should not directly call some
// of the OpenGL functions, instead we have to use QOpenGLFunctions.
//
// Here we replace the functions with a macro that will invoke the proper
// calls.
#if defined(Q_OS_WIN) && !defined(Q_NO_OPENGL)
#  define GLFUNC_(x) QOpenGLContext::currentContext()->functions()->x
#else
#  define GLFUNC_(x) x
#endif

#define glActiveTexture(...)        GLFUNC_(glActiveTexture(__VA_ARGS__))
#define glAttachShader(...)         GLFUNC_(glAttachShader(__VA_ARGS__))
#define glBindAttribLocation(...)   GLFUNC_(glBindAttribLocation(__VA_ARGS__))
#define glBindBuffer(...)           GLFUNC_(glBindBuffer(__VA_ARGS__))
#define glBindFramebuffer(...)      GLFUNC_(glBindFramebuffer(__VA_ARGS__))
#define glBindRenderbuffer(...)     GLFUNC_(glBindRenderbuffer(__VA_ARGS__))
#define glBlendColor(...)           GLFUNC_(glBlendColor(__VA_ARGS__))
#define glBlendEquation(...)        GLFUNC_(glBlendEquation(__VA_ARGS__))
#define glBlendEquationSeparate(...)    GLFUNC_(glBlendEquationSeparate(__VA_ARGS__))
#define glBlendFuncSeparate(...)    GLFUNC_(glBlendFuncSeparate(__VA_ARGS__))
#define glBufferData(...)           GLFUNC_(glBufferData(__VA_ARGS__))
#define glBufferSubData(...)        GLFUNC_(glBufferSubData(__VA_ARGS__))
#define glCheckFramebufferStatus(...)   GLFUNC_(glCheckFramebufferStatus(__VA_ARGS__))
#define glClearDepthf(...)          GLFUNC_(glClearDepthf(__VA_ARGS__))
#define glCompileShader(...)        GLFUNC_(glCompileShader(__VA_ARGS__))
#define glCompressedTexImage2D(...) GLFUNC_(glCompressedTexImage2D(__VA_ARGS__))
#define glCompressedTexSubImage2D(...)  GLFUNC_(glCompressedTexSubImage2D(__VA_ARGS__))
#define glCreateProgram(...)        GLFUNC_(glCreateProgram(__VA_ARGS__))
#define glCreateShader(...)         GLFUNC_(glCreateShader(__VA_ARGS__))
#define glDeleteBuffers(...)        GLFUNC_(glDeleteBuffers(__VA_ARGS__))
#define glDeleteFramebuffers(...)   GLFUNC_(glDeleteFramebuffers(__VA_ARGS__))
#define glDeleteProgram(...)        GLFUNC_(glDeleteProgram(__VA_ARGS__))
#define glDeleteRenderbuffers(...)  GLFUNC_(glDeleteRenderbuffers(__VA_ARGS__))
#define glDeleteShader(...)         GLFUNC_(glDeleteShader(__VA_ARGS__))
#define glDepthRangef(...)          GLFUNC_(glDepthRangef(__VA_ARGS__))
#define glDetachShader(...)         GLFUNC_(glDetachShader(__VA_ARGS__))
#define glDisableVertexAttribArray(...) GLFUNC_(glDisableVertexAttribArray(__VA_ARGS__))
#define glEnableVertexAttribArray(...)  GLFUNC_(glEnableVertexAttribArray(__VA_ARGS__))
#define glFramebufferRenderbuffer(...)  GLFUNC_(glFramebufferRenderbuffer(__VA_ARGS__))
#define glFramebufferTexture2D(...) GLFUNC_(glFramebufferTexture2D(__VA_ARGS__))
#define glGenBuffers(...)           GLFUNC_(glGenBuffers(__VA_ARGS__))
#define glGenerateMipmap(...)       GLFUNC_(glGenerateMipmap(__VA_ARGS__))
#define glGenFramebuffers(...)      GLFUNC_(glGenFramebuffers(__VA_ARGS__))
#define glGenRenderbuffers(...)     GLFUNC_(glGenRenderbuffers(__VA_ARGS__))
#define glGetActiveAttrib(...)      GLFUNC_(glGetActiveAttrib(__VA_ARGS__))
#define glGetActiveUniform(...)     GLFUNC_(glGetActiveUniform(__VA_ARGS__))
#define glGetAttachedShaders(...)   GLFUNC_(glGetAttachedShaders(__VA_ARGS__))
#define glGetAttribLocation(...)    GLFUNC_(glGetAttribLocation(__VA_ARGS__))
#define glGetBufferParameteriv(...) GLFUNC_(glGetBufferParameteriv(__VA_ARGS__))
#define glGetFramebufferAttachmentParameteriv(...)  GLFUNC_(glGetFramebufferAttachmentParameteriv(__VA_ARGS__))
#define glGetProgramiv(...)         GLFUNC_(glGetProgramiv(__VA_ARGS__))
#define glGetProgramInfoLog(...)    GLFUNC_(glGetProgramInfoLog(__VA_ARGS__))
#define glGetRenderbufferParameteriv(...)   GLFUNC_(glGetRenderbufferParameteriv(__VA_ARGS__))
#define glGetShaderiv(...)          GLFUNC_(glGetShaderiv(__VA_ARGS__))
#define glGetShaderInfoLog(...)     GLFUNC_(glGetShaderInfoLog(__VA_ARGS__))
#define glGetShaderPrecisionFormat(...) GLFUNC_(glGetShaderPrecisionFormat(__VA_ARGS__))
#define glGetShaderSource(...)      GLFUNC_(glGetShaderSource(__VA_ARGS__))
#define glGetUniformfv(...)         GLFUNC_(glGetUniformfv(__VA_ARGS__))
#define glGetUniformiv(...)         GLFUNC_(glGetUniformiv(__VA_ARGS__))
#define glGetUniformLocation(...)   GLFUNC_(glGetUniformLocation(__VA_ARGS__))
#define glGetVertexAttribfv(...)    GLFUNC_(glGetVertexAttribfv(__VA_ARGS__))
#define glGetVertexAttribiv(...)    GLFUNC_(glGetVertexAttribiv(__VA_ARGS__))
#define glGetVertexAttribPointerv(...)  GLFUNC_(glGetVertexAttribPointerv(__VA_ARGS__))
#define glIsBuffer(...) GLFUNC_(glIsBuffer(__VA_ARGS__))
#define glIsFramebuffer(...)        GLFUNC_(glIsFramebuffer(__VA_ARGS__))
#define glIsProgram(...)            GLFUNC_(glIsProgram(__VA_ARGS__))
#define glIsRenderbuffer(...)       GLFUNC_(glIsRenderbuffer(__VA_ARGS__))
#define glIsShader(...)             GLFUNC_(glIsShader(__VA_ARGS__))
#define glLinkProgram(...)          GLFUNC_(glLinkProgram(__VA_ARGS__))
#define glProgramParameteri(...)    GLFUNC_(glProgramParameteri(__VA_ARGS__)) // GZ TRIAL
#define glProgramParameteriEXT(...)    GLFUNC_(glProgramParameteriEXT(__VA_ARGS__)) // GZ TRIAL
#define glReleaseShaderCompiler(...)    GLFUNC_(glReleaseShaderCompiler(__VA_ARGS__))
#define glRenderbufferStorage(...)  GLFUNC_(glRenderbufferStorage(__VA_ARGS__))
#define glSampleCoverage(...)       GLFUNC_(glSampleCoverage(__VA_ARGS__))
#define glShaderBinary(...)         GLFUNC_(glShaderBinary(__VA_ARGS__))
#define glShaderSource(...)         GLFUNC_(glShaderSource(__VA_ARGS__))
#define glStencilFuncSeparate(...)  GLFUNC_(glStencilFuncSeparate(__VA_ARGS__))
#define glStencilMaskSeparate(...)  GLFUNC_(glStencilMaskSeparate(__VA_ARGS__))
#define glStencilOpSeparate(...)    GLFUNC_(glStencilOpSeparate(__VA_ARGS__))
#define glUniform1f(...)            GLFUNC_(glUniform1f(__VA_ARGS__))
#define glUniform1fv(...)           GLFUNC_(glUniform1fv(__VA_ARGS__))
#define glUniform1i(...)            GLFUNC_(glUniform1i(__VA_ARGS__))
#define glUniform1iv(...)           GLFUNC_(glUniform1iv(__VA_ARGS__))
#define glUniform2f(...)            GLFUNC_(glUniform2f(__VA_ARGS__))
#define glUniform2fv(...)           GLFUNC_(glUniform2fv(__VA_ARGS__))
#define glUniform2i(...)            GLFUNC_(glUniform2i(__VA_ARGS__))
#define glUniform2iv(...)           GLFUNC_(glUniform2iv(__VA_ARGS__))
#define glUniform3f(...)            GLFUNC_(glUniform3f(__VA_ARGS__))
#define glUniform3fv(...)           GLFUNC_(glUniform3fv(__VA_ARGS__))
#define glUniform3i(...)            GLFUNC_(glUniform3i(__VA_ARGS__))
#define glUniform3iv(...)           GLFUNC_(glUniform3iv(__VA_ARGS__))
#define glUniform4f(...)            GLFUNC_(glUniform4f(__VA_ARGS__))
#define glUniform4fv(...)           GLFUNC_(glUniform4fv(__VA_ARGS__))
#define glUniform4i(...)            GLFUNC_(glUniform4i(__VA_ARGS__))
#define glUniform4iv(...)           GLFUNC_(glUniform4iv(__VA_ARGS__))
#define glUniformMatrix2fv(...)     GLFUNC_(glUniformMatrix2fv(__VA_ARGS__))
#define glUniformMatrix3fv(...)     GLFUNC_(glUniformMatrix3fv(__VA_ARGS__))
#define glUniformMatrix4fv(...)     GLFUNC_(glUniformMatrix4fv(__VA_ARGS__))
#define glUseProgram(...)           GLFUNC_(glUseProgram(__VA_ARGS__))
#define glValidateProgram(...)      GLFUNC_(glValidateProgram(__VA_ARGS__))
#define glVertexAttrib1f(...)       GLFUNC_(glVertexAttrib1f(__VA_ARGS__))
#define glVertexAttrib1fv(...)      GLFUNC_(glVertexAttrib1fv(__VA_ARGS__))
#define glVertexAttrib2f(...)       GLFUNC_(glVertexAttrib2f(__VA_ARGS__))
#define glVertexAttrib2fv(...)      GLFUNC_(glVertexAttrib2fv(__VA_ARGS__))
#define glVertexAttrib3f(...)       GLFUNC_(glVertexAttrib3f(__VA_ARGS__))
#define glVertexAttrib3fv(...)      GLFUNC_(glVertexAttrib3fv(__VA_ARGS__))
#define glVertexAttrib4f(...)       GLFUNC_(glVertexAttrib4f(__VA_ARGS__))
#define glVertexAttrib4fv(...)      GLFUNC_(glVertexAttrib4fv(__VA_ARGS__))
#define glVertexAttribPointer(...)  GLFUNC_(glVertexAttribPointer(__VA_ARGS__))

#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
#define glGetString(...)            GLFUNC_(glGetString(__VA_ARGS__))
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
#define glBindTexture(...)          GLFUNC_(glBindTexture(__VA_ARGS__))
#define glBlendFunc(...)            GLFUNC_(glBlendFunc(__VA_ARGS__))
#define glClear(...)                GLFUNC_(glClear(__VA_ARGS__))
#define glClearColor(...)           GLFUNC_(glClearColor(__VA_ARGS__))
#define glCullFace(...)             GLFUNC_(glCullFace(__VA_ARGS__))
#define glDeleteTextures(...)       GLFUNC_(glDeleteTextures(__VA_ARGS__))
#define glDepthMask(...)            GLFUNC_(glDepthMask(__VA_ARGS__))
#define glDisable(...)              GLFUNC_(glDisable(__VA_ARGS__))
#define glDrawArrays(...)           GLFUNC_(glDrawArrays(__VA_ARGS__))
#define glDrawElements(...)         GLFUNC_(glDrawElements(__VA_ARGS__))
#define glEnable(...)               GLFUNC_(glEnable(__VA_ARGS__))
#define glGenTextures(...)          GLFUNC_(glGenTextures(__VA_ARGS__))
#define glGetError(...)             GLFUNC_(glGetError(__VA_ARGS__))
#define glGetIntegerv(...)          GLFUNC_(glGetIntegerv(__VA_ARGS__))
#define glFrontFace(...)            GLFUNC_(glFrontFace(__VA_ARGS__))
#define glIsEnabled(...)            GLFUNC_(glIsEnabled(__VA_ARGS__))
#define glIsTexture(...)            GLFUNC_(glIsTexture(__VA_ARGS__))
#define glLineWidth(...)            GLFUNC_(glLineWidth(__VA_ARGS__))
#define glPixelStorei(...)          GLFUNC_(glPixelStorei(__VA_ARGS__))
#define glPolygonOffset(...)        GLFUNC_(glPolygonOffset(__VA_ARGS__))
#define glStencilMask(...)          GLFUNC_(glStencilMask(__VA_ARGS__))
#define glTexImage2D(...)           GLFUNC_(glTexImage2D(__VA_ARGS__))
#define glTexParameterf(...)        GLFUNC_(glTexParameterf(__VA_ARGS__))
#define glTexParameterfv(...)       GLFUNC_(glTexParameterfv(__VA_ARGS__))
#define glTexParameteri(...)        GLFUNC_(glTexParameteri(__VA_ARGS__))
#define glTexParameteriv(...)       GLFUNC_(glTexParameteriv(__VA_ARGS__))
#define glViewport(...)             GLFUNC_(glViewport(__VA_ARGS__))
#endif

#ifndef NDEBUG
# define GL(line) do { \
	line;\
	if (checkGLErrors(__FILE__, __LINE__))\
		exit(-1);\
	} while(0)
#else
# define GL(line) line
#endif

// In ANGLE and other OpenGL ES systems, GL_LINE_SMOOTH is undefined.
// We can define it here to not break compilation, but must test for real OpenGL context before calling.
# ifndef GL_LINE_SMOOTH
#  define GL_LINE_SMOOTH 0x0B20
# endif


#if defined(QT_OPENGL_ES_1) 
#pragma message "==========================This goes to OpenGL ES 1 and will not work"
#elif defined(QT_OPENGL_ES_2) 
#pragma message "==========================This goes to OpenGL ES 2"
#else 
#pragma message "==========================This goes to Desktop OpenGL"
#endif


const char* getGLErrorText(int code);
int checkGLErrors(const char *file, int line);

// To build on arm platforms we need to re-introduce the Qt 5.4 removed
// typedefs and defines of GLdouble/GL_DOUBLE, which are not present in GLES
#if defined(QT_OPENGL_ES_2)
# ifndef GL_DOUBLE
#  define GL_DOUBLE GL_FLOAT
# endif
# ifndef GLdouble
typedef GLfloat GLdouble;
# endif
#endif

#endif // _STELOPENGL_HPP_
