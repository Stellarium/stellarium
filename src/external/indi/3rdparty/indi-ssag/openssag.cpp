/*
 * File: openssag.cpp
 *
 * Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
 *
 * Migration to libusb 1.0 by Peter Polakovic
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef OSX_EMBEDED_MODE
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif
#include <time.h>
#include <unistd.h>

#include "openssag.h"
#include "openssag_priv.h"

/*
 * MT9M001 Pixel Array
 *
 * |-----------------1312 Pixels------------------|
 *
 *    |--------------1289 Pixels---------------|
 *
 *       |-----------1280 Pixels------------|
 *
 * +----------------------------------------------+     -
 * |  Black Rows          8                       |     |
 * |  +----------------------------------------+  |     |               -
 * |  |  Padding          4                    |  |     |               |
 * |  |  +----------------------------------+  |  |     |               |               -
 * |  |  | SXGA                             |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * | 7| 5|                                  |4 |16|     | 1048 Pixels   | 1033 Pixels   | 1024 Pixels
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  +----------------------------------+  |  |     |               |               -
 * |  |                   5                    |  |     |               |
 * |  +----------------------------------------+  |     |               -
 * |                      7                       |     |
 * +----------------------------------------------+     -
 *
 *
 */

/* USB commands to control the camera */
enum USB_REQUEST
{
    USB_RQ_GUIDE           = 16, /* 0x10 */
    USB_RQ_EXPOSE          = 18, /* 0x12 */
    USB_RQ_SET_INIT_PACKET = 19, /* 0x13 */
    USB_RQ_PRE_EXPOSE      = 20, /* 0x14 */
    USB_RQ_SET_BUFFER_MODE = 85, /* 0x55 */

    /* These aren't tested yet */
    USB_RQ_CANCEL_GUIDE             = 24, /* 0x18 */
    USB_RQ_CANCEL_GUIDE_NORTH_SOUTH = 34, /* 0x22 */
    USB_RQ_CANCEL_GUIDE_EAST_WEST   = 33  /* 0x21 */
};

#define USB_TIMEOUT 5000

/* USB Bulk endpoint to grab data from */
#define BUFFER_ENDPOINT 0x82

/* Image size. Values must be even numbers. */
#define IMAGE_WIDTH  1280
#define IMAGE_HEIGHT 1024

/* Horizontal Blanking (in pixels) */
#define HORIZONTAL_BLANKING 244
/* Vertical Blanking (in rows) */
#define VERTICAL_BLANKING 25

/* Buffer size is determined by image size + horizontal/vertical blanking */
#define BUFFER_WIDTH  (IMAGE_WIDTH + HORIZONTAL_BLANKING)
#define BUFFER_HEIGHT (IMAGE_HEIGHT + VERTICAL_BLANKING + 1)
#define BUFFER_SIZE   (BUFFER_WIDTH * BUFFER_HEIGHT)

/* Number of pixel columns/rows to skip. Values must be even numbers. */
#define ROW_START    12
#define COLUMN_START 20

/* Shutter width */
#define SHUTTER_WIDTH (IMAGE_HEIGHT + VERTICAL_BLANKING)

/* Pixel offset appears to be calculated based on the dimensions of the chip.
 * 31 = 16 + 4 + 4 + 7 and there are 8 rows of optically black pixels. At the
 * moment, I'm not exactly sure why this would be required. It also appears to
 * change randomly at times. */
#define PIXEL_OFFSET (8 * (BUFFER_WIDTH + 31))

/* Number of seconds to wait for camera to renumerate after loading firmware */
#define RENUMERATE_TIMEOUT 10

using namespace OpenSSAG;

libusb_context *ctx = NULL;

SSAG::SSAG()
{
    if (ctx == NULL)
    {
        int rc = libusb_init(&ctx);
        if (rc < 0)
        {
            DBG("Can't initialize libusb (%d)", rc);
        }
    }
}

