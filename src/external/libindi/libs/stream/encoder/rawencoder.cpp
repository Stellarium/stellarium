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

#include "rawencoder.h"
#include "stream/streammanager.h"
#include "indiccd.h"

#include <zlib.h>

namespace INDI
{

RawEncoder::RawEncoder()
{
    name = "RAW";
    compressedFrame = (uint8_t *)malloc(1);
}

RawEncoder::~RawEncoder()
{
    free(compressedFrame);
}

const char *RawEncoder::getDeviceName()
{
    return currentCCD->getDeviceName();
}

bool RawEncoder::upload(IBLOB *bp, const uint8_t *buffer, uint32_t nbytes, bool isCompressed)
{
    // Do we want to compress ?
    if (isCompressed)
    {
        /* Compress frame */
        compressedFrame = (uint8_t *)realloc(compressedFrame, sizeof(uint8_t) * nbytes + nbytes / 64 + 16 + 3);
        uLongf compressedBytes = sizeof(uint8_t) * nbytes + nbytes / 64 + 16 + 3;

        int ret = compress2(compressedFrame, &compressedBytes, buffer, nbytes, 4);
        if (ret != Z_OK)
        {
            /* this should NEVER happen */
            DEBUGF(INDI::Logger::DBG_ERROR, "internal error - compression failed: %d", ret);
            return false;
        }

        // Send it compressed
        bp->blob    = compressedFrame;
        bp->bloblen = compressedBytes;
        bp->size    = nbytes;
        strcpy(bp->format, ".stream.z");
    }
    else
    {
        // Send it uncompressed
        bp->blob    = (const_cast<uint8_t *>(buffer));
        bp->bloblen = nbytes;
        bp->size    = nbytes;
        strcpy(bp->format, ".stream");
    }

    return true;
}
}
