/*
    Copyright (C) 2017 by Jasem Mutlaq <mutlaqja@ikarustech.com>

    MJPEG Encoder Interface

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

#include "encoderinterface.h"

namespace INDI
{

class MJPEGEncoder : public EncoderInterface
{
public:
    MJPEGEncoder();
    ~MJPEGEncoder();

    virtual bool upload(IBLOB *bp, const uint8_t *buffer, uint32_t nbytes, bool isCompressed=false) override;

private:
    const char *getDeviceName();
    int jpeg_compress_8u_gray (const uint8_t * src, uint16_t width, uint16_t height, int stride, uint8_t * dest, int * destsize, int quality);
    int jpeg_compress_8u_rgb (const uint8_t * src, uint16_t width, uint16_t height, int stride, uint8_t * dest, int * destsize, int quality);
    uint8_t *jpegBuffer = nullptr;
    uint16_t jpegBufferSize=1;

};

}
