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
 * OpenGL ES CM 1.0 port of part of GLU by Mike Gorchak <mike@malva.ua>
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>    /* UINT_MAX */
#include <math.h>

#include "glues_mipmap.h"

typedef union
{
   unsigned char ub[4];
   unsigned short us[2];
   unsigned int ui;
   char b[4];
   short s[2];
   int i;
   float f;
} Type_Widget;

/* Pixel storage modes */
typedef struct
{
   GLint pack_alignment;
   GLint pack_row_length;
   GLint pack_skip_rows;
   GLint pack_skip_pixels;
   GLint pack_swap_bytes;
   GLint pack_image_height;

   GLint unpack_alignment;
   GLint unpack_row_length;
   GLint unpack_skip_rows;
   GLint unpack_skip_pixels;
   GLint unpack_swap_bytes;
   GLint unpack_image_height;
} PixelStorageModes;

static int gluBuild2DMipmapLevelsCore(GLenum, GLint, GLsizei, GLsizei,
                                      GLsizei, GLsizei, GLenum, GLenum,
                                      GLint, GLint, GLint, const void*);

/*
 * internal function declarations
 */
static GLfloat bytes_per_element(GLenum type);
static GLint elements_per_group(GLenum format, GLenum type);
static GLint image_size(GLint width, GLint height, GLenum format, GLenum type);
static void fill_image(const PixelStorageModes*,
                       GLint width, GLint height, GLenum format,
                       GLenum type, GLboolean index_format,
                       const void* userdata, GLushort* newimage);
static void empty_image(const PixelStorageModes*,
                        GLint width, GLint height, GLenum format,
                        GLenum type, GLboolean index_format,
                        const GLushort* oldimage, void* userdata);
static void scale_internal(GLint components, GLint widthin, GLint heightin,
                           const GLushort* datain,
                           GLint widthout, GLint heightout,
                           GLushort* dataout);

static void scale_internal_ubyte(GLint components, GLint widthin,
                           GLint heightin, const GLubyte* datain,
                           GLint widthout, GLint heightout,
                           GLubyte* dataout, GLint element_size,
                           GLint ysize, GLint group_size);

static int checkMipmapArgs(GLenum, GLenum, GLenum);
static GLboolean legalFormat(GLenum);
static GLboolean legalType(GLenum);
static GLboolean isTypePackedPixel(GLenum);
static GLboolean isLegalFormatForPackedPixelType(GLenum, GLenum);
static GLboolean isLegalLevels(GLint, GLint, GLint, GLint);
static void closestFit(GLenum, GLint, GLint, GLint, GLenum, GLenum,
                       GLint*, GLint*);

/* packedpixel type scale routines */
static void extract565(int, const void*, GLfloat []);
static void shove565(const GLfloat [], int ,void*);
static void extract4444(int, const void*, GLfloat []);
static void shove4444(const GLfloat [], int ,void*);
static void extract5551(int, const void*, GLfloat []);
static void shove5551(const GLfloat [], int ,void*);
static void scaleInternalPackedPixel(int,
                                     void (*)(int, const void*,GLfloat []),
                                     void (*)(const GLfloat [],int, void*),
                                     GLint,GLint, const void*,
                                     GLint,GLint,void*,GLint,GLint,GLint);
static void halveImagePackedPixel(int,
                                  void (*)(int, const void*,GLfloat []),
                                  void (*)(const GLfloat [],int, void*),
                                  GLint, GLint, const void*,
                                  void*, GLint, GLint, GLint);
static void halve1DimagePackedPixel(int,
                                    void (*)(int, const void*,GLfloat []),
                                    void (*)(const GLfloat [],int, void*),
                                    GLint, GLint, const void*,
                                    void*, GLint, GLint, GLint);

static void halve1Dimage_ubyte(GLint, GLuint, GLuint,const GLubyte*,
                               GLubyte*, GLint, GLint, GLint);

static void retrieveStoreModes(PixelStorageModes* psm)
{
   psm->unpack_alignment=1;
   psm->unpack_row_length=0;
   psm->unpack_skip_rows=0;
   psm->unpack_skip_pixels=0;
   psm->unpack_swap_bytes=0;

   psm->pack_alignment=1;
   psm->pack_row_length=0;
   psm->pack_skip_rows=0;
   psm->pack_skip_pixels=0;
   psm->pack_swap_bytes=0;
}

static int computeLog(GLuint value)
{
   int i=0;

   /* Error! */
   if (value==0)
   {
      return -1;
   }

   for (;;)
   {
      if (value & 1)
      {
         /* Error ! */
         if (value!=1)
         {
            return -1;
         }
         return i;
      }
      value =value >> 1;
      i++;
   }
}

/*
** Compute the nearest power of 2 number.  This algorithm is a little
** strange, but it works quite well.
*/
static int nearestPower(GLuint value)
{
   int i=1;

   /* Error! */
   if (value==0)
   {
      return -1;
   }

   for (;;)
   {
      if (value==1)
      {
         return i;
      }
      else
      {
         if (value==3)
         {
            return i*4;
         }
      }
      value=value>>1;
      i*=2;
   }
}

#define __GLU_SWAP_2_BYTES(s)\
(GLushort)(((GLushort)((const GLubyte*)(s))[1])<<8 | ((const GLubyte*)(s))[0])

#define __GLU_SWAP_4_BYTES(s)\
(GLuint)(((GLuint)((const GLubyte*)(s))[3])<<24 | \
        ((GLuint)((const GLubyte*)(s))[2])<<16 | \
        ((GLuint)((const GLubyte*)(s))[1])<<8  | ((const GLubyte*)(s))[0])

static void halveImage(GLint components, GLuint width, GLuint height,
                       const GLushort* datain, GLushort* dataout)
{
   int i, j, k;
   int newwidth, newheight;
   int delta;
   GLushort* s;
   const GLushort* t;

   newwidth=width/2;
   newheight=height/2;
   delta=width*components;
   s=dataout;
   t=datain;

   /* Piece o' cake! */
   for (i=0; i<newheight; i++)
   {
      for (j=0; j<newwidth; j++)
      {
         for (k=0; k<components; k++)
         {
            s[0]=(t[0]+t[components]+t[delta]+t[delta+components]+2)/4;
            s++; t++;
         }
         t+=components;
      }
      t+=delta;
    }
}

static void halveImage_ubyte(GLint components, GLuint width, GLuint height,
                        const GLubyte* datain, GLubyte* dataout,
                        GLint element_size, GLint ysize, GLint group_size)
{
   int i, j, k;
   int newwidth, newheight;
   int padBytes;
   GLubyte* s;
   const char* t;

   /* handle case where there is only 1 column/row */
   if (width==1 || height==1)
   {
      assert(!(width==1 && height==1) ); /* can't be 1x1 */
      halve1Dimage_ubyte(components, width, height, datain, dataout, element_size, ysize, group_size);
      return;
   }

   newwidth=width/2;
   newheight=height/2;
   padBytes=ysize-(width*group_size);
   s=dataout;
   t=(const char*)datain;

   /* Piece o' cake! */
   for (i=0; i<newheight; i++)
   {
      for (j=0; j<newwidth; j++)
      {
         for (k=0; k<components; k++)
         {
            s[0]=(*(const GLubyte*)t+
                  *(const GLubyte*)(t+group_size)+
                  *(const GLubyte*)(t+ysize)+
                  *(const GLubyte*)(t+ysize+group_size)+2)/4;
                  s++; t+=element_size;
         }
         t+=group_size;
      }
      t+=padBytes;
      t+=ysize;
   }
}

