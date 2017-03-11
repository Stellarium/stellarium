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
 * OpenGL ES 1.0 CM port of part of GLUES by Mike Gorchak <mike@malva.ua>
 *
 * Important: Fabien Chereau - September 2009
 * Most of the content was ripped out for the use of Stellarium
 *
 */

#ifndef __glues_h__
#define __glues_h__

//just include Qt's OpenGL wrapper instead of our own selection logic
#include <qopengl.h>

#ifdef __cplusplus
   extern "C" {
#endif

/*************************************************************/

/* ErrorCode */
#define GLUES_INVALID_ENUM                   100900
#define GLUES_INVALID_VALUE                  100901
#define GLUES_OUT_OF_MEMORY                  100902
#define GLUES_INCOMPATIBLE_GL_VERSION        100903
#define GLUES_INVALID_OPERATION              100904

/*************************************************************/

typedef struct GLUEStesselator GLUEStesselator;
typedef GLUEStesselator GLUEStesselatorObj;
typedef GLUEStesselator GLUEStriangulatorObj;

/* Internal convenience typedefs */
typedef void (* _GLUESfuncptr)();

GLint gluesBuild2DMipmapLevels(GLenum target, GLint internalFormat,
							 GLsizei width, GLsizei height, GLenum format,
							 GLenum type, GLint userLevel, GLint baseLevel,
							 GLint maxLevel, const void *data);
GLint gluesBuild2DMipmaps(GLenum target, GLint internalFormat,
							 GLsizei width, GLsizei height, GLenum format,
							 GLenum type, const void* data);

#ifndef GLUES_TESS_MAX_COORD
#define GLUES_TESS_MAX_COORD 1.0e37f
#endif

/* TessCallback */
#define GLUES_TESS_BEGIN                     100100
#define GLUES_BEGIN                          100100
#define GLUES_TESS_VERTEX                    100101
#define GLUES_VERTEX                         100101
#define GLUES_TESS_END                       100102
#define GLUES_END                            100102
#define GLUES_TESS_ERROR                     100103
#define GLUES_TESS_EDGE_FLAG                 100104
#define GLUES_EDGE_FLAG                      100104
#define GLUES_TESS_COMBINE                   100105
#define GLUES_TESS_BEGIN_DATA                100106
#define GLUES_TESS_VERTEX_DATA               100107
#define GLUES_TESS_END_DATA                  100108
#define GLUES_TESS_ERROR_DATA                100109
#define GLUES_TESS_EDGE_FLAG_DATA            100110
#define GLUES_TESS_COMBINE_DATA              100111

/* TessContour */
#define GLUES_CW                             100120
#define GLUES_CCW                            100121
#define GLUES_INTERIOR                       100122
#define GLUES_EXTERIOR                       100123
#define GLUES_UNKNOWN                        100124

/* TessProperty */
#define GLUES_TESS_WINDING_RULE              100140
#define GLUES_TESS_BOUNDARY_ONLY             100141
#define GLUES_TESS_TOLERANCE                 100142

/* TessError */
#define GLUES_TESS_ERROR1                    100151
#define GLUES_TESS_ERROR2                    100152
#define GLUES_TESS_ERROR3                    100153
#define GLUES_TESS_ERROR4                    100154
#define GLUES_TESS_ERROR5                    100155
#define GLUES_TESS_ERROR6                    100156
#define GLUES_TESS_ERROR7                    100157
#define GLUES_TESS_ERROR8                    100158
#define GLUES_TESS_MISSING_BEGIN_POLYGON     100151
#define GLUES_TESS_MISSING_BEGIN_CONTOUR     100152
#define GLUES_TESS_MISSING_END_POLYGON       100153
#define GLUES_TESS_MISSING_END_CONTOUR       100154
#define GLUES_TESS_COORD_TOO_LARGE           100155
#define GLUES_TESS_NEED_COMBINE_CALLBACK     100156

/* TessWinding */
#define GLUES_TESS_WINDING_ODD               100130
#define GLUES_TESS_WINDING_NONZERO           100131
#define GLUES_TESS_WINDING_POSITIVE          100132
#define GLUES_TESS_WINDING_NEGATIVE          100133
#define GLUES_TESS_WINDING_ABS_GEQ_TWO       100134

void gluesBeginPolygon(GLUEStesselator* tess);
void gluesDeleteTess(GLUEStesselator* tess);
void gluesEndPolygon(GLUEStesselator* tess);
void gluesGetTessProperty(GLUEStesselator* tess, GLenum which, double* data);
GLUEStesselator* gluesNewTess(void);
void gluesNextContour(GLUEStesselator* tess, GLenum type);
void gluesTessBeginContour(GLUEStesselator* tess);
void gluesTessBeginPolygon(GLUEStesselator* tess, GLvoid* data);
void gluesTessCallback(GLUEStesselator* tess, GLenum which, _GLUESfuncptr CallBackFunc);
void gluesTessEndContour(GLUEStesselator* tess);
void gluesTessEndPolygon(GLUEStesselator* tess);
void gluesTessNormal(GLUEStesselator* tess, double valueX, double valueY, double valueZ);
void gluesTessProperty(GLUEStesselator* tess, GLenum which, double data);
void gluesTessVertex(GLUEStesselator* tess, double* location, GLvoid* data);

const GLubyte* gluesErrorString(GLenum errorCode);

#ifdef __cplusplus
}
#endif

#endif /* __glues_h__ */
