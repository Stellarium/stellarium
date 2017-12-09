/*******************************************************************************
  Copyright(c) 2011 Jasem Mutlaq. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

/*! INDI property type */
typedef enum {
    INDI_NUMBER, /*!< INumberVectorProperty. */
    INDI_SWITCH, /*!< ISwitchVectorProperty. */
    INDI_TEXT,   /*!< ITextVectorProperty. */
    INDI_LIGHT,  /*!< ILightVectorProperty. */
    INDI_BLOB,   /*!< IBLOBVectorProperty. */
    INDI_UNKNOWN
} INDI_PROPERTY_TYPE;

/*! INDI Equatorial Axis type */
typedef enum {
    AXIS_RA, /*!< Right Ascension Axis. */
    AXIS_DE  /*!< Declination Axis. */
} INDI_EQ_AXIS;

/*! INDI Horizontal Axis type */
typedef enum {
    AXIS_AZ, /*!< Azimuth Axis. */
    AXIS_ALT /*!< Altitude Axis. */
} INDI_HO_AXIS;

/*! North/South Direction type */
typedef enum {
    DIRECTION_NORTH = 0, /*!< North direction */
    DIRECTION_SOUTH      /*!< South direction */
} INDI_DIR_NS;

/*! West/East Direction type */
typedef enum {
    DIRECTION_WEST = 0, /*!< West direction */
    DIRECTION_EAST      /*!< East direction */
} INDI_DIR_WE;

/*! INDI Error Types */
typedef enum {
    INDI_DEVICE_NOT_FOUND    = -1, /*!< Device not found error */
    INDI_PROPERTY_INVALID    = -2, /*!< Property invalid error */
    INDI_PROPERTY_DUPLICATED = -3, /*!< Property duplicated error */
    INDI_DISPATCH_ERROR      = -4  /*!< Dispatch error */
} INDI_ERROR_TYPE;

typedef enum
{
    INDI_MONO       = 0,
    INDI_BAYER_RGGB = 8,
    INDI_BAYER_GRBG = 9,
    INDI_BAYER_GBRG = 10,
    INDI_BAYER_BGGR = 11,
    INDI_BAYER_CYYM = 16,
    INDI_BAYER_YCMY = 17,
    INDI_BAYER_YMCY = 18,
    INDI_BAYER_MYYC = 19,
    INDI_RGB        = 100,
    INDI_BGR        = 101,
    INDI_JPG        = 200,
} INDI_PIXEL_FORMAT;