bool SSAG::Connect(bool bootload)
{
    if ((this->handle = libusb_open_device_with_vid_pid(ctx, SSAG_VENDOR_ID, SSAG_PRODUCT_ID)) == NULL)
    {
        if (bootload)
        {
            Loader *loader = new Loader();
            if (loader->Connect())
            {
                if (!loader->LoadFirmware())
                {
                    DBG("Failed to upload firmware to the device");
                    return false;
                }
                loader->Disconnect();
                for (int i = 0; i < RENUMERATE_TIMEOUT; i++)
                {
                    if (this->Connect(false))
                        return true;
                    sleep(1);
                }
                DBG("Device did not renumerate. Timed out.");
                return false;
            }
            else
            {
                DBG("Failed to connect loader");
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    int rc;
    if (libusb_kernel_driver_active(this->handle, 0) == 1)
    {
        rc = libusb_detach_kernel_driver(this->handle, 0);
        if (rc < 0)
        {
            DBG("Can't detach kernel driver (%d)", rc);
        }
    }

    rc = libusb_set_configuration(this->handle, 1);
    if (rc < 0)
    {
        DBG("Can't set configuration (%d)", rc);
    }

    rc = libusb_claim_interface(this->handle, 0);
    if (rc < 0)
    {
        DBG("Can't claim interface (%d)", rc);
    }

    this->SetBufferMode();
    this->SetGain(1);
    this->InitSequence();

    return true;
}

bool SSAG::Connect()
{
    return this->Connect(true);
}

void SSAG::Disconnect()
{
    if (this->handle)
        libusb_close(this->handle);
    this->handle = NULL;
}

void SSAG::SetBufferMode()
{
    unsigned char data[4];
    int rc = libusb_control_transfer(this->handle, 0xc0, USB_RQ_SET_BUFFER_MODE, 0x00, 0x63, data, sizeof(data),
                                     USB_TIMEOUT);
    if (rc < 0)
    {
        DBG("Can't read buffer mode data (%d)", rc);
    }
    //  DBG("Buffer Mode Data: %02x%02x%02x%02x", data[0], data[1], data[2], data[3]);
}

bool SSAG::IsConnected()
{
    return (this->handle != NULL);
}

struct raw_image *SSAG::Expose(int duration)
{
    this->InitSequence();
    unsigned char data[16];
    int rc = libusb_control_transfer(this->handle, 0xc0, USB_RQ_EXPOSE, duration & 0xFFFF, duration >> 16, data, 2,
                                     USB_TIMEOUT);
    if (rc < 0)
    {
        DBG("Can't send USB_RQ_EXPOSE request (%d)", rc);
    }
    struct raw_image *image = (raw_image *)malloc(sizeof(struct raw_image));
    image->width            = IMAGE_WIDTH;
    image->height           = IMAGE_HEIGHT;
    image->data             = this->ReadBuffer(duration + USB_TIMEOUT);

    //  DBG("Pixel Offset: %d", PIXEL_OFFSET);
    //  DBG("Buffer Size: %d", BUFFER_SIZE);
    //  DBG("  Buffer Width: %d", BUFFER_WIDTH);
    //  DBG("  Buffer Height: %d", BUFFER_HEIGHT);

    if (!image->data)
    {
        free(image);
        return NULL;
    }

    return image;
}

void SSAG::StartExposure(int duration)
{
    this->InitSequence();
    unsigned char data[16];
    int rc = libusb_control_transfer(this->handle, 0xc0, USB_RQ_EXPOSE, duration & 0xFFFF, duration >> 16, data, 2,
                                     USB_TIMEOUT);
    if (rc < 0)
    {
        DBG("Can't send USB_RQ_EXPOSE request (%d)", rc);
    }
}

void SSAG::CancelExposure()
{
    /* Not tested */
    unsigned char data = 0;
    int transferred;
    int rc = libusb_bulk_transfer(this->handle, LIBUSB_ENDPOINT_IN, &data, 1, &transferred, USB_TIMEOUT);
    if (rc < 0)
    {
        DBG("Can't abort exposure (%d)", rc);
    }
}

void SSAG::Guide(int direction, int duration)
{
    this->Guide(direction, duration, duration);
}

void SSAG::Guide(int direction, int yduration, int xduration)
{
    unsigned char data[8];

    memcpy(data, &xduration, 4);
    memcpy(data + 4, &yduration, 4);

    int rc =
        libusb_control_transfer(this->handle, 0x40, USB_RQ_GUIDE, 0, (int)direction, data, sizeof(data), USB_TIMEOUT);
    if (rc < 0)
    {
        DBG("Can't send USB_RQ_GUIDE request (%d)", rc);
    }
}

void SSAG::InitSequence()
{
    unsigned char init_packet[18] = {                                               /* Gain settings */
                                      0x00, static_cast<unsigned char>(this->gain), /* G1 Gain */
                                      0x00, static_cast<unsigned char>(this->gain), /* B  Gain */
                                      0x00, static_cast<unsigned char>(this->gain), /* R  Gain */
                                      0x00, static_cast<unsigned char>(this->gain), /* G2 Gain */

                                      /* Vertical Offset. Reg0x01 */
                                      ROW_START >> 8, ROW_START & 0xff,

                                      /* Horizontal Offset. Reg0x02 */
                                      COLUMN_START >> 8, COLUMN_START & 0xff,

                                      /* Image height - 1. Reg0x03 */
                                      (IMAGE_HEIGHT - 1) >> 8, (unsigned char)((IMAGE_HEIGHT - 1) & 0xff),

                                      /* Image width - 1. Reg0x04 */
                                      (IMAGE_WIDTH - 1) >> 8, (unsigned char)((IMAGE_WIDTH - 1) & 0xff),

                                      /* Shutter Width. Reg0x09 */
                                      SHUTTER_WIDTH >> 8, SHUTTER_WIDTH & 0xff
    };

    int wValue = BUFFER_SIZE & 0xffff;
    int wIndex = BUFFER_SIZE >> 16;
    int rc;

    rc = libusb_control_transfer(this->handle, 0x40, USB_RQ_SET_INIT_PACKET, wValue, wIndex, init_packet,
                                 sizeof(init_packet), USB_TIMEOUT);
    if (rc < 0)
    {
        DBG("Can't send USB_RQ_SET_INIT_PACKET request (%d)", rc);
    }
    rc = libusb_control_transfer(this->handle, 0x40, USB_RQ_PRE_EXPOSE, PIXEL_OFFSET, 0, NULL, 0, USB_TIMEOUT);
    if (rc < 0)
    {
        DBG("Can't send USB_RQ_PRE_EXPOSE request (%d)", rc);
    }
}

unsigned char *SSAG::ReadBuffer(int timeout)
{
    unsigned char *data = (unsigned char *)malloc(BUFFER_SIZE);
    unsigned char *dptr, *iptr;
    int transferred;

    int rc =
        libusb_bulk_transfer(this->handle, BUFFER_ENDPOINT, data, BUFFER_SIZE, &transferred, timeout + USB_TIMEOUT);

    if (transferred != BUFFER_SIZE)
    { // fixed!
        DBG("Failed to receive bytes of image data (%d)", rc);
        free(data);
        return NULL;
    }

    unsigned char *image = (unsigned char *)malloc(IMAGE_WIDTH * IMAGE_HEIGHT);
    dptr                 = data;
    iptr                 = image;
    for (int i = 0; i < IMAGE_HEIGHT; i++)
    {
        memcpy(iptr, dptr, IMAGE_WIDTH);
        /* Horizontal Blanking can be ignored */
        dptr += BUFFER_WIDTH;
        iptr += IMAGE_WIDTH;
    }
    free(data);
    return (unsigned char *)image;
}

void SSAG::SetGain(int gain)
{
    /* See the MT9M001 datasheet for more information on the following code. */
    if (gain < 1 || gain > 15)
    {
        DBG("Gain was out of valid range: %d (Should be 1-15)", gain);
        return;
    }

    /* Default PHD Setting */
    if (gain == 7)
    {
        this->gain = 0x3b;
    }
    else if (gain <= 4)
    {
        this->gain = gain * 8;
    }
    else if (gain <= 8)
    {
        this->gain = (gain * 4) + 0x40;
    }
    else if (gain <= 15)
    {
        this->gain = (gain - 8) + 0x60;
    }

    //  DBG("Setting gain to %d (Register value 0x%02x)", gain, this->gain);
}

void SSAG::FreeRawImage(raw_image *image)
{
    free(image->data);
    free(image);
}