/* */
static void halve1Dimage_ubyte(GLint components, GLuint width, GLuint height,
                               const GLubyte* dataIn, GLubyte* dataOut,
                               GLint element_size, GLint ysize,
                               GLint group_size)
{
   GLint halfWidth=width/2;
   GLint halfHeight=height/2;
   const char* src=(const char*)dataIn;
   GLubyte* dest=dataOut;
   int jj;

   assert(width==1 || height==1); /* must be 1D */
   assert(width!=height);         /* can't be square */

   if (height==1)
   {                              /* 1 row */
      assert(width!=1);           /* widthxheight can't be 1x1 */
      halfHeight=1;

      for (jj=0; jj<halfWidth; jj++)
      {
         int kk;
         for (kk=0; kk<components; kk++)
         {
            *dest=(*(const GLubyte*)src+
                   *(const GLubyte*)(src+group_size))/2;

            src+=element_size;
            dest++;
         }
         src+=group_size;         /* skip to next 2 */
      }
      {
         int padBytes=ysize-(width*group_size);
         src+=padBytes;           /* for assertion only */
      }
   }
   else
   {
      if (width==1)
      {                           /* 1 column */
         int padBytes=ysize-(width*group_size);
         assert(height!=1);       /* widthxheight can't be 1x1 */
         halfWidth=1;

         /* one vertical column with possible pad bytes per row */
         /* average two at a time */
         for (jj=0; jj<halfHeight; jj++)
         {
            int kk;

            for (kk=0; kk<components; kk++)
            {
               *dest=(*(const GLubyte*)src+*(const GLubyte*)(src+ysize))/2;

               src+=element_size;
               dest++;
            }
            src+=padBytes; /* add pad bytes, if any, to get to end to row */
            src+=ysize;
         }
      }
   }

   assert(src==&((const char*)dataIn)[ysize*height]);
   assert((char*)dest==&((char*)dataOut)[components*element_size*halfWidth*halfHeight]);
} /* halve1Dimage_ubyte() */

static void scale_internal(GLint components, GLint widthin, GLint heightin,
                           const GLushort* datain,
                           GLint widthout, GLint heightout,
                           GLushort* dataout)
{
   float x, lowx, highx, convx, halfconvx;
   float y, lowy, highy, convy, halfconvy;
   float xpercent,ypercent;
   float percent;

   /* Max components in a format is 4, so... */
   float totals[4];
   float area;
   int i,j,k,yint,xint,xindex,yindex;
   int temp;

   if (widthin==widthout*2 && heightin==heightout*2)
   {
      halveImage(components, widthin, heightin, datain, dataout);
      return;
   }

   convy=(float)heightin/heightout;
   convx=(float)widthin/widthout;
   halfconvx=convx/2;
   halfconvy=convy/2;
   for (i=0; i<heightout; i++)
   {
      y=convy*(i+0.5f);
      if (heightin>heightout)
      {
         highy=y+halfconvy;
         lowy=y-halfconvy;
      }
      else
      {
         highy=y+0.5f;
         lowy=y-0.5f;
      }
      for (j=0; j<widthout; j++)
      {
         x=convx*(j+0.5f);
         if (widthin>widthout)
         {
            highx=x+halfconvx;
            lowx=x-halfconvx;
         }
         else
         {
            highx=x+0.5f;
            lowx=x-0.5f;
         }
         /*
         ** Ok, now apply box filter to box that goes from (lowx, lowy)
         ** to (highx, highy) on input data into this pixel on output
         ** data.
         */
         totals[0] = totals[1] = totals[2] = totals[3] = 0.0;
         area = 0.0;

         y=lowy;
         yint=(int)floor(y);
         while (y<highy)
         {
            yindex=(yint+heightin) % heightin;
            if (highy<yint+1)
            {
               ypercent=highy-y;
            }
            else
            {
               ypercent=yint+1-y;
            }

            x=lowx;
            xint=(int)floor(x);

            while (x<highx)
            {
               xindex=(xint+widthin) % widthin;
               if (highx<xint+1)
               {
                  xpercent=highx-x;
               }
               else
               {
                  xpercent=xint+1-x;
               }

               percent=xpercent*ypercent;
               area+=percent;
               temp=(xindex+(yindex*widthin))*components;
               for (k=0; k<components; k++)
               {
                  totals[k]+=datain[temp+k]*percent;
               }

               xint++;
               x=(GLfloat)xint;
            }
            yint++;
            y=(GLfloat)yint;
         }

         temp=(j+(i*widthout))*components;
         for (k=0; k<components; k++)
         {
            /* totals[] should be rounded in the case of enlarging an RGB
             * ramp when the type is 4444
             */
            dataout[temp+k]=(GLushort)((totals[k]+0.5f)/area);
         }
      }
   }
}

static void scale_internal_ubyte(GLint components, GLint widthin,
                                 GLint heightin, const GLubyte* datain,
                                 GLint widthout, GLint heightout,
                                 GLubyte* dataout, GLint element_size,
                                 GLint ysize, GLint group_size)
{
   float convx;
   float convy;
   float percent;

   /* Max components in a format is 4, so... */
   float totals[4];
   float area;
   int i, j, k, xindex;

   const char* temp;
   const char* temp0;
   const char* temp_index;
   int outindex;

   int lowx_int, highx_int, lowy_int, highy_int;
   float x_percent, y_percent;
   float lowx_float, highx_float, lowy_float, highy_float;
   float convy_float, convx_float;
   int convy_int, convx_int;
   int l, m;
   const char* left;
   const char* right;

   if (widthin==widthout*2 && heightin==heightout*2)
   {
      halveImage_ubyte(components, widthin, heightin, (const GLubyte*)datain,
                      (GLubyte*)dataout, element_size, ysize, group_size);
      return;
   }

   convy=(float)heightin/heightout;
   convx=(float)widthin/widthout;
   convy_int=(int)floor(convy);
   convy_float=convy-convy_int;
   convx_int=(int)floor(convx);
   convx_float=convx-convx_int;

   area=convx*convy;

   lowy_int=0;
   lowy_float=0;
   highy_int=convy_int;
   highy_float=convy_float;

   for (i=0; i<heightout; i++)
   {
      /* Clamp here to be sure we don't read beyond input buffer. */
      if (highy_int>=heightin)
      {
         highy_int=heightin-1;
      }
      lowx_int = 0;
      lowx_float = 0;
      highx_int = convx_int;
      highx_float = convx_float;

      for (j=0; j<widthout; j++)
      {
         /*
         ** Ok, now apply box filter to box that goes from (lowx, lowy)
         ** to (highx, highy) on input data into this pixel on output
         ** data.
         */
         totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

         /* calculate the value for pixels in the 1st row */
         xindex=lowx_int*group_size;
         if ((highy_int>lowy_int) && (highx_int>lowx_int))
         {
            y_percent=1-lowy_float;
            temp=(const char*)datain+xindex+lowy_int*ysize;
            percent=y_percent*(1-lowx_float);
            for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
            {
               totals[k]+=(GLubyte)(*(temp_index))*percent;
            }

            left=temp;
            for(l=lowx_int+1; l<highx_int; l++)
            {
               temp+=group_size;
               for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
               {
                  totals[k]+=(GLubyte)(*(temp_index))*y_percent;
               }
            }

            temp+=group_size;
            right=temp;
            percent=y_percent*highx_float;
            for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
            {
               totals[k]+=(GLubyte)(*(temp_index))*percent;
            }

            /* calculate the value for pixels in the last row */
            y_percent=highy_float;
            percent=y_percent*(1-lowx_float);
            temp=(const char*)datain+xindex+highy_int*ysize;
            for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
            {
               totals[k]+=(GLubyte)(*(temp_index))*percent;
            }
            for (l=lowx_int+1; l<highx_int; l++)
            {
               temp+=group_size;
               for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
               {
                  totals[k]+=(GLubyte)(*(temp_index))*y_percent;
               }
            }
            temp+=group_size;
            percent=y_percent*highx_float;
            for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
            {
               totals[k]+=(GLubyte)(*(temp_index))*percent;
            }

            /* calculate the value for pixels in the 1st and last column */
            for (m=lowy_int+1; m<highy_int; m++)
            {
               left+=ysize;
               right+=ysize;
               for (k=0; k<components; k++, left+=element_size, right+=element_size)
               {
                  totals[k]+=(GLubyte)(*(left))*(1-lowx_float)+(GLubyte)(*(right))*highx_float;
               }
            }
         }
         else
         {
            if (highy_int>lowy_int)
            {
               x_percent=highx_float-lowx_float;
               percent=(1-lowy_float)*x_percent;
               temp=(const char*)datain+xindex+lowy_int*ysize;
               for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
               {
                  totals[k]+=(GLubyte)(*(temp_index))*percent;
               }
               for (m=lowy_int+1; m<highy_int; m++)
               {
                  temp+=ysize;
                  for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
                  {
                     totals[k]+=(GLubyte)(*(temp_index))*x_percent;
                  }
               }
               percent = x_percent * highy_float;
               temp+=ysize;
               for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
               {
                  totals[k]+=(GLubyte)(*(temp_index))*percent;
               }
            }
            else
            {
               if (highx_int>lowx_int)
               {
                  y_percent=highy_float-lowy_float;
                  percent=(1-lowx_float)*y_percent;
                  temp=(const char*)datain+xindex+lowy_int*ysize;
                  for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
                  {
                     totals[k]+=(GLubyte)(*(temp_index))*percent;
                  }
                  for (l=lowx_int+1; l<highx_int; l++)
                  {
                     temp+=group_size;
                     for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
                     {
                        totals[k]+=(GLubyte)(*(temp_index))*y_percent;
                     }
                  }
                  temp+=group_size;
                  percent=y_percent*highx_float;
                  for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
                  {
                     totals[k] += (GLubyte)(*(temp_index)) * percent;
                  }
               }
               else
               {
                  percent=(highy_float-lowy_float)*(highx_float-lowx_float);
                  temp=(const char*)datain+xindex+lowy_int*ysize;
                  for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
                  {
                     totals[k]+=(GLubyte)(*(temp_index))*percent;
                  }
               }
            }
         }

         /* this is for the pixels in the body */
         temp0=(const char*)datain+xindex+group_size+(lowy_int+1)*ysize;
         for (m=lowy_int+1; m<highy_int; m++)
         {
            temp=temp0;
            for(l=lowx_int+1; l<highx_int; l++)
            {
               for (k=0, temp_index=temp; k<components; k++, temp_index+=element_size)
               {
                  totals[k]+=(GLubyte)(*(temp_index));
               }
               temp+=group_size;
            }
            temp0 += ysize;
         }

         outindex=(j+(i*widthout))*components;
         for (k=0; k<components; k++)
         {
            dataout[outindex+k]=(GLubyte)(totals[k]/area);
         }
         lowx_int=highx_int;
         lowx_float=highx_float;
         highx_int += convx_int;
         highx_float += convx_float;
         if (highx_float>1)
         {
            highx_float-=1.0;
            highx_int++;
         }
      }
      lowy_int=highy_int;
      lowy_float=highy_float;
      highy_int+=convy_int;
      highy_float+=convy_float;
      if (highy_float>1)
      {
         highy_float-=1.0;
         highy_int++;
      }
   }
}

