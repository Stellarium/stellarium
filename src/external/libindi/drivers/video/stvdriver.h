#if 0
STV driver
Copyright (C) 2006 Markus Wildi, markus.wildi@datacomm.ch

This library is free software;
you can redistribute it and / or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY;
without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library;
if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301  USA
#endif

#pragma once

#define OFF 0
#define ON  1

#define REQUEST_DOWNLOAD              0x00
#define REQUEST_DOWNLOAD_ALL          0x01
#define DOWNLOAD_COMPLETE             0x02
#define REQUEST_BUFFER_STATUS         0x03
#define REQUEST_IMAGE_INFO            0x04
#define REQUEST_IMAGE_DATA            0x05
#define ACK                           0x06
#define REQUEST_COMPRESSED_IMAGE_DATA 0x07
#define SEND_KEY_PATTERN              0x08
#define DISPLAY_ECHO                  0x09
#define FILE_STATUS                   0x0b
#define REQUEST_ACK                   0x10
#define NACK                          0x15

/* Rotary Knob Key Patterns */
#define LR_ROTARY_DECREASE_PATTERN 0x8000
#define LR_ROTARY_INCREASE_PATTERN 0x4000
#define UD_ROTARY_DECREASE_PATTERN 0x2000
#define UD_ROTARY_INCREASE_PATTERN 0x1000
#define SHIFT_PATTERN              0x0008 /* increases rotary speed when 1 */

/* Mode Key Patterns */
#define CAL_KEY_PATTERN     0x0100
#define TRACK_KEY_PATTERN   0x0200
#define DISPLAY_KEY_PATTERN 0x0400
#define FILEOPS_KEY_PATTERN 0x0800
#define A_KEY_PATTERN       0x0010
#define SETUP_KEY_PATTERN   0x0020
#define B_KEY_PATTERN       0x0040
#define INT_KEY_PATTERN     0x0080
#define FOCUS_KEY_PATTERN   0x0001
#define IMAGE_KEY_PATTERN   0x0002
#define MONITOR_KEY_PATTERN 0x0004

/* The following bit masks have been take from Sbig's documentation */
#define ID_BITS_MASK        0x0001 /* mask for no bits*/
#define ID_BITS_10          0x0001 /*       image is full 10 bits*/
#define ID_BITS_8           0x0000 /*       image from focus, only 8 bits*/
#define ID_UNITS_MASK       0x0002 /* mask for units for scope*/
#define ID_UNITS_INCHES     0x0002 /*       units were inches*/
#define ID_UNITS_CM         0x0000 /*       units were cm*/
#define ID_SCOPE_MASK       0x0004 /* mask for telescope type*/
#define ID_SCOPE_REFRACTOR  0x0004 /*       scope was refractor*/
#define ID_SCOPE_REFLECTOR  0x0000 /*       scope was reflector*/
#define ID_DATETIME_MASK    0x0008 /* mask for date/time valid*/
#define ID_DATETIME_VALID   0x0008 /*       date/time was set*/
#define ID_DATETIME_INVALID 0x0000 /*       date/time was not set*/
#define ID_BIN_MASK         0x0030 /* mask for binning mode*/
#define ID_BIN_1X1          0x0010 /*       binning was 1x1*/
#define ID_BIN_2X2          0x0020 /*       binning was 2x2*/
#define ID_BIN_3X3          0x0030 /*       binning was 3x3*/
#define ID_PM_MASK          0x0400 /* mask for am/pm in time*/
#define ID_PM_PM            0x0400 /*       time was pm, add 12 hours*/
#define ID_PM_AM            0x0000 /*       time was am, don;t add 12 hours*/
#define ID_FILTER_MASK      0x0800 /* mask for filter status*/
#define ID_FILTER_LUNAR     0x0800 /*       lunar filter was used for image*/
#define ID_FILTER_NP        0x0000 /*       no filter was used for image*/
#define ID_DARKSUB_MASK     0x1000 /* mask for dark subtraction*/
#define ID_DARKSUB_YES      0x1000 /*       image was dark subtracted*/
#define ID_DARKSUB_NO       0x0000 /*       image was not dark subtracted*/
#define ID_MOSAIC_MASK      0x6000 /* mask for mosaic status*/
#define ID_MOSAIC_NONE      0x0000 /*       no mosaic, one image per frame*/
#define ID_MOSAIC_SMALL     0x2000 /*       small mosaic: 40x40 pixels/image*/
#define ID_MOSAIC_LARGE     0x4000 /*       large mosaic: 106x100 pixels/image*/

