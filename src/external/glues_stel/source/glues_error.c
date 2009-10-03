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
 */

#include <stdio.h>
#include <stdlib.h>

#include "glues_error.h"

static unsigned char* __gluTessErrors[]=
{
   (unsigned char*) " ",
   (unsigned char*) "gluTessBeginPolygon() must precede a gluTessEndPolygon()",
   (unsigned char*) "gluTessBeginContour() must precede a gluTessEndContour()",
   (unsigned char*) "gluTessEndPolygon() must follow a gluTessBeginPolygon()",
   (unsigned char*) "gluTessEndContour() must follow a gluTessBeginContour()",
   (unsigned char*) "a coordinate is too large",
   (unsigned char*) "need combine callback",
};

const unsigned char* __gluTessErrorString(int errnum)
{
   return __gluTessErrors[errnum];
}

struct token_string
{
   GLuint Token;
   const char* String;
};

static const struct token_string Errors[]=
{
   /* GL */
   {GL_NO_ERROR, "no error"},
   {GL_INVALID_ENUM, "invalid enumerant"},
   {GL_INVALID_VALUE, "invalid value"},
   {GL_INVALID_OPERATION, "invalid operation"},
   {GL_STACK_OVERFLOW, "stack overflow"},
   {GL_STACK_UNDERFLOW, "stack underflow"},
   {GL_OUT_OF_MEMORY, "out of memory"},

   /* GLU */
   { GLU_INVALID_ENUM, "invalid enumerant"},
   { GLU_INVALID_VALUE, "invalid value"},
   { GLU_OUT_OF_MEMORY, "out of memory"},
   { GLU_INCOMPATIBLE_GL_VERSION, "incompatible gl version"},
   { GLU_INVALID_OPERATION, "invalid operation"},
   { ~0, NULL } /* end of list indicator */
};

const GLubyte* gluErrorString(GLenum errorCode)
{
   int i;

   for (i=0; Errors[i].String; i++)
   {
      if (Errors[i].Token==errorCode)
      {
         return (const GLubyte*) Errors[i].String;
      }
   }

   if ((errorCode>=GLU_TESS_ERROR1) && (errorCode<=GLU_TESS_ERROR6))
   {
      return (const GLubyte*) __gluTessErrorString(errorCode-(GLU_TESS_ERROR1-1));
   }

   return (const GLubyte*)0;
}