static int checkMipmapArgs(GLenum internalFormat, GLenum format, GLenum type)
{
   if (!legalFormat(format) || !legalType(type))
   {
      return GLU_INVALID_ENUM;
   }

   if (!isLegalFormatForPackedPixelType(format, type))
   {
      return GLU_INVALID_OPERATION;
   }

   return 0;
} /* checkMipmapArgs() */

static GLboolean legalFormat(GLenum format)
{
   switch(format)
   {
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
           return GL_TRUE;
      default:
           return GL_FALSE;
   }
}

static GLboolean legalType(GLenum type)
{
   switch(type)
   {
      case GL_UNSIGNED_BYTE:
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
           return GL_TRUE;
      default:
           return GL_FALSE;
   }
}

/* */
static GLboolean isTypePackedPixel(GLenum type)
{
   assert(legalType(type));

   if (type==GL_UNSIGNED_SHORT_5_6_5 ||
       type==GL_UNSIGNED_SHORT_4_4_4_4 ||
       type==GL_UNSIGNED_SHORT_5_5_5_1)
   {
      return 1;
   }
   else
   {
      return 0;
   }
} /* isTypePackedPixel() */

/* Determines if the packed pixel type is compatible with the format */
static GLboolean isLegalFormatForPackedPixelType(GLenum format, GLenum type)
{
   /* if not a packed pixel type then return true */
   if (!isTypePackedPixel(type))
   {
      return GL_TRUE;
   }

   /* 5_6_5 is only compatible with RGB */
   if ((type==GL_UNSIGNED_SHORT_5_6_5) && format!=GL_RGB)
   {
      return GL_FALSE;
   }

   /* 4_4_4_4 & 5_5_5_1
    * are only compatible with RGBA
    */
   if ((type==GL_UNSIGNED_SHORT_4_4_4_4 || type==GL_UNSIGNED_SHORT_5_5_5_1) &&
       (format != GL_RGBA))
   {
      return GL_FALSE;
   }

   return GL_TRUE;
} /* isLegalFormatForPackedPixelType() */

static GLboolean isLegalLevels(GLint userLevel,GLint baseLevel,GLint maxLevel,
                               GLint totalLevels)
{
   if (baseLevel < 0 || baseLevel < userLevel || maxLevel < baseLevel ||
       totalLevels < maxLevel)
   {
      return GL_FALSE;
   }
   else
   {
      return GL_TRUE;
   }
} /* isLegalLevels() */

/* Given user requested texture size, determine if it fits. If it
 * doesn't then halve both sides and make the determination again
 * until it does fit (for IR only).
 */
static void closestFit(GLenum target, GLint width, GLint height,
                       GLint internalFormat, GLenum format, GLenum type,
                       GLint *newWidth, GLint *newHeight)
{
   GLint maxsize;

   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);

   /* clamp user's texture sizes to maximum sizes, if necessary */
   *newWidth=nearestPower(width);
   if (*newWidth>maxsize)
   {
      *newWidth=maxsize;
   }
   *newHeight=nearestPower(height);
   if (*newHeight>maxsize)
   {
      *newHeight = maxsize;
   }
} /* closestFit() */

GLAPI GLint APIENTRY 
gluScaleImage(GLenum format, GLsizei widthin, GLsizei heightin,
              GLenum typein, const void* datain, GLsizei widthout,
              GLsizei heightout, GLenum typeout, void* dataout)
{
   int components;
   GLushort* beforeImage=NULL;
   GLushort* afterImage=NULL;
   PixelStorageModes psm;

   if (widthin==0 || heightin==0 || widthout==0 || heightout==0)
   {
      return 0;
   }

   if (widthin<0 || heightin<0 || widthout<0 || heightout<0)
   {
      return GLU_INVALID_VALUE;
   }

   if (!legalFormat(format) || !legalType(typein) || !legalType(typeout))
   {
      return GLU_INVALID_ENUM;
   }
   if (!isLegalFormatForPackedPixelType(format, typein))
   {
      return GLU_INVALID_OPERATION;
   }
   if (!isLegalFormatForPackedPixelType(format, typeout))
   {
      return GLU_INVALID_OPERATION;
   }
   beforeImage=malloc(image_size(widthin, heightin, format, GL_UNSIGNED_SHORT));
   afterImage=malloc(image_size(widthout, heightout, format, GL_UNSIGNED_SHORT));
   if (beforeImage==NULL || afterImage==NULL)
   {
      if (beforeImage!=NULL)
      {
         free(beforeImage);
      }
      if (afterImage!=NULL)
      {
         free(afterImage);
      }
      return GLU_OUT_OF_MEMORY;
   }

   retrieveStoreModes(&psm);
   fill_image(&psm,widthin, heightin, format, typein, 0, datain, beforeImage);
   components=elements_per_group(format, 0);
   scale_internal(components, widthin, heightin, beforeImage, widthout, heightout, afterImage);
   empty_image(&psm, widthout, heightout, format, typeout, 0, afterImage, dataout);
   free((GLbyte*)beforeImage);
   free((GLbyte*)afterImage);

   return 0;
}

/* To make swapping images less error prone */
#define __GLU_INIT_SWAP_IMAGE void* tmpImage
#define __GLU_SWAP_IMAGE(a,b) tmpImage=a; a=b; b=tmpImage;

