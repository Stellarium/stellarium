/*
 GigE interface wrapper on araviss
 Copyright (C) 2016 Hendrik Beijeman (hbeyeman@gmail.com)

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
#include "ArvGeneric.h"
#include "BlackFly.h"

using namespace arv;

BlackFly::BlackFly(void *camera_device) : ArvGeneric(camera_device)
{
    printf("%s\n", __PRETTY_FUNCTION__);
}

bool BlackFly::_custom_settings()
{
    printf("%s\n", __PRETTY_FUNCTION__);
    ::ArvDevice *dev = this->dev;
#define WRITE_REGISTER(reg, val)                                       \
    {                                                                  \
        error  = NULL;                                                 \
        result = arv_device_write_register(dev, (reg), (val), &error); \
        if ((result != 1) || (error != NULL))                          \
        {                                                              \
            goto error;                                                \
        }                                                              \
    }

    gboolean result;
    guint32 val;
    GError *error = NULL;

    int i;
    for (i = 0; i < 4; i++)
    {
        WRITE_REGISTER(0xF0F00614UL, 0x00000000UL); /* Set Isochronous transfer off */
        WRITE_REGISTER(0xF0F00608UL, 0xE0000000UL); /* Set Current video format 7 */
        WRITE_REGISTER(0xF0F00604UL, 0x00000000UL); /* Set Current video mode 0  */

        WRITE_REGISTER(0xF0F00630UL, 0x00800000UL); /* Little Endian, please! */

        WRITE_REGISTER(0xF0F00A08UL, 0x00000000UL); /* Mode0 IMAGE_POSITION (max)*/
        WRITE_REGISTER(0xF0F00A0CUL, 0x08000600UL); /* Mode0 IMAGE_SIZE     (max) */
        WRITE_REGISTER(0xF0F00A10UL, 0x0a000000UL); /* Mode0 IMAGE_FORMAT, configure RAW16   */
        WRITE_REGISTER(0xF0F00A7CUL, 0x40000000UL); /* Mode0 IMAGE_POSITION/COLOR_CODING_ID */
        WRITE_REGISTER(0xF0F00A44UL, 0x34540000UL); /* Mode0 BYTE_PER_PACKET */

        /* Disable auto-frame-rate */
        WRITE_REGISTER(0xF0F0081CUL, 0xC2000FFFUL); /* FRAME_RATE, No AUTO, OFF */
    }

    return true;

error:
    return false;
}

bool BlackFly::connect(void)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    bool const ret = ArvGeneric::connect();
    if (ret)
    {
        this->_configure();
    }
    return ret;
}

void BlackFly::_fixup(void)
{
    ::ArvDevice *dev = this->dev;
#define WRITE_REGISTER(reg, val)                                       \
    {                                                                  \
        error  = NULL;                                                 \
        result = arv_device_write_register(dev, (reg), (val), &error); \
        if ((result != 1) || (error != NULL))                          \
        {                                                              \
            goto error;                                                \
        }                                                              \
    }

    gboolean result;
    guint32 val;
    GError *error = NULL;

    int i;
    for (i = 0; i < 4; i++)
    {
        WRITE_REGISTER(0xF0F00614UL, 0x00000000UL); /* Isochronous transfer off */
        WRITE_REGISTER(0xF0F00630UL, 0x00800000UL); /* Little Endian, please! */
    }
error:
    return;
}

void BlackFly::exposure_start(void)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    /* At some point in stream start, the endianness gets reset by the camera itself... why? genicam? */
    this->_fixup();
    ArvGeneric::exposure_start();
}

bool BlackFly::_configure(void)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    this->_set_initial_config();
    return this->_get_initial_config();
}

bool BlackFly::_get_initial_config(void)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    ArvGeneric::_get_initial_config();

    /* Pitch can only be found in the datasheet, so hardcode ours here */
    this->cam.pixel_pitch.set_single(3.45);

    /* Probably can find this somewhere in genicam, too, but it depends on framerate
     * and the maximum exposure is consequently not published */
    this->cam.exposure.update(5000, 11900000);
}

bool BlackFly::_set_initial_config(void)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    ArvGeneric::_set_initial_config();
    this->_custom_settings();
}
