/*
    Copyright (C) 2017 by Jasem Mutlaq <mutlaqja@ikarustech.com>
    Copyright (C) 2014 by geehalel <geehalel@gmail.com>

    Encoder Interface.

    Subclass to implement specific recording backend.

    Current supported encoders:
    1. raw INDI encoder (8 bit grayscale and RGB24 color)

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
#include "indiccd.h"

namespace INDI
{

const char *EncoderInterface::getName()
{
    return name;
}

void EncoderInterface::init(CCD *activeCCD)
{
    currentCCD = activeCCD;
}

bool EncoderInterface::setSize(uint16_t width, uint16_t height)
{
    rawWidth  = width;
    rawHeight = height;
    return true;
}

bool EncoderInterface::setPixelFormat(INDI_PIXEL_FORMAT pixelFormat, uint8_t pixelDepth)
{
    this->pixelFormat = pixelFormat;
    this->pixelDepth = pixelDepth;
    return true;
}
}
