/*
 QHYCCD SDK
 
 Copyright (c) 2014 QHYCCD.
 All Rights Reserved.
 
 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option)
 any later version.
 
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.
 
 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 The full GNU General Public License is included in this distribution in the
 file called LICENSE.
 */

/*! \file qhyccderr.h
      \brief QHYCCD SDK error define
  */

#ifndef __QHYCCDERR_H__
#define __QHYCCDERR_H__

#define QHYCCD_READ_DIRECTLY            0x2001

#define QHYCCD_DELAY_200MS              0x2000
/**
 * It means the camera using GiGaE transfer data */
#define QHYCCD_QGIGAE                   7

/**
 * It means the camera using usb sync transfer data */
#define QHYCCD_USBSYNC                  6

/**
 * It means the camera using usb async transfer data */
#define QHYCCD_USBASYNC                 5

/**
 * It means the camera is color one */
#define QHYCCD_COLOR                    4

/**
 * It means the camera is mono one*/
#define QHYCCD_MONO                     3

/**
 * It means the camera has cool function */
#define QHYCCD_COOL                     2

/**
 * It means the camera do not have cool function */
#define QHYCCD_NOTCOOL                  1

/**
 * camera works well */
#define QHYCCD_SUCCESS                  0

/**
 * Other error */
#define QHYCCD_ERROR                    0xFFFFFFFF

#if 0
/**
 * There is no camera connected */
#define QHYCCD_ERROR_NO_DEVICE         -2

/**
 * Do not support the function */
#define QHYCCD_ERROR        -3

/**
 * Set camera params error */
#define QHYCCD_ERROR_SETPARAMS         -4

/**
 * Get camera params error */
#define QHYCCD_ERROR_GETPARAMS         -5

/**
 * The camera is exposing now */
#define QHYCCD_ERROR_EXPOSING          -6

/**
 * The camera expose failed */
#define QHYCCD_ERROR_EXPFAILED         -7

/**
 * There is another instance is getting data from camera */
#define QHYCCD_ERROR_GETTINGDATA       -8

/**
 * Get data from camera failed */
#define QHYCCD_ERROR_GETTINGFAILED     -9

/**
 * Init camera failed */
#define QHYCCD_ERROR_INITCAMERA        -10

/**
 * Release SDK resouce failed */
#define QHYCCD_ERROR_RELEASERESOURCE   -11

/**
 * Init SDK resouce failed */
#define QHYCCD_ERROR_INITRESOURCE      -12

/**
 * There is no match camera */
#define QHYCCD_ERROR             -13

/**
 * Open cam failed */
#define QHYCCD_ERROR_OPENCAM           -14

/**
 * Init cam class failed */
#define QHYCCD_ERROR_INITCLASS         -15

/**
 * Set Resolution failed */
#define QHYCCD_ERROR        -16

/**
 * Set usbtraffic failed */
#define QHYCCD_ERROR        -17

/**
 * Set usb speed failed */
#define QHYCCD_ERROR          -18

/**
 * Set expose time failed */
#define QHYCCD_ERROR_SETEXPOSE         -19

/**
 * Set cam gain failed */
#define QHYCCD_ERROR_SETGAIN           -20

/**
 * Set cam white balance red failed */
#define QHYCCD_ERROR_SETRED            -21

/**
 * Set cam white balance blue failed */
#define QHYCCD_ERROR_SETBLUE           -22

/**
 * Set cam white balance blue failed */
#define QHYCCD_ERROR_EVTCMOS           -23

/**
 * Set cam white balance blue failed */
#define QHYCCD_ERROR_EVTUSB            -24

/**
 * Set cam white balance blue failed */
#define QHYCCD_ERROR           -25
#endif

#endif
