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
 *
 * Important: Fabien Chereau - September 2009
 * Most of the content was ripped out for the use of Stellarium
 */

#ifndef __GLUES_REGISTRY_H__
#define __GLUES_REGISTRY_H__

#ifdef __cplusplus
   extern "C" {
#endif
				 
/* ErrorCode */
#define GLUES_INVALID_ENUM                   100900
#define GLUES_INVALID_VALUE                  100901
#define GLUES_OUT_OF_MEMORY                  100902
#define GLUES_INCOMPATIBLE_GL_VERSION        100903
#define GLUES_INVALID_OPERATION              100904
#define GLUES_STACK_OVERFLOW				0x0503
#define GLUES_STACK_UNDERFLOW				0x0504
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

/* NurbsError */
#define GLUES_NURBS_ERROR1                   100251
#define GLUES_NURBS_ERROR2                   100252
#define GLUES_NURBS_ERROR3                   100253
#define GLUES_NURBS_ERROR4                   100254
#define GLUES_NURBS_ERROR5                   100255
#define GLUES_NURBS_ERROR6                   100256
#define GLUES_NURBS_ERROR7                   100257
#define GLUES_NURBS_ERROR8                   100258
#define GLUES_NURBS_ERROR9                   100259
#define GLUES_NURBS_ERROR10                  100260
#define GLUES_NURBS_ERROR11                  100261
#define GLUES_NURBS_ERROR12                  100262
#define GLUES_NURBS_ERROR13                  100263
#define GLUES_NURBS_ERROR14                  100264
#define GLUES_NURBS_ERROR15                  100265
#define GLUES_NURBS_ERROR16                  100266
#define GLUES_NURBS_ERROR17                  100267
#define GLUES_NURBS_ERROR18                  100268
#define GLUES_NURBS_ERROR19                  100269
#define GLUES_NURBS_ERROR20                  100270
#define GLUES_NURBS_ERROR21                  100271
#define GLUES_NURBS_ERROR22                  100272
#define GLUES_NURBS_ERROR23                  100273
#define GLUES_NURBS_ERROR24                  100274
#define GLUES_NURBS_ERROR25                  100275
#define GLUES_NURBS_ERROR26                  100276
#define GLUES_NURBS_ERROR27                  100277
#define GLUES_NURBS_ERROR28                  100278
#define GLUES_NURBS_ERROR29                  100279
#define GLUES_NURBS_ERROR30                  100280
#define GLUES_NURBS_ERROR31                  100281
#define GLUES_NURBS_ERROR32                  100282
#define GLUES_NURBS_ERROR33                  100283
#define GLUES_NURBS_ERROR34                  100284
#define GLUES_NURBS_ERROR35                  100285
#define GLUES_NURBS_ERROR36                  100286
#define GLUES_NURBS_ERROR37                  100287

#ifdef __cplusplus
}
#endif

#endif /* __GLUES_REGISTRY_H__ */