static int gluBuild2DMipmapLevelsCore(GLenum target, GLint internalFormat,
                                      GLsizei width, GLsizei height,
                                      GLsizei widthPowerOf2,
                                      GLsizei heightPowerOf2,
                                      GLenum format, GLenum type,
                                      GLint userLevel,
                                      GLint baseLevel,GLint maxLevel,
                                      const void* data)
{
   GLint newwidth, newheight;
   GLint level, levels;
   const void* usersImage;     /* passed from user. Don't touch! */
   void* srcImage;
   void* dstImage;             /* scratch area to build mipmapped images */
   __GLU_INIT_SWAP_IMAGE;
   GLint memreq;
   GLint cmpts;

   GLint myswap_bytes, groups_per_line, element_size, group_size;
   GLint rowsize, padding;
   PixelStorageModes psm;

   assert(checkMipmapArgs(internalFormat,format,type)==0);
   assert(width>=1 && height>=1);

   srcImage=dstImage=NULL;

   newwidth=widthPowerOf2;
   newheight=heightPowerOf2;
   levels=computeLog(newwidth);
   level=computeLog(newheight);
   if (level>levels)
   {
      levels=level;
   }

   levels+=userLevel;

   retrieveStoreModes(&psm);
   myswap_bytes=psm.unpack_swap_bytes;
   cmpts=elements_per_group(format,type);
   if (psm.unpack_row_length>0)
   {
      groups_per_line=psm.unpack_row_length;
   }
   else
   {
      groups_per_line=width;
   }

   element_size=(GLint)bytes_per_element(type);
   group_size=element_size*cmpts;
   if (element_size==1)
   {
      /* Nothing to swap */
      myswap_bytes=0;
   }

   rowsize=groups_per_line*group_size;
   padding=(rowsize%psm.unpack_alignment);
   if (padding)
   {
      rowsize+=psm.unpack_alignment-padding;
   }
   usersImage=(const GLubyte*)data+psm.unpack_skip_rows*rowsize+psm.unpack_skip_pixels*group_size;

   level=userLevel;

   /* already power-of-two square */
   if (width==newwidth && height==newheight)
   {
      /* Use usersImage for level userLevel */
      if (baseLevel<=level && level<=maxLevel)
      {
         glTexImage2D(target, level, internalFormat, width,
                      height, 0, format, type, usersImage);
      }
      if (levels==0)
      {
         /* we're done. clean up and return */
         glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
         return 0;
      }

      {
         int nextWidth=newwidth/2;
         int nextHeight=newheight/2;

         /* clamp to 1 */
         if (nextWidth<1)
         {
            nextWidth=1;
         }
         if (nextHeight<1)
         {
            nextHeight=1;
         }
         memreq=image_size(nextWidth, nextHeight, format, type);
      }

      switch(type)
      {
         case GL_UNSIGNED_BYTE:
              dstImage = (GLubyte *)malloc(memreq);
              break;
         case GL_UNSIGNED_SHORT_5_6_5:
         case GL_UNSIGNED_SHORT_4_4_4_4:
         case GL_UNSIGNED_SHORT_5_5_5_1:
              dstImage = (GLushort *)malloc(memreq);
              break;
         default:
              return GLU_INVALID_ENUM;
      }
      if (dstImage==NULL)
      {
         glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
         return GLU_OUT_OF_MEMORY;
      }
      else
      {
         switch(type)
         {
            case GL_UNSIGNED_BYTE:
                 halveImage_ubyte(cmpts, width, height, (const GLubyte*)usersImage,
                                 (GLubyte*)dstImage, element_size, rowsize, group_size);
                 break;
            case GL_UNSIGNED_SHORT_5_6_5:
                 halveImagePackedPixel(3, extract565, shove565, width, height,
                                       usersImage, dstImage, element_size,
                                       rowsize, myswap_bytes);
                 break;
            case GL_UNSIGNED_SHORT_4_4_4_4:
                 halveImagePackedPixel(4, extract4444, shove4444, width, height,
                                       usersImage, dstImage, element_size,
                                       rowsize, myswap_bytes);
                 break;
            case GL_UNSIGNED_SHORT_5_5_5_1:
                 halveImagePackedPixel(4, extract5551, shove5551, width, height,
                                       usersImage, dstImage, element_size,
                                       rowsize, myswap_bytes);
                 break;
            default:
                 assert(0);
                 break;
         }
      }

      newwidth=width/2;
      newheight=height/2;

      /* clamp to 1 */
      if (newwidth<1)
      {
         newwidth=1;
      }
      if (newheight<1)
      {
         newheight=1;
      }

      myswap_bytes=0;
      rowsize=newwidth*group_size;
      memreq=image_size(newwidth, newheight, format, type);
      /* Swap srcImage and dstImage */
      __GLU_SWAP_IMAGE(srcImage,dstImage);
      switch(type)
      {
         case GL_UNSIGNED_BYTE:
              dstImage=(GLubyte*)malloc(memreq);
              break;
         case GL_UNSIGNED_SHORT_5_6_5:
         case GL_UNSIGNED_SHORT_4_4_4_4:
         case GL_UNSIGNED_SHORT_5_5_5_1:
              dstImage=(GLushort*)malloc(memreq);
              break;
         default:
              return GLU_INVALID_ENUM;
      }
      if (dstImage==NULL)
      {
         glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
         return GLU_OUT_OF_MEMORY;
      }

      /* level userLevel+1 is in srcImage; level userLevel already saved */
      level = userLevel+1;
   }
   else
   {
      /* user's image is *not* nice power-of-2 sized square */
      memreq=image_size(newwidth, newheight, format, type);
      switch(type)
      {
         case GL_UNSIGNED_BYTE:
              dstImage=(GLubyte*)malloc(memreq);
              break;
         case GL_UNSIGNED_SHORT_5_6_5:
         case GL_UNSIGNED_SHORT_4_4_4_4:
         case GL_UNSIGNED_SHORT_5_5_5_1:
              dstImage=(GLushort*)malloc(memreq);
              break;
         default:
              return GLU_INVALID_ENUM;
      }

      if (dstImage==NULL)
      {
         glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
         return GLU_OUT_OF_MEMORY;
      }

      switch(type)
      {
         case GL_UNSIGNED_BYTE:
              scale_internal_ubyte(cmpts, width, height,
                 (const GLubyte*)usersImage, newwidth, newheight,
                 (GLubyte*)dstImage, element_size, rowsize, group_size);
              break;
         case GL_UNSIGNED_SHORT_5_6_5:
              scaleInternalPackedPixel(3, extract565, shove565, width, height,
                 usersImage, newwidth, newheight, (void*)dstImage, element_size,
                 rowsize, myswap_bytes);
              break;
         case GL_UNSIGNED_SHORT_4_4_4_4:
              scaleInternalPackedPixel(4, extract4444, shove4444, width, height,
                 usersImage, newwidth, newheight, (void*)dstImage, element_size,
                 rowsize,myswap_bytes);
              break;
         case GL_UNSIGNED_SHORT_5_5_5_1:
              scaleInternalPackedPixel(4,extract5551, shove5551, width, height,
                 usersImage, newwidth, newheight, (void*)dstImage, element_size,
                 rowsize, myswap_bytes);
              break;
         default:
              assert(0);
              break;
      }
      myswap_bytes=0;
      rowsize=newwidth*group_size;

      /* Swap dstImage and srcImage */
      __GLU_SWAP_IMAGE(srcImage,dstImage);

      /* use as little memory as possible */
      if (levels!=0)
      {
         {
            int nextWidth=newwidth/2;
            int nextHeight=newheight/2;

            if (nextWidth<1)
            {
               nextWidth=1;
            }
            if (nextHeight<1)
            {
               nextHeight=1;
            }

            memreq=image_size(nextWidth, nextHeight, format, type);
         }

         switch(type)
         {
            case GL_UNSIGNED_BYTE:
                 dstImage = (GLubyte *)malloc(memreq);
                 break;
            case GL_UNSIGNED_SHORT_5_6_5:
            case GL_UNSIGNED_SHORT_4_4_4_4:
            case GL_UNSIGNED_SHORT_5_5_5_1:
                 dstImage = (GLushort *)malloc(memreq);
                 break;
            default:
                 return GLU_INVALID_ENUM;
         }
         if (dstImage==NULL)
         {
            glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
            return GLU_OUT_OF_MEMORY;
         }
      }

      /* level userLevel is in srcImage; nothing saved yet */
      level=userLevel;
   }

   if (baseLevel<=level && level<=maxLevel)
   {
      glTexImage2D(target, level, internalFormat, newwidth, newheight, 0,
                   format, type, (void*)srcImage);
   }

   level++; /* update current level for the loop */
   for (; level<=levels; level++)
   {
      switch(type)
      {
         case GL_UNSIGNED_BYTE:
              halveImage_ubyte(cmpts, newwidth, newheight, (GLubyte*)srcImage,
                 (GLubyte *)dstImage, element_size, rowsize, group_size);
              break;
         case GL_UNSIGNED_SHORT_5_6_5:
              halveImagePackedPixel(3, extract565, shove565, newwidth,
                 newheight, srcImage, dstImage, element_size, rowsize,
                 myswap_bytes);
              break;
         case GL_UNSIGNED_SHORT_4_4_4_4:
              halveImagePackedPixel(4, extract4444, shove4444, newwidth,
                 newheight, srcImage, dstImage, element_size, rowsize,
                 myswap_bytes);
              break;
         case GL_UNSIGNED_SHORT_5_5_5_1:
              halveImagePackedPixel(4, extract5551, shove5551, newwidth,
                 newheight, srcImage, dstImage, element_size, rowsize,
                 myswap_bytes);
              break;
         default:
              assert(0);
              break;
      }

      __GLU_SWAP_IMAGE(srcImage,dstImage);

      if (newwidth>1)
      {
         newwidth/=2;
         rowsize/=2;
      }
      if (newheight>1)
      {
         newheight/=2;
      }

      {
         /* compute amount to pad per row, if any */
         int rowPad=rowsize%psm.unpack_alignment;

         /* should row be padded? */
         if (rowPad == 0)
         {  /* nope, row should not be padded */
            /* call tex image with srcImage untouched since it's not padded */
            if (baseLevel<=level && level<=maxLevel)
            {
               glTexImage2D(target, level, internalFormat, newwidth, newheight,
                            0, format, type, (void*) srcImage);
            }
         }
         else
         {  /* yes, row should be padded */
            /* compute length of new row in bytes, including padding */
            int newRowLength=rowsize+psm.unpack_alignment-rowPad;
            int ii;
            unsigned char* dstTrav;
            unsigned char* srcTrav; /* indices for copying */

            /* allocate new image for mipmap of size newRowLength x newheight */
            void* newMipmapImage=malloc((size_t)(newRowLength*newheight));
            if (newMipmapImage==NULL)
            {
               /* out of memory so return */
               glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
               return GLU_OUT_OF_MEMORY;
            }

            /* copy image from srcImage into newMipmapImage by rows */
            for (ii=0,
                 dstTrav=(unsigned char*) newMipmapImage,
                 srcTrav=(unsigned char*) srcImage;
                 ii<newheight;
                 ii++, dstTrav+= newRowLength, /* make sure the correct distance... */
                 srcTrav+= rowsize)
            {  /* ...is skipped */
               memcpy(dstTrav, srcTrav, rowsize);
               /* note that the pad bytes are not visited and will contain
                * garbage, which is ok.
                */
            }
            /* ...and use this new image for mipmapping instead */
            if (baseLevel<=level && level<=maxLevel)
            {
               glTexImage2D(target, level, internalFormat, newwidth, newheight,
                            0, format, type, newMipmapImage);
            }
            free(newMipmapImage); /* don't forget to free it! */
         }
      }
   } /* for level */

   glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);

   free(srcImage); /*if you get to here, a srcImage has always been malloc'ed*/

   if (dstImage)
   { /* if it's non-rectangular and only 1 level */
      free(dstImage);
   }

   return 0;
} /* gluBuild2DMipmapLevelsCore() */

