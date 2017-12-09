/*
    Copyright (C) 2017 by Jasem Mutlaq <mutlaqja@ikarustech.com>

    INDI Raw Encoder

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "mjpegencoder.h"
#include "stream/streammanager.h"
#include "indiccd.h"

#include <zlib.h>
#include <jpeglib.h>
#include <jerror.h>

static void init_destination(j_compress_ptr cinfo)
{
    INDI_UNUSED(cinfo);
    /* No work necessary here */
}

static boolean empty_output_buffer(j_compress_ptr cinfo)
{
    INDI_UNUSED(cinfo);
    /* No work necessary here */
    return TRUE;
}

static void term_destination(j_compress_ptr cinfo)
{
    INDI_UNUSED(cinfo);
    /* no work necessary here */
}

namespace INDI
{

MJPEGEncoder::MJPEGEncoder()
{
    name = "MJPEG";    
}

MJPEGEncoder::~MJPEGEncoder()
{
    delete [] jpegBuffer;
}

const char *MJPEGEncoder::getDeviceName()
{
    return currentCCD->getDeviceName();
}

bool MJPEGEncoder::upload(IBLOB *bp, const uint8_t *buffer, uint32_t nbytes, bool isCompressed)
{
    // We do not support compression
    if (isCompressed)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Compression is not supported in MJPEG stream.");
        return false;
    }

    INDI_UNUSED(nbytes);
    int bufsize = rawWidth * rawHeight * ((pixelFormat == INDI_RGB) ? 3 : 1);
    if (bufsize != jpegBufferSize)
    {
        delete [] jpegBuffer;
        jpegBuffer = new uint8_t[bufsize];
        jpegBufferSize = bufsize;
    }

    if (pixelFormat == INDI_RGB)
        jpeg_compress_8u_rgb(buffer, rawWidth, rawHeight, rawWidth*3, jpegBuffer, &bufsize, 70);
    else
        jpeg_compress_8u_gray(buffer, rawWidth, rawHeight, rawWidth, jpegBuffer, &bufsize, 70);

    bp->blob    = jpegBuffer;
    bp->bloblen = bufsize;
    bp->size    = bufsize;
    strcpy(bp->format, ".stream_jpg");

    return true;
}

/*
FROM: https://svn.csail.mit.edu/rrg_pods/jpeg-utils/

Name:         jpeg-utils
Maintainers:  Albert Huang <albert@csail.mit.edu>
Summary:      Wrapper functions around libjpeg to simplify JPEG compression and
              decompression with in-memory buffers.
Description:
  note: This is a simple enough library that you could just copy the .c
  and .h files in to your own programs.

  To use the library, link against -ljpeg-utils, or use
  pkg-config --cflags --libs jpeg-utils

Requirements:
  libjpeg62

  For faster performance, install libjpeg-turbo, an SSE-accelerated
  library that is ABI compatible with libjpeg62.
*/

int MJPEGEncoder::jpeg_compress_8u_gray (const uint8_t * src, uint16_t width, uint16_t height, int stride, uint8_t * dest, int * destsize, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_destination_mgr jdest;
    int out_size = *destsize;

    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_compress (&cinfo);
    jdest.next_output_byte = dest;
    jdest.free_in_buffer = out_size;
    jdest.init_destination = init_destination;
    jdest.empty_output_buffer = empty_output_buffer;
    jdest.term_destination = term_destination;
    cinfo.dest = &jdest;

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;
    jpeg_set_defaults (&cinfo);
    jpeg_set_quality (&cinfo, quality, TRUE);

    jpeg_start_compress (&cinfo, TRUE);
    while (cinfo.next_scanline < height)
    {
        JSAMPROW row = (JSAMPROW)(src + cinfo.next_scanline * stride);
        jpeg_write_scanlines (&cinfo, &row, 1);
    }

    jpeg_finish_compress (&cinfo);
    *destsize = out_size - jdest.free_in_buffer;
    jpeg_destroy_compress (&cinfo);
    return 0;
}

int MJPEGEncoder::jpeg_compress_8u_rgb (const uint8_t * src, uint16_t width, uint16_t height, int stride, uint8_t * dest, int * destsize, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_destination_mgr jdest;
    int out_size = *destsize;

    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_compress (&cinfo);
    jdest.next_output_byte = dest;
    jdest.free_in_buffer = out_size;
    jdest.init_destination = init_destination;
    jdest.empty_output_buffer = empty_output_buffer;
    jdest.term_destination = term_destination;
    cinfo.dest = &jdest;

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults (&cinfo);
    jpeg_set_quality (&cinfo, quality, TRUE);

    jpeg_start_compress (&cinfo, TRUE);
    while (cinfo.next_scanline < height)
    {
        JSAMPROW row = (JSAMPROW)(src + cinfo.next_scanline * stride);
        jpeg_write_scanlines (&cinfo, &row, 1);
    }
    jpeg_finish_compress (&cinfo);
    *destsize = out_size - jdest.free_in_buffer;
    jpeg_destroy_compress (&cinfo);
    return 0;
}

}
