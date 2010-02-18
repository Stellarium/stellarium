/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 *
 * OpenGL ES CM 1.0 port of part of GLUES by Mike Gorchak <mike@malva.ua>
 */

#ifndef __GLUESES_MIPMAP_H__
#define __GLUESES_MIPMAP_H__

#if defined(Q_OS_MAC) || defined(__APPLE__) || defined(__APPLE_CC__)
# include <OpenGL/gl.h>
#elif defined(QT_OPENGL_ES_1) || defined(QT_OPENGL_ES_1_CL)
# include <GLES/gl.h>
#elif defined(QT_OPENGL_ES_2) || defined(USE_OPENGL_ES2)
# include <GLES2/gl2.h>
#else
# include <GL/gl.h>
#endif

#ifdef __cplusplus
   extern "C" {
#endif

/* ErrorCode */
#define GLUES_INVALID_ENUM                   100900
#define GLUES_INVALID_VALUE                  100901
#define GLUES_OUT_OF_MEMORY                  100902
#define GLUES_INCOMPATIBLE_GL_VERSION        100903
#define GLUES_INVALID_OPERATION              100904

GLint gluesBuild2DMipmapLevels(GLenum target, GLint internalFormat,
							 GLsizei width, GLsizei height, GLenum format,
							 GLenum type, GLint userLevel, GLint baseLevel,
							 GLint maxLevel, const void *data);
GLint gluesBuild2DMipmaps(GLenum target, GLint internalFormat,
							 GLsizei width, GLsizei height, GLenum format,
							 GLenum type, const void* data);

#ifdef __cplusplus
}
#endif

#endif /* __GLUESES_REGISTRY_H__ */