GLAPI GLint APIENTRY
gluBuild2DMipmapLevels(GLenum target, GLint internalFormat,
                       GLsizei width, GLsizei height, GLenum format,
                       GLenum type, GLint userLevel, GLint baseLevel,
                       GLint maxLevel, const void* data)
{
   int level, levels;

   int rc=checkMipmapArgs(internalFormat,format,type);

   if (rc!=0)
   {
      return rc;
   }

   if (width<1 || height<1)
   {
      return GLU_INVALID_VALUE;
   }

   levels=computeLog(width);
   level=computeLog(height);
   if (level>levels)
   {
      levels=level;
   }

   levels+=userLevel;
   if (!isLegalLevels(userLevel, baseLevel, maxLevel, levels))
   {
      return GLU_INVALID_VALUE;
   }

   return gluBuild2DMipmapLevelsCore(target, internalFormat, width, height,
                                     width, height, format, type,
                                     userLevel, baseLevel, maxLevel, data);
} /* gluBuild2DMipmapLevels() */

GLAPI GLint APIENTRY
gluBuild2DMipmaps(GLenum target, GLint internalFormat, GLsizei width,
                  GLsizei height, GLenum format, GLenum type, const void* data)
{
   GLint widthPowerOf2, heightPowerOf2;
   int level, levels;

   int rc=checkMipmapArgs(internalFormat,format,type);
   if (rc!=0)
   {
      return rc;
   }

   if (width<1 || height<1)
   {
      return GLU_INVALID_VALUE;
   }

   closestFit(target, width, height, internalFormat, format, type,
              &widthPowerOf2,&heightPowerOf2);

   levels=computeLog(widthPowerOf2);
   level=computeLog(heightPowerOf2);
   if (level>levels)
   {
      levels=level;
   }

   return gluBuild2DMipmapLevelsCore(target,internalFormat, width, height,
                                     widthPowerOf2, heightPowerOf2, format,
                                     type, 0, 0, levels, data);
}  /* gluBuild2DMipmaps() */

/*
 * Utility Routines
 */
static GLint elements_per_group(GLenum format, GLenum type)
{
   /*
    * Return the number of elements per group of a specified format
    */

   /* If the type is packedpixels then answer is 1 (ignore format) */
   if (type==GL_UNSIGNED_SHORT_5_6_5 ||
       type==GL_UNSIGNED_SHORT_4_4_4_4 ||
       type==GL_UNSIGNED_SHORT_5_5_5_1)
   {
      return 1;
   }

   /* Types are not packed pixels, so get elements per group */
   switch(format)
   {
      case GL_RGB:
           return 3;
      case GL_LUMINANCE_ALPHA:
           return 2;
      case GL_RGBA:
           return 4;
      default:
           return 1;
   }
}

static GLfloat bytes_per_element(GLenum type)
{
   /*
    * Return the number of bytes per element, based on the element type
    */
   switch(type)
   {
      case GL_UNSIGNED_BYTE:
           return(sizeof(GLubyte));
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
           return(sizeof(GLushort));
      default:
           return 4;
   }
}

/*
** Compute memory required for internal packed array of data of given type
** and format.
*/
static GLint image_size(GLint width, GLint height, GLenum format, GLenum type)
{
   int bytes_per_row;
   int components;

   assert(width>0);
   assert(height>0);
   components=elements_per_group(format, type);

   bytes_per_row=(int)(bytes_per_element(type)*width);

   return bytes_per_row*height*components;
}

/*
** Extract array from user's data applying all pixel store modes.
** The internal format used is an array of unsigned shorts.
*/
static void fill_image(const PixelStorageModes* psm,
                       GLint width, GLint height, GLenum format,
                       GLenum type, GLboolean index_format,
                       const void* userdata, GLushort* newimage)
{
   GLint components;
   GLint element_size;
   GLint rowsize;
   GLint padding;
   GLint groups_per_line;
   GLint group_size;
   GLint elements_per_line;
   const GLubyte* start;
   const GLubyte* iter;
   GLushort* iter2;
   GLint i, j, k;
   GLint myswap_bytes;

   myswap_bytes=psm->unpack_swap_bytes;
   components=elements_per_group(format,type);
   if (psm->unpack_row_length>0)
   {
      groups_per_line = psm->unpack_row_length;
   }
   else
   {
      groups_per_line = width;
   }

   {
      element_size=(GLint)bytes_per_element(type);
      group_size=element_size*components;
      if (element_size==1)
      {
         myswap_bytes=0;
      }

      rowsize=groups_per_line*group_size;
      padding=(rowsize%psm->unpack_alignment);
      if (padding)
      {
         rowsize+=psm->unpack_alignment-padding;
      }
      start=(const GLubyte*)userdata+psm->unpack_skip_rows*rowsize+
                            psm->unpack_skip_pixels*group_size;
      elements_per_line=width*components;

      iter2=newimage;
      for (i=0; i<height; i++)
      {
         iter=start;
         for (j=0; j<elements_per_line; j++)
         {
            float extractComponents[4];

            switch (type)
            {
               case GL_UNSIGNED_BYTE:
                    *iter2++=(*iter)*257;
                    break;
               case GL_UNSIGNED_SHORT_5_6_5:
                    extract565(myswap_bytes, iter, extractComponents);
                    for (k=0; k<3; k++)
                    {
                       *iter2++=(GLushort)(extractComponents[k]*65535);
                    }
                    break;
               case GL_UNSIGNED_SHORT_4_4_4_4:
                    extract4444(myswap_bytes, iter, extractComponents);
                    for (k=0; k<4; k++)
                    {
                       *iter2++=(GLushort)(extractComponents[k]*65535);
                    }
                    break;
               case GL_UNSIGNED_SHORT_5_5_5_1:
                    extract5551(myswap_bytes, iter, extractComponents);
                    for (k = 0; k < 4; k++)
                    {
                       *iter2++=(GLushort)(extractComponents[k]*65535);
                    }
                    break;
            }
            iter+=element_size;
         } /* for j */
         start+=rowsize;
#if 1
         /* want 'iter' pointing at start, not within, row for assertion
          * purposes
          */
         iter= start;
#endif
      } /* for i */

      /* iterators should be one byte past end */
      if (!isTypePackedPixel(type))
      {
         assert(iter2==&newimage[width*height*components]);
      }
      else
      {
         assert(iter2==&newimage[width*height*elements_per_group(format, 0)]);
      }
      assert(iter==&((const GLubyte*)userdata)[rowsize*height+
             psm->unpack_skip_rows*rowsize+psm->unpack_skip_pixels*group_size]);

   }
}

