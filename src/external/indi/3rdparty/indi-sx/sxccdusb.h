/*
  Starlight Xpress CCD INDI Driver

  Code is based on SX SDK by David Schmenk and Craig Stark
  Copyright (c) 2003 David Schmenk
  All rights reserved.

  Changes for INDI project by Peter Polakovic
  Copyright (c) 2012-2013 Cloudmakers, s. r. o.
  All Rights Reserved.

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, and/or sell copies of the Software, and to permit persons
  to whom the Software is furnished to do so, provided that the above
  copyright notice(s) and this permission notice appear in all copies of
  the Software and that both the above copyright notice(s) and this
  permission notice appear in supporting documentation.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
  OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
  INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
  NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  Except as contained in this notice, the name of a copyright holder
  shall not be used in advertising or otherwise to promote the sale, use
  or other dealings in this Software without prior written authorization
  of the copyright holder.
*/

#pragma once

#ifdef OSX_EMBEDED_MODE
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

/*
 * CCD color representation.
 *  Packed colors allow individual sizes up to 16 bits.
 *  2X2 matrix bits are represented as:
 *      0 1
 *      2 3
 */
#define SXCCD_COLOR_PACKED_RGB        0x8000
#define SXCCD_COLOR_PACKED_BGR        0x4000
#define SXCCD_COLOR_PACKED_RED_SIZE   0x0F00
#define SXCCD_COLOR_PACKED_GREEN_SIZE 0x00F0
#define SXCCD_COLOR_PACKED_BLUE_SIZE  0x000F
#define SXCCD_COLOR_MATRIX_ALT_EVEN   0x2000
#define SXCCD_COLOR_MATRIX_ALT_ODD    0x1000
#define SXCCD_COLOR_MATRIX_2X2        0x0000
#define SXCCD_COLOR_MATRIX_RED_MASK   0x0F00
#define SXCCD_COLOR_MATRIX_GREEN_MASK 0x00F0
#define SXCCD_COLOR_MATRIX_BLUE_MASK  0x000F
#define SXCCD_COLOR_MONOCHROME        0x0FFF

/*
 * Capabilities (Caps) bit definitions.
 */
#define SXCCD_CAPS_STAR2K   0x01
#define SXCCD_CAPS_COMPRESS 0x02
#define SXCCD_CAPS_EEPROM   0x04
#define SXCCD_CAPS_GUIDER   0x08
#define SXUSB_CAPS_COOLER   0x10
#define SXUSB_CAPS_SHUTTER  0x20

/*
 * CCD command flags bit definitions.
 */
#define CCD_EXP_FLAGS_FIELD_ODD        0x01
#define CCD_EXP_FLAGS_FIELD_EVEN       0x02
#define CCD_EXP_FLAGS_FIELD_BOTH       (CCD_EXP_FLAGS_FIELD_EVEN | CCD_EXP_FLAGS_FIELD_ODD)
#define CCD_EXP_FLAGS_FIELD_MASK       CCD_EXP_FLAGS_FIELD_BOTH
#define CCD_EXP_FLAGS_SPARE2           0x04
#define CCD_EXP_FLAGS_NOWIPE_FRAME     0x08
#define CCD_EXP_FLAGS_SPARE4           0x10
#define CCD_EXP_FLAGS_TDI              0x20
#define CCD_EXP_FLAGS_NOCLEAR_FRAME    0x40
#define CCD_EXP_FLAGS_NOCLEAR_REGISTER 0x80

// Upper bits in byte of CCD_EXP_FLAGS word
#define CCD_EXP_FLAGS_SPARE8         0x01
#define CCD_EXP_FLAGS_SPARE9         0x02
#define CCD_EXP_FLAGS_SPARE10        0x04
#define CCD_EXP_FLAGS_SPARE11        0x08
#define CCD_EXP_FLAGS_SPARE12        0x10
#define CCD_EXP_FLAGS_SHUTTER_MANUAL 0x20
#define CCD_EXP_FLAGS_SHUTTER_OPEN   0x40
#define CCD_EXP_FLAGS_SHUTTER_CLOSE  0x80

/*
 * Serial port queries.
 */
#define SXCCD_SERIAL_PORT_AVAIL_OUTPUT 0
#define SXCCD_SERIAL_PORT_AVAIL_INPUT  1

/*
 * Limits.
 */
#define SXCCD_MAX_CAMS 2

/*
 * libusb types abstraction.
 */

#define DEVICE libusb_device *
#define HANDLE libusb_device_handle *

/*
 * Structure to hold camera information.
 */
struct t_sxccd_params
{
    unsigned short hfront_porch;
    unsigned short hback_porch;
    unsigned short width;
    unsigned short vfront_porch;
    unsigned short vback_porch;
    unsigned short height;
    float pix_width;
    float pix_height;
    unsigned short color_matrix;
    char bits_per_pixel;
    char num_serial_ports;
    char extra_caps;
    char vclk_delay;
};

/*
 * Prototypes.
 */

void sxDebug(bool enable);
int sxList(DEVICE *sxDevices, const char **names, int maxCount);
int sxOpen(HANDLE *sxHandles);
int sxOpen(DEVICE sxDevice, HANDLE *sxHandle);
void sxClose(HANDLE *sxHandle);
unsigned short sxGetCameraModel(HANDLE sxHandle);
unsigned long sxGetFirmwareVersion(HANDLE sxHandle);
unsigned short sxGetBuildNumber(HANDLE sxHandle);
int sxReset(HANDLE sxHandle);
int sxGetCameraParams(HANDLE sxHandle, unsigned short camIndex, struct t_sxccd_params *params);
int sxClearPixels(HANDLE sxHandle, unsigned short flags, unsigned short camIndex);
int sxLatchPixels(HANDLE sxHandle, unsigned short flags, unsigned short camIndex, unsigned short xoffset,
                  unsigned short yoffset, unsigned short width, unsigned short height, unsigned short xbin,
                  unsigned short ybin);
int sxExposePixels(HANDLE sxHandle, unsigned short flags, unsigned short camIndex, unsigned short xoffset,
                   unsigned short yoffset, unsigned short width, unsigned short height, unsigned short xbin,
                   unsigned short ybin, unsigned long msec);
int sxExposePixelsGated(HANDLE sxHandle, unsigned short flags, unsigned short camIndex, unsigned short xoffset,
                        unsigned short yoffset, unsigned short width, unsigned short height, unsigned short xbin,
                        unsigned short ybin, unsigned long msec);
int sxReadPixels(HANDLE sxHandle, void *pixels, unsigned long count);
int sxSetShutter(HANDLE sxHandle, unsigned short state);
int sxSetTimer(HANDLE sxHandle, unsigned long msec);
unsigned long sxGetTimer(HANDLE sxHandle);
int sxSetSTAR2000(HANDLE sxHandle, char star2k);
int sxSetSerialPort(HANDLE sxHandle, unsigned short portIndex, unsigned short property, unsigned short value);
unsigned short sxGetSerialPort(HANDLE sxHandle, unsigned short portIndex, unsigned short property);
int sxWriteSerialPort(HANDLE sxHandle, unsigned short portIndex, unsigned short flush, unsigned short count,
                      char *data);
int sxReadSerialPort(HANDLE sxHandle, unsigned short portIndex, unsigned short count, char *data);
int sxReadEEPROM(HANDLE sxHandle, unsigned short address, unsigned short count, char *data);
int sxSetCooler(HANDLE sxHandle, unsigned char SetStatus, unsigned short SetTemp, unsigned char *RetStatus,
                unsigned short *RetTemp);
bool sxIsInterlaced(short model);
bool sxIsColor(short model);