/* IMAGE_INFO - data for the image

   Notes:
   height     - 0 or 0xFFFF if no data present
   exposure   - 100-60000 = 1.00 - 600 secs by 0.01
                60001-60999 = 0.001 - 0.999 secs by 0.001
   packedDate - bits  6-0  = year - 1999 (0 -127)
                bits 11-7  = day ( 1-31)
                bits 15-12 = month (1-12)
   packedTime - bits  6-0  = seconds (0-59)
                bits 7-12  = minutes (0-59)
                bits 15-13 = hours mod 12 (0-11)
                             + bit in descriptor can add 12
*/

typedef struct
{
    unsigned int descriptor;    /* set of bits*/
    unsigned int height, width; /* image sze */
    unsigned int top, left;     /* position in buffer */
    double exposure;            /* exposure time */
    unsigned int noExposure;    /* number of exposures added */
    unsigned int analogGain;    /*analog gain */
    int digitalGain;            /* digital gain */
    unsigned int focalLength;   /*focal length of telescope */
    unsigned int aperture;      /* aperture diameter */
    unsigned int packedDate;    /* date of image */
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int packedTime;          /* time of image */
    unsigned int seconds;             /* time of image */
    unsigned int minutes;             /* time of image */
    unsigned int hours;               /* time of image */
    double ccdTemp;                   /* temperature of ccd in 1/100 deg C */
    unsigned int siteID;              /* site id */
    unsigned int eGain;               /* eGain in 1/100th e/ADU */
    unsigned int background;          /* background for display */
    unsigned int range;               /* range for display */
    unsigned int pedestal;            /* Track and Accumulate pedestal */
    unsigned int ccdTop, ccdLeft;     /* position of pixels on CCD array */
    unsigned int adcResolution;       /* value, 8 or 10 bits */
    unsigned int units;               /* 0= cm, 1=inch */
    unsigned int telescopeType;       /* 0=refractor, 1= reflector */
    unsigned int dateTimeValid;       /* 0= valid */
    unsigned int binning;             /* 1x1=1, 2x2=2, 3x3=3 */
    unsigned int filterStatus;        /* 0= no filter, 1= lunar filter */
    unsigned int darkFrameSuntracted; /* 0= no, 1= yes */
    unsigned int imageIsMosaic;       /* 0=no, 1=40x40 pixels, 2=106x100 pixels */
    double pixelSize;                 /* 7.4 um */
    double minValue;                  /* Pixel Contents */
    double maxValue;

} IMAGE_INFO;
/*
 * $Id: serial.h 49 2006-08-25 18:07:14Z lukas $
 *
 * Copyright (C) 2006, Lukas Zimmermann, Basel, Switzerland.
 *
 *
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Or visit http://www.gnu.org/licenses/gpl.html.
*/

#define PARITY_NONE 0
#define PARITY_EVEN 1
#define PARITY_ODD  2

typedef unsigned char byte; /* define byte type */

/* Restores terminal settings of open serial port device and close the file. */
void shutdown_serial(int fd);

/* Opens and initializes a serial device and returns it's file descriptor. */
int init_serial(char *device_name, int bit_rate, int word_size, int parity, int stop_bits);

/* Calculates the 16 bit CRC of an array of bytes and returns it. */
unsigned int calc_crc(byte byte_array[], int size);
