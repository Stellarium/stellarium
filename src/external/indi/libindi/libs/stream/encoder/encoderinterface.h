/*
    Copyright (C) 2017 by Jasem Mutlaq <mutlaqja@ikarustech.com>

    Encoder Interface

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

namespace INDI
{

class CCD;

class EncoderInterface
{
  public:
    EncoderInterface() = default;
    virtual ~EncoderInterface() = default;

    virtual void init(CCD *activeCCD);

    virtual bool setPixelFormat(INDI_PIXEL_FORMAT pixelFormat, uint8_t pixelDepth);

    virtual bool setSize(uint16_t width, uint16_t height);

    virtual bool upload(IBLOB *bp, const uint8_t *buffer, uint32_t nbytes, bool isCompressed=false) = 0;

    const char *getName();

  protected:
    const char *name;
    CCD *currentCCD = nullptr;
    INDI_PIXEL_FORMAT pixelFormat;            // INDI Pixel Format
    uint8_t pixelDepth = 8;                   // Bits per Pixels
    uint16_t rawWidth, rawHeight;
};

}