/*
** Insert array into user's data applying all pixel store modes.
** The internal format is an array of unsigned shorts.
** empty_image() because it is the opposite of fill_image().
*/
static void empty_image(const PixelStorageModes* psm, GLint width,
                        GLint height, GLenum format, GLenum type,
                        GLboolean index_format, const GLushort* oldimage,
                        void* userdata)
{
   GLint components;
   GLint element_size;
   GLint rowsize;
   GLint padding;
   GLint groups_per_line;
   GLint group_size;
   GLint elements_per_line;
   GLubyte* start;
   GLubyte* iter;
   const GLushort* iter2;
   GLint i, j, k;
   GLint myswap_bytes;

   myswap_bytes=psm->pack_swap_bytes;
   components=elements_per_group(format, type);
   if (psm->pack_row_length>0)
   {
      groups_per_line = psm->pack_row_length;
   }
   else
   {
      groups_per_line = width;
   }

   {
      float shoveComponents[4];

      element_size=(GLint)bytes_per_element(type);
      group_size=element_size*components;
      if (element_size==1)
      {
         myswap_bytes = 0;
      }

      rowsize=groups_per_line*group_size;
      padding=(rowsize%psm->pack_alignment);
      if (padding)
      {
         rowsize+=psm->pack_alignment-padding;
      }
      start=(GLubyte*) userdata+psm->pack_skip_rows*rowsize+psm->pack_skip_pixels*group_size;
      elements_per_line=width*components;

      iter2=oldimage;
      for (i=0; i<height; i++)
      {
         iter=start;
         for (j=0; j<elements_per_line; j++)
         {
            Type_Widget widget;

            switch(type)
            {
               case GL_UNSIGNED_BYTE:
                    *iter=*iter2++>>8;
                    break;
               case GL_UNSIGNED_SHORT_5_6_5:
                    for (k=0; k<3; k++)
                    {
                       shoveComponents[k]=*iter2++/65535.0f;
                    }
                    shove565(shoveComponents, 0, (void*)&widget.us[0]);
                    if (myswap_bytes)
                    {
                       iter[0]=widget.ub[1];
                       iter[1]=widget.ub[0];
                    }
                    else
                    {
                       *(GLushort*)iter=widget.us[0];
                    }
                    break;
               case GL_UNSIGNED_SHORT_4_4_4_4:
                    for (k=0; k<4; k++)
                    {
                       shoveComponents[k]=*iter2++/65535.0f;
                    }
                    shove4444(shoveComponents,0,(void *)&widget.us[0]);
                    if (myswap_bytes)
                    {
                       iter[0]=widget.ub[1];
                       iter[1]=widget.ub[0];
                    }
                    else
                    {
                       *(GLushort *)iter = widget.us[0];
                    }
                    break;
               case GL_UNSIGNED_SHORT_5_5_5_1:
                    for (k=0; k<4; k++)
                    {
                       shoveComponents[k]=*iter2++/65535.0f;
                    }
                    shove5551(shoveComponents,0,(void *)&widget.us[0]);
                    if (myswap_bytes)
                    {
                       iter[0]=widget.ub[1];
                       iter[1]=widget.ub[0];
                    }
                    else
                    {
                       *(GLushort*)iter=widget.us[0];
                    }
                    break;
            }
            iter+=element_size;
         } /* for j */
         start+=rowsize;
#if 1
         /* want 'iter' pointing at start, not within, row for assertion
          * purposes
          */
         iter=start;
#endif
      } /* for i */

      /* iterators should be one byte past end */
      if (!isTypePackedPixel(type))
      {
         assert(iter2==&oldimage[width*height*components]);
      }
      else
      {
         assert(iter2==&oldimage[width*height*elements_per_group(format, 0)]);
      }
      assert(iter==&((GLubyte *)userdata)[rowsize*height+psm->pack_skip_rows*rowsize+
                   psm->pack_skip_pixels*group_size]);

   }
} /* empty_image() */

/*--------------------------------------------------------------------------
 * Decimation of packed pixel types
 *--------------------------------------------------------------------------
 */
static void extract565(int isSwap, const void* packedPixel, GLfloat extractComponents[])
{
   GLushort ushort=*(const GLushort*)packedPixel;

   if (isSwap)
   {
      ushort=__GLU_SWAP_2_BYTES(packedPixel);
   }
   else
   {
      ushort=*(const GLushort*)packedPixel;
   }

   /* 11111000,00000000 == 0xf800 */
   /* 00000111,11100000 == 0x07e0 */
   /* 00000000,00011111 == 0x001f */

   extractComponents[0]=(float)((ushort&0xf800)>>11)/31.0f; /* 31 = 2^5-1*/
   extractComponents[1]=(float)((ushort&0x07e0)>>5)/63.0f;  /* 63 = 2^6-1*/
   extractComponents[2]=(float)((ushort&0x001f))/31.0f;
} /* extract565() */

static void shove565(const GLfloat shoveComponents[], int index, void* packedPixel)
{
   /* 11111000,00000000 == 0xf800 */
   /* 00000111,11100000 == 0x07e0 */
   /* 00000000,00011111 == 0x001f */

   assert(0.0<=shoveComponents[0] && shoveComponents[0]<=1.0);
   assert(0.0<=shoveComponents[1] && shoveComponents[1]<=1.0);
   assert(0.0<=shoveComponents[2] && shoveComponents[2]<=1.0);

   /* due to limited precision, need to round before shoving */
   ((GLushort*)packedPixel)[index]=((GLushort)((shoveComponents[0]*31)+0.5)<<11)&0xf800;
   ((GLushort*)packedPixel)[index]|=((GLushort)((shoveComponents[1]*63)+0.5)<<5)&0x07e0;
   ((GLushort*)packedPixel)[index]|=((GLushort)((shoveComponents[2]*31)+0.5))&0x001f;
} /* shove565() */

static void extract4444(int isSwap,const void* packedPixel, GLfloat extractComponents[])
{
   GLushort ushort;

   if (isSwap)
   {
      ushort=__GLU_SWAP_2_BYTES(packedPixel);
   }
   else
   {
      ushort=*(const GLushort*)packedPixel;
   }

   /* 11110000,00000000 == 0xf000 */
   /* 00001111,00000000 == 0x0f00 */
   /* 00000000,11110000 == 0x00f0 */
   /* 00000000,00001111 == 0x000f */

   extractComponents[0]=(float)((ushort&0xf000)>>12)/15.0f;/* 15=2^4-1 */
   extractComponents[1]=(float)((ushort&0x0f00)>>8)/15.0f;
   extractComponents[2]=(float)((ushort&0x00f0)>>4)/15.0f;
   extractComponents[3]=(float)((ushort&0x000f))/15.0f;
} /* extract4444() */

static void shove4444(const GLfloat shoveComponents[], int index,void* packedPixel)
{
   assert(0.0<=shoveComponents[0] && shoveComponents[0]<=1.0);
   assert(0.0<=shoveComponents[1] && shoveComponents[1]<=1.0);
   assert(0.0<=shoveComponents[2] && shoveComponents[2]<=1.0);
   assert(0.0<=shoveComponents[3] && shoveComponents[3]<=1.0);

   /* due to limited precision, need to round before shoving */
   ((GLushort*)packedPixel)[index]=((GLushort)((shoveComponents[0]*15)+0.5)<<12)&0xf000;
   ((GLushort*)packedPixel)[index]|=((GLushort)((shoveComponents[1]*15)+0.5)<<8)&0x0f00;
   ((GLushort*)packedPixel)[index]|=((GLushort)((shoveComponents[2]*15)+0.5)<<4)&0x00f0;
   ((GLushort*)packedPixel)[index]|=((GLushort)((shoveComponents[3]*15)+0.5))&0x000f;
} /* shove4444() */

