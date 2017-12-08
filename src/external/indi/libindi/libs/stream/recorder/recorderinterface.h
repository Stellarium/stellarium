/*
    Copyright (C) 2014 by geehalel <geehalel@gmail.com>

    V4L2 Record

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

#pragma once

#include "indidevapi.h"
#include "indibasetypes.h"

#include <stdio.h>
#include <cstdlib>
#include <stdint.h>

#include <vector>

#if 0
#ifdef OSX_EMBEDED_MODE
#define v4l2_fourcc(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define V4L2_PIX_FMT_GREY    v4l2_fourcc('G', 'R', 'E', 'Y') /*  8  Greyscale     */
#define V4L2_PIX_FMT_SBGGR8  v4l2_fourcc('B', 'A', '8', '1') /*  8  BGBG.. GRGR.. */
#define V4L2_PIX_FMT_SBGGR16 v4l2_fourcc('B', 'Y', 'R', '2') /* 16  BGBG.. GRGR.. */
#define V4L2_PIX_FMT_SGBRG8  v4l2_fourcc('G', 'B', 'R', 'G') /*  8  GBGB.. RGRG.. */
#define V4L2_PIX_FMT_BGR24   v4l2_fourcc('B', 'G', 'R', '3') /* 24  BGR-8-8-8     */
#define V4L2_PIX_FMT_RGB24   v4l2_fourcc('R', 'G', 'B', '3') /* 24  RGB-8-8-8     */
#define V4L2_PIX_FMT_SRGGB8  v4l2_fourcc('R', 'G', 'G', 'B') /*  8  RGRG.. GBGB.. */
#define V4L2_PIX_FMT_SGRBG8  v4l2_fourcc('G', 'R', 'B', 'G') /*  8  GRGR.. BGBG.. */

#else
#include <linux/videodev2.h>
#endif
#endif

namespace INDI
{
class RecorderInterface
{
  public:
    RecorderInterface() = default;
    virtual ~RecorderInterface() = default;

    virtual const char *getName();
    virtual const char *getExtension() = 0;
    // true when direct encoding of pixel format
    virtual bool setPixelFormat(INDI_PIXEL_FORMAT pixelFormat, uint8_t pixelDepth=8) = 0;
    // set full image size in pixels
    virtual bool setSize(uint16_t width, uint16_t height) = 0;
    // Set FPS
    virtual bool setFPS(float FPS) { m_FPS = FPS; return true; }
    virtual bool open(const char *filename, char *errmsg)                          = 0;
    virtual bool close()                                                           = 0;
    // when frame is in known encoding format
    virtual bool writeFrame(const uint8_t *frame, uint32_t nbytes) = 0;
    // If streaming is enabled, then any subframing is already done by the stream recorder
    // and no need to do any further subframing operations. Otherwise, subframing must be done.
    // This is to reduce process time and save memory for a dedicated subframe buffer
    virtual void setStreamEnabled(bool enable) = 0;

  protected:
    const char *name;
    float m_FPS = 1;
};

}
