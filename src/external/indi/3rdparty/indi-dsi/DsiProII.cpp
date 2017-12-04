/*
 * Copyright (c) 2009, Roland Roberts <roland@astrofoto.org>
 *
 */

#include "DsiProII.h"

#include <indidevapi.h>

using namespace DSI;

void DsiProII::initImager(const char *devname)
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

DsiProII::DsiProII(const char *devname) : Device(devname)
{
    // color_type = ColorType.NONE;
    is_high_gain = true;

    is_color = false;

    /* Turn off test mode so we can actually return an image */
    test_pattern = false;

    /* The DSI Pro II, uhm, can it be put in 2x2 binning mode?  We can't
       handle that yet, so say "no" */
    is_binnable = false;

    /* The DSI Pro II should be equipped with a temperature sensor */
    has_tempsensor = true;

    /* Sony lists the pixel size a 9.6x7.5 microns.  Craig Stark, in a note at
     * http://www.skyinsight.com/wiki/ regarding the Orion Star Shoot, points
     * out that that size does not fill the size of the chip. I'm not sure we
     * care; I think all we care about is the pixel-to-pixel center distance
     * as this determines how the final image has to be scaled to have a 1:1
     * aspect ratio for display.
     */
    aspect_ratio = 0.78125;

    read_width       = 795;
    read_height_even = 299;
    read_height_odd  = 298;
    read_height      = read_height_even + read_height_odd;

    read_bpp = 2;

    image_width    = 748;
    image_height   = 577;
    image_offset_x = 30;
    image_offset_y = 13;

    timeout_response = 1000;
    timeout_request  = 1000;
    timeout_image    = 5000;
    pixel_size_x     = 8.6;
    pixel_size_y     = 8.3;

    exposure_time = 10;

    initImager();
}

DsiProII::~DsiProII()
{
}