static void extract5551(int isSwap,const void* packedPixel, GLfloat extractComponents[])
{
   GLushort ushort;

   if (isSwap)
   {
      ushort=__GLU_SWAP_2_BYTES(packedPixel);
   }
   else
   {
      ushort=*(const GLushort*)packedPixel;
   }

   /* 11111000,00000000 == 0xf800 */
   /* 00000111,11000000 == 0x07c0 */
   /* 00000000,00111110 == 0x003e */
   /* 00000000,00000001 == 0x0001 */

   extractComponents[0]=(float)((ushort&0xf800)>>11)/31.0f;/* 31 = 2^5-1*/
   extractComponents[1]=(float)((ushort&0x07c0)>>6)/31.0f;
   extractComponents[2]=(float)((ushort&0x003e)>>1)/31.0f;
   extractComponents[3]=(float)((ushort&0x0001));
} /* extract5551() */

static void shove5551(const GLfloat shoveComponents[], int index,void* packedPixel)
{
   /* 11111000,00000000 == 0xf800 */
   /* 00000111,11000000 == 0x07c0 */
   /* 00000000,00111110 == 0x003e */
   /* 00000000,00000001 == 0x0001 */

   assert(0.0<=shoveComponents[0] && shoveComponents[0]<=1.0);
   assert(0.0<=shoveComponents[1] && shoveComponents[1]<=1.0);
   assert(0.0<=shoveComponents[2] && shoveComponents[2]<=1.0);
   assert(0.0<=shoveComponents[3] && shoveComponents[3]<=1.0);

   /* due to limited precision, need to round before shoving */
   ((GLushort*)packedPixel)[index]=((GLushort)((shoveComponents[0]*31)+0.5)<<11)&0xf800;
   ((GLushort*)packedPixel)[index]|=((GLushort)((shoveComponents[1]*31)+0.5)<<6)&0x07c0;
   ((GLushort*)packedPixel)[index]|=((GLushort)((shoveComponents[2]*31)+0.5)<<1)&0x003e;
   ((GLushort*)packedPixel)[index]|=((GLushort)((shoveComponents[3])+0.5))&0x0001;
} /* shove5551() */

static void scaleInternalPackedPixel(int components,
                                     void (*extractPackedPixel)
                                     (int, const void*,GLfloat []),
                                     void (*shovePackedPixel)
                                     (const GLfloat [], int, void*),
                                     GLint widthIn,GLint heightIn,
                                     const void* dataIn,
                                     GLint widthOut,GLint heightOut,
                                     void* dataOut,
                                     GLint pixelSizeInBytes,
                                     GLint rowSizeInBytes, GLint isSwap)
{
   float convx;
   float convy;
   float percent;

   /* Max components in a format is 4, so... */
   float totals[4];
   float extractTotals[4], extractMoreTotals[4], shoveTotals[4];

   float area;
   int i,j,k,xindex;

   const char *temp, *temp0;
   int outindex;

   int lowx_int, highx_int, lowy_int, highy_int;
   float x_percent, y_percent;
   float lowx_float, highx_float, lowy_float, highy_float;
   float convy_float, convx_float;
   int convy_int, convx_int;
   int l, m;
   const char* left;
   const char* right;

   if (widthIn==widthOut*2 && heightIn==heightOut*2)
   {
      halveImagePackedPixel(components,extractPackedPixel,shovePackedPixel,
                            widthIn, heightIn, dataIn, dataOut,
                            pixelSizeInBytes,rowSizeInBytes,isSwap);
      return;
   }
   convy=(float)heightIn/heightOut;
   convx=(float)widthIn/widthOut;
   convy_int=(int)floor(convy);
   convy_float=convy-convy_int;
   convx_int=(int)floor(convx);
   convx_float=convx-convx_int;

   area=convx*convy;

   lowy_int=0;
   lowy_float=0;
   highy_int=convy_int;
   highy_float=convy_float;

   for (i=0; i<heightOut; i++)
   {
      lowx_int=0;
      lowx_float=0;
      highx_int=convx_int;
      highx_float=convx_float;

      for (j=0; j<widthOut; j++)
      {
         /*
         ** Ok, now apply box filter to box that goes from (lowx, lowy)
         ** to (highx, highy) on input data into this pixel on output
         ** data.
         */
         totals[0]=totals[1]=totals[2]=totals[3]=0.0;

         /* calculate the value for pixels in the 1st row */
         xindex=lowx_int*pixelSizeInBytes;
         if ((highy_int>lowy_int) && (highx_int>lowx_int))
         {
            y_percent=1-lowy_float;
            temp=(const char*)dataIn+xindex+lowy_int*rowSizeInBytes;
            percent=y_percent*(1-lowx_float);
            (*extractPackedPixel)(isSwap, temp, extractTotals);
            for (k=0; k<components; k++)
            {
               totals[k]+=extractTotals[k]*percent;
            }
            left=temp;
            for (l=lowx_int+1; l<highx_int; l++)
            {
               temp+=pixelSizeInBytes;
               (*extractPackedPixel)(isSwap, temp, extractTotals);
               for (k=0; k<components; k++)
               {
                  totals[k]+=extractTotals[k]*y_percent;
               }
            }
            temp+=pixelSizeInBytes;
            right=temp;
            percent=y_percent*highx_float;
            (*extractPackedPixel)(isSwap, temp, extractTotals);
            for (k=0; k<components; k++)
            {
               totals[k]+=extractTotals[k]*percent;
            }

            /* calculate the value for pixels in the last row */
            y_percent=highy_float;
            percent=y_percent*(1-lowx_float);
            temp=(const char*)dataIn+xindex+highy_int*rowSizeInBytes;
            (*extractPackedPixel)(isSwap, temp, extractTotals);
            for (k=0; k<components; k++)
            {
               totals[k]+=extractTotals[k]*percent;
            }
            for (l=lowx_int+1; l<highx_int; l++)
            {
               temp+=pixelSizeInBytes;
               (*extractPackedPixel)(isSwap, temp, extractTotals);
               for (k=0; k<components; k++)
               {
                  totals[k]+=extractTotals[k]*y_percent;
               }
            }
            temp+=pixelSizeInBytes;
            percent=y_percent*highx_float;
            (*extractPackedPixel)(isSwap, temp, extractTotals);
            for (k=0; k<components; k++)
            {
               totals[k]+=extractTotals[k]*percent;
            }

            /* calculate the value for pixels in the 1st and last column */
            for (m=lowy_int+1; m<highy_int; m++)
            {
               left+=rowSizeInBytes;
               right+=rowSizeInBytes;
               (*extractPackedPixel)(isSwap, left, extractTotals);
               (*extractPackedPixel)(isSwap, right, extractMoreTotals);
               for (k=0; k<components; k++)
               {
                  totals[k]+=(extractTotals[k]*(1-lowx_float)+extractMoreTotals[k]*highx_float);
               }
            }
         }
         else
         {
            if (highy_int>lowy_int)
            {
               x_percent=highx_float-lowx_float;
               percent=(1-lowy_float)*x_percent;
               temp=(const char*)dataIn+xindex+lowy_int*rowSizeInBytes;
               (*extractPackedPixel)(isSwap, temp, extractTotals);
               for (k=0; k<components; k++)
               {
                  totals[k]+=extractTotals[k]*percent;
               }
               for (m=lowy_int+1; m<highy_int; m++)
               {
                  temp+=rowSizeInBytes;
                  (*extractPackedPixel)(isSwap, temp, extractTotals);
                  for (k=0; k<components; k++)
                  {
                     totals[k]+=extractTotals[k]*x_percent;
                  }
               }
               percent=x_percent*highy_float;
               temp+=rowSizeInBytes;
               (*extractPackedPixel)(isSwap, temp, extractTotals);
               for (k=0; k<components; k++)
               {
                  totals[k]+=extractTotals[k]*percent;
               }
            }
            else
            {
               if (highx_int > lowx_int)
               {
                  y_percent=highy_float-lowy_float;
                  percent=(1-lowx_float)*y_percent;
                  temp=(const char*)dataIn+xindex+lowy_int*rowSizeInBytes;
                  (*extractPackedPixel)(isSwap, temp, extractTotals);
                  for (k=0; k<components; k++)
                  {
                     totals[k]+= extractTotals[k] * percent;
                  }
                  for (l=lowx_int+1; l<highx_int; l++)
                  {
                     temp+=pixelSizeInBytes;
                     (*extractPackedPixel)(isSwap, temp, extractTotals);
                     for (k=0; k<components; k++)
                     {
                        totals[k]+=extractTotals[k]*y_percent;
                     }
                  }
                  temp+=pixelSizeInBytes;
                  percent=y_percent*highx_float;
                  (*extractPackedPixel)(isSwap, temp, extractTotals);
                  for (k=0; k<components; k++)
                  {
                     totals[k]+=extractTotals[k]*percent;
                  }
               }
               else
               {
                  percent=(highy_float-lowy_float)*(highx_float-lowx_float);
                  temp=(const char*)dataIn+xindex+lowy_int*rowSizeInBytes;
                  (*extractPackedPixel)(isSwap, temp, extractTotals);
                  for (k=0; k<components; k++)
                  {
                     totals[k]+=extractTotals[k]*percent;
                  }
               }
            }
         }

         /* this is for the pixels in the body */
         temp0=(const char*)dataIn+xindex+pixelSizeInBytes+(lowy_int+1)*rowSizeInBytes;
         for (m=lowy_int+1; m<highy_int; m++)
         {
            temp=temp0;
            for (l=lowx_int+1; l<highx_int; l++)
            {
               (*extractPackedPixel)(isSwap, temp, extractTotals);
               for (k=0; k<components; k++)
               {
                  totals[k]+=extractTotals[k];
               }
               temp+=pixelSizeInBytes;
            }
            temp0 += rowSizeInBytes;
         }

         outindex=(j+(i*widthOut));
         for (k=0; k<components; k++)
         {
            shoveTotals[k]=totals[k]/area;
         }
         (*shovePackedPixel)(shoveTotals, outindex, (void*)dataOut);
         lowx_int=highx_int;
         lowx_float=highx_float;
         highx_int+=convx_int;
         highx_float+=convx_float;
         if (highx_float>1)
         {
            highx_float-=1.0;
            highx_int++;
         }
      }
      lowy_int=highy_int;
      lowy_float=highy_float;
      highy_int+=convy_int;
      highy_float+=convy_float;

      if (highy_float>1)
      {
         highy_float-=1.0;
         highy_int++;
      }
   }

   assert(outindex==(widthOut*heightOut-1));
} /* scaleInternalPackedPixel() */

