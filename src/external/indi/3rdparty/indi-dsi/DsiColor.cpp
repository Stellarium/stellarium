/*
 * Copyright (c) 2009, Roland Roberts <roland@astrofoto.org>
 * Copyright (c) 2015, Ben Gilsrud <bgilsrud@gmail.com>
 */

#include "DsiColor.h"

#include <indidevapi.h>

using namespace DSI;

void DsiColor::initImager(const char *devname)
{
    INDI_UNUSED(devname);
    command(DeviceCommand::SET_ROW_COUNT_EVEN, read_height_even);
    command(DeviceCommand::SET_ROW_COUNT_ODD, read_height_odd);

    if (is_high_gain)
        sendRegister(AdRegister::CONFIGURATION, 0x58);
    else
        sendRegister(AdRegister::CONFIGURATION, 0xd8);

    sendRegister(AdRegister::MUX_CONFIG, 0xc0);

    /* Sigh.  "Interestingly," it would appear that you MUST take at least one
     * throw-away image or else GET_EXP_TIMER_COUNT won't work on "real"
     * exposures.  Sounds like a firmware bug to me, but not something we can
     * do anything about.  Although MaximDL uses 4 exposures of 1000 ticks
     * (100 ms) each, this shorter exposure works just fine.
     */
    for (int i = 0; i < 1; i++)
    {
        unsigned char *foo = getImage(1);
        delete[] foo;
    }
}

DsiColor::DsiColor(const char *devname) : Device(devname)
{
    // color_type = ColorType.NONE;
    is_high_gain = true;

    //FIXME: We currently lie about color so we don't have to worry about
    //       demosaicing the CMYG filter.
    is_color = false;

    /* Turn off test mode so we can actually return an image */
    test_pattern = false;

    /* The DSI Color I cannot be put into 2x2 binning mode. */
    is_binnable = false;

    /* The DSI Color I is not supposed to equipped with a temperature sensor */
    has_tempsensor = false;

    aspect_ratio = 0.78125;

    read_width       = 537;
    read_height_even = 253;
    read_height_odd  = 252;
    read_height      = read_height_even + read_height_odd;

    read_bpp = 2;

    image_width    = 508;
    image_height   = 488;
    image_offset_x = 23;
    image_offset_y = 13;
    pixel_size_x   = 9.6;
    pixel_size_y   = 7.5;

    exposure_time = 10;

    initImager();
}

DsiColor::~DsiColor()
{
}
