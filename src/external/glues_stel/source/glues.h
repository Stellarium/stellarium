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
 * OpenGL ES 1.0 CM port of part of GLU by Mike Gorchak <mike@malva.ua>
 *
 * Important: Fabien Chereau - September 2009
 * Most of the content was ripped out for the use of Stellarium
 *
 */

#ifndef __glues_h__
#define __glues_h__

#include "GLee.h"

#ifdef __cplusplus
   extern "C" {
#endif

/*************************************************************/

#ifdef APIENTRY
# undef APIENTRY
# define APIENTRY
#endif

/* Boolean */
#define GLU_FALSE                          0
#define GLU_TRUE                           1

/* Version */
#define GLU_VERSION_1_1                    1
#define GLU_VERSION_1_2                    1
#define GLU_VERSION_1_3                    1

/* StringName */
#define GLU_VERSION                        100800
#define GLU_EXTENSIONS                     100801

/* ErrorCode */
#define GLU_INVALID_ENUM                   100900
#define GLU_INVALID_VALUE                  100901
#define GLU_OUT_OF_MEMORY                  100902
#define GLU_INCOMPATIBLE_GL_VERSION        100903
#define GLU_INVALID_OPERATION              100904

/*************************************************************/


typedef struct GLUtesselator GLUtesselator;
typedef GLUtesselator GLUtesselatorObj;
typedef GLUtesselator GLUtriangulatorObj;

/* Internal convenience typedefs */
typedef void (* _GLUfuncptr)();

GLint gluScaleImage(GLenum format, GLsizei widthin,
                             GLsizei heightin, GLenum typein,
                             const void* datain, GLsizei widthout,
                             GLsizei heightout, GLenum typeout, void* dataout);
GLint gluBuild2DMipmapLevels(GLenum target, GLint internalFormat,
                             GLsizei width, GLsizei height, GLenum format,
                             GLenum type, GLint userLevel, GLint baseLevel,
                             GLint maxLevel, const void *data);
GLint gluBuild2DMipmaps(GLenum target, GLint internalFormat,
                             GLsizei width, GLsizei height, GLenum format,
                             GLenum type, const void* data);

#ifndef GLU_TESS_MAX_COORD
#define GLU_TESS_MAX_COORD 1.0e37f
#endif

/* TessCallback */
#define GLU_TESS_BEGIN                     100100
#define GLU_BEGIN                          100100
#define GLU_TESS_VERTEX                    100101
#define GLU_VERTEX                         100101
#define GLU_TESS_END                       100102
#define GLU_END                            100102
#define GLU_TESS_ERROR                     100103
#define GLU_TESS_EDGE_FLAG                 100104
#define GLU_EDGE_FLAG                      100104
#define GLU_TESS_COMBINE                   100105
#define GLU_TESS_BEGIN_DATA                100106
#define GLU_TESS_VERTEX_DATA               100107
#define GLU_TESS_END_DATA                  100108
#define GLU_TESS_ERROR_DATA                100109
#define GLU_TESS_EDGE_FLAG_DATA            100110
#define GLU_TESS_COMBINE_DATA              100111

/* TessContour */
#define GLU_CW                             100120
#define GLU_CCW                            100121
#define GLU_INTERIOR                       100122
#define GLU_EXTERIOR                       100123
#define GLU_UNKNOWN                        100124

/* TessProperty */
#define GLU_TESS_WINDING_RULE              100140
#define GLU_TESS_BOUNDARY_ONLY             100141
#define GLU_TESS_TOLERANCE                 100142

/* TessError */
#define GLU_TESS_ERROR1                    100151
#define GLU_TESS_ERROR2                    100152
#define GLU_TESS_ERROR3                    100153
#define GLU_TESS_ERROR4                    100154
#define GLU_TESS_ERROR5                    100155
#define GLU_TESS_ERROR6                    100156
#define GLU_TESS_ERROR7                    100157
#define GLU_TESS_ERROR8                    100158
#define GLU_TESS_MISSING_BEGIN_POLYGON     100151
#define GLU_TESS_MISSING_BEGIN_CONTOUR     100152
#define GLU_TESS_MISSING_END_POLYGON       100153
#define GLU_TESS_MISSING_END_CONTOUR       100154
#define GLU_TESS_COORD_TOO_LARGE           100155
#define GLU_TESS_NEED_COMBINE_CALLBACK     100156

/* TessWinding */
#define GLU_TESS_WINDING_ODD               100130
#define GLU_TESS_WINDING_NONZERO           100131
#define GLU_TESS_WINDING_POSITIVE          100132
#define GLU_TESS_WINDING_NEGATIVE          100133
#define GLU_TESS_WINDING_ABS_GEQ_TWO       100134

void gluBeginPolygon(GLUtesselator* tess);
void gluDeleteTess(GLUtesselator* tess);
void gluEndPolygon(GLUtesselator* tess);
void gluGetTessProperty(GLUtesselator* tess, GLenum which, GLdouble* data);
GLUtesselator* gluNewTess(void);
void gluNextContour(GLUtesselator* tess, GLenum type);
void gluTessBeginContour(GLUtesselator* tess);
void gluTessBeginPolygon(GLUtesselator* tess, GLvoid* data);
void gluTessCallback(GLUtesselator* tess, GLenum which, _GLUfuncptr CallBackFunc);
void gluTessEndContour(GLUtesselator* tess);
void gluTessEndPolygon(GLUtesselator* tess);
void gluTessNormal(GLUtesselator* tess, GLdouble valueX, GLdouble valueY, GLdouble valueZ);
void gluTessProperty(GLUtesselator* tess, GLenum which, GLdouble data);
void gluTessVertex(GLUtesselator* tess, GLdouble* location, GLvoid* data);

const GLubyte* gluErrorString(GLenum errorCode);

#ifdef __cplusplus
}
#endif

#endif /* __glues_h__ */