/* rowSizeInBytes is at least the width (in bytes) due to padding on
 *  inputs; not always equal. Output NEVER has row padding.
 */
static void halveImagePackedPixel(int components,
                                  void (*extractPackedPixel)
                                  (int, const void*,GLfloat []),
                                  void (*shovePackedPixel)
                                  (const GLfloat [],int, void*),
                                  GLint width, GLint height,
                                  const void* dataIn, void* dataOut,
                                  GLint pixelSizeInBytes,
                                  GLint rowSizeInBytes, GLint isSwap)
{
   /* handle case where there is only 1 column/row */
   if (width==1 || height==1)
   {
      assert(!(width==1 && height==1)); /* can't be 1x1 */
      halve1DimagePackedPixel(components,extractPackedPixel,shovePackedPixel,
                              width,height,dataIn,dataOut,pixelSizeInBytes,
                              rowSizeInBytes,isSwap);
      return;
   }

   {
      int ii, jj;

      int halfWidth=width/2;
      int halfHeight=height/2;
      const char* src=(const char*)dataIn;
      int padBytes=rowSizeInBytes-(width*pixelSizeInBytes);
      int outIndex=0;

      for (ii=0; ii<halfHeight; ii++)
      {
         for (jj=0; jj<halfWidth; jj++)
         {
#define BOX4 4
            float totals[4];              /* 4 is maximum components */
            float extractTotals[BOX4][4]; /* 4 is maximum components */
            int cc;

            (*extractPackedPixel)(isSwap, src, &extractTotals[0][0]);
            (*extractPackedPixel)(isSwap, (src+pixelSizeInBytes), &extractTotals[1][0]);
            (*extractPackedPixel)(isSwap, (src+rowSizeInBytes), &extractTotals[2][0]);
            (*extractPackedPixel)(isSwap, (src+rowSizeInBytes+pixelSizeInBytes), &extractTotals[3][0]);
            for (cc=0; cc<components; cc++)
            {
               int kk;

               /* grab 4 pixels to average */
               totals[cc]=0.0;
               for (kk=0; kk<BOX4; kk++)
               {
                  totals[cc]+=extractTotals[kk][cc];
               }
               totals[cc]/=(float)BOX4;
            }
            (*shovePackedPixel)(totals, outIndex, dataOut);

            outIndex++;
            /* skip over to next square of 4 */
            src+=pixelSizeInBytes+pixelSizeInBytes;
         }
         /* skip past pad bytes, if any, to get to next row */
         src+=padBytes;

         /* src is at beginning of a row here, but it's the second row of
          * the square block of 4 pixels that we just worked on so we
          * need to go one more row.
          * i.e.,
          *                   OO...
          *           here -->OO...
          *       but want -->OO...
          *                   OO...
          *                   ...
          */
         src+=rowSizeInBytes;
      }

      /* both pointers must reach one byte after the end */
      assert(src==&((const char*)dataIn)[rowSizeInBytes*height]);
      assert(outIndex==halfWidth*halfHeight);
   }
} /* halveImagePackedPixel() */

static void halve1DimagePackedPixel(int components,
                                    void (*extractPackedPixel)
                                    (int, const void*,GLfloat []),
                                    void (*shovePackedPixel)
                                    (const GLfloat [],int, void*),
                                    GLint width, GLint height,
                                    const void* dataIn, void* dataOut,
                                    GLint pixelSizeInBytes,
                                    GLint rowSizeInBytes, GLint isSwap)
{
   int halfWidth=width/2;
   int halfHeight=height/2;
   const char *src=(const char*)dataIn;
   int jj;

   assert(width==1 || height==1); /* must be 1D */
   assert(width!= height);        /* can't be square */

   if (height==1)
   {
      /* 1 row */
      int outIndex=0;

      assert(width!=1);           /* widthxheight can't be 1x1 */
      halfHeight=1;

      /* one horizontal row with possible pad bytes */
      for (jj=0; jj<halfWidth; jj++)
      {
#define BOX2 2
         float totals[4];               /* 4 is maximum components */
         float extractTotals[BOX2][4];  /* 4 is maximum components */
         int cc;

         /* average two at a time, instead of four */
         (*extractPackedPixel)(isSwap, src, &extractTotals[0][0]);
         (*extractPackedPixel)(isSwap, (src+pixelSizeInBytes), &extractTotals[1][0]);
         for (cc=0; cc<components; cc++)
         {
            int kk;

            /* grab 2 pixels to average */
            totals[cc]=0.0;
            for (kk=0; kk<BOX2; kk++)
            {
               totals[cc]+=extractTotals[kk][cc];
            }
            totals[cc]/=(float)BOX2;
         }
         (*shovePackedPixel)(totals, outIndex, dataOut);

         outIndex++;
         /* skip over to next group of 2 */
         src+= pixelSizeInBytes + pixelSizeInBytes;
      }

      {
         int padBytes=rowSizeInBytes-(width*pixelSizeInBytes);
         src+=padBytes;                 /* for assertion only */
      }
      assert(src==&((const char*)dataIn)[rowSizeInBytes]);
      assert(outIndex==halfWidth*halfHeight);
   }
   else
   {
      if (width==1)             /* 1 column */
      {
         int outIndex=0;

         assert(height!=1);        /* widthxheight can't be 1x1 */
         halfWidth=1;
         /* one vertical column with possible pad bytes per row */
         /* average two at a time */

         for (jj=0; jj<halfHeight; jj++)
         {
#define BOX2 2
            float totals[4];               /* 4 is maximum components */
            float extractTotals[BOX2][4];  /* 4 is maximum components */
            int cc;

            /* average two at a time, instead of four */
            (*extractPackedPixel)(isSwap, src, &extractTotals[0][0]);
            (*extractPackedPixel)(isSwap, (src+rowSizeInBytes), &extractTotals[1][0]);
            for (cc=0; cc<components; cc++)
            {
               int kk;

               /* grab 2 pixels to average */
               totals[cc]=0.0;
               for (kk=0; kk<BOX2; kk++)
               {
                  totals[cc]+=extractTotals[kk][cc];
               }
               totals[cc]/=(float)BOX2;
            }
            (*shovePackedPixel)(totals, outIndex, dataOut);

            outIndex++;
            src+=rowSizeInBytes+rowSizeInBytes; /* go to row after next */
         }

         assert(src==&((const char*)dataIn)[rowSizeInBytes*height]);
         assert(outIndex==halfWidth*halfHeight);
      }
   }
} /* halve1DimagePackedPixel() */

/*===========================================================================*/
