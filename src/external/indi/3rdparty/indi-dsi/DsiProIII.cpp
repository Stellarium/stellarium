/*
 * Copyright (c) 2009, Roland Roberts <roland@astrofoto.org>
 *
 * Modifications and extensions for DSI III support 2015, G. Schmidt (gs)
 *
 */

#include "DsiProIII.h"

#include <indidevapi.h>

using namespace DSI;

void DsiProIII::initImager(const char *devname)
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

DsiProIII::DsiProIII(const char *devname) : Device(devname)
{
    // color_type = ColorType.NONE;
    is_high_gain = false;

    is_color = false;

    /* Turn off test mode so we can actually return an image */
    test_pattern = false;

    /* The DSI Pro III supports binning */
    is_binnable = true;

    /* The DSI Pro III is equipped with a temperature sensor */
    has_tempsensor = true;

    /* Sony lists the pixel size a 6.45x6.45 microns. */

    aspect_ratio = 1.0;

    read_width       = 1434;
    read_height_even = 0;
    read_height_odd  = 1050;
    read_height      = read_height_even + read_height_odd;

    read_bpp = 2;

    image_width    = 1360;
    image_height   = 1024;
    image_offset_x = 30;
    image_offset_y = 13;

    timeout_response = 1000;
    timeout_request  = 1000;
    timeout_image    = 5000;
    pixel_size_x     = 6.45;
    pixel_size_y     = 6.45;

    exposure_time = 10;

    initImager();
}

DsiProIII::~DsiProIII()
{
}
