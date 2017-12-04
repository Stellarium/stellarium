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

/*!
   @file qhyccdcamdef.h
   @brief QHYCCD SDK error define
  */

#ifndef __QHYCCDCAMDEF_H__
#define __QHYCCDCAMDEF_H__

//#define GIGAESUPPORT

//#define QHYCCD_OPENCV_SUPPORT


/* IMG series */

/**
 * Type define for IMG0S */
#define DEVICETYPE_IMG0S        1000

/**
 * Type define for IMG0H */
#define DEVICETYPE_IMG0H        1001

/**
 * Type define for IMG0L */
#define DEVICETYPE_IMG0L        1002

/**
 * Type define for IMG0X */
#define DEVICETYPE_IMG0X        1003

/**
 * Type define for IMG1S */
#define DEVICETYPE_IMG1S        1004

/**
 * Type define for IMG2S */
#define DEVICETYPE_IMG2S        1005

/**
 * Type define for IMG1E */
#define DEVICETYPE_IMG1E        1006


/* QHY5 seires */

/**
 * Type define for QHY5 */
#define DEVICETYPE_QHY5         2001


/* QHY5II series */

/**
 * Type define for QHY5II */
#define DEVICETYPE_QHY5II       3001

/**
 * Type define for QHY5LII_M */
#define DEVICETYPE_QHY5LII_M    3002

/**
 * Type define for QHY5LII_C */
#define DEVICETYPE_QHY5LII_C    3003

/**
 * Type define for QHY5TII */
#define DEVICETYPE_QHY5TII      3004

/**
 * Type define for QHY5RII */
#define DEVICETYPE_QHY5RII      3005

/**
 * Type define for QHY5PII */
#define DEVICETYPE_QHY5PII      3006

/**
 * Type define for QHY5VII */
#define DEVICETYPE_QHY5VII      3007

/**
 * Type define for QHY5HII */
#define DEVICETYPE_QHY5HII      3008

/**
 * Type define for QHYXXX */
#define DEVICETYPE_MINICAM5S_M  3009

/**
 * Type define for QHYXXX */
#define DEVICETYPE_MINICAM5S_C  3010

/**
 * Type define for QHY5PII_C */
#define DEVICETYPE_QHY5PII_C    3011

/**
 * Type define for QHY5RII-M */
#define DEVICETYPE_QHY5RII_M    3012

/**
 * Type define for QHY5RII-M */
#define DEVICETYPE_MINICAM5F_M  3013

/**
 * Type define for QHY5PII_M */
#define DEVICETYPE_QHY5PII_M    3014

/**
 * Type define for QHY5TII */
#define DEVICETYPE_QHY5TII_C    3015

/**
 * Type define for POLEMASTER */
#define DEVICETYPE_POLEMASTER   3016

/**
 * Type define for QHY5IIEND */
#define DEVICETYPE_QHY5IIEND   3999


/* QHY5III seires */

/**
 * Type define for QHY5III174*/
#define DEVICETYPE_QHY5III174   4000

/**
 * Type define for QHY5III174 */
#define DEVICETYPE_QHY5III174M  4001

/**
 * Type define for QHY5III174C*/
#define DEVICETYPE_QHY5III174C  4002

/**
 * Type define for QHY174*/
#define DEVICETYPE_QHY174       4003

/**
 * Type define for QHY174M*/
#define DEVICETYPE_QHY174M      4004

/**
 * Type define for QHY174C*/
#define DEVICETYPE_QHY174C      4005

/**
 * Type define for QHY5III178*/
#define DEVICETYPE_QHY5III178   4006

/**
 * Type define for QHY5III178C*/
#define DEVICETYPE_QHY5III178C  4007

/**
 * Type define for QHY5III178M*/
#define DEVICETYPE_QHY5III178M  4008

/**
 * Type define for QHY178*/
#define DEVICETYPE_QHY178       4009

/**
 * Type define for QHY178M*/
#define DEVICETYPE_QHY178M      4010

/**
 * Type define for QHY178C*/
#define DEVICETYPE_QHY178C      4011

/**
 * Type define for QHY5III185*/
#define DEVICETYPE_QHY5III185   4012

/**
 * Type define for QHY5III185C*/
#define DEVICETYPE_QHY5III185C  4013

/**
 * Type define for QHY5III185M*/
#define DEVICETYPE_QHY5III185M  4014

/**
 * Type define for QHY185*/
#define DEVICETYPE_QHY185       4015

/**
 * Type define for QHY185M*/
#define DEVICETYPE_QHY185M      4016

/**
 * Type define for QHY185C*/
#define DEVICETYPE_QHY185C      4017

/**
 * Type define for QHY5III224*/
#define DEVICETYPE_QHY5III224   4018

/**
 * Type define for QHY5III224C*/
#define DEVICETYPE_QHY5III224C  4019

/**
 * Type define for QHY5III224M*/
#define DEVICETYPE_QHY5III224M  4020

/**
 * Type define for QHY224*/
#define DEVICETYPE_QHY224       4021

/**
 * Type define for QHY224M*/
#define DEVICETYPE_QHY224M      4022

/**
 * Type define for QHY224C*/
#define DEVICETYPE_QHY224C      4023

/**
 * Type define for QHY5III290*/
#define DEVICETYPE_QHY5III290   4024

/**
 * Type define for QHY5III290C*/
#define DEVICETYPE_QHY5III290C  4025

/**
 * Type define for QHY5III290M*/
#define DEVICETYPE_QHY5III290M  4026

/**
 * Type define for QHY290*/
#define DEVICETYPE_QHY290       4027

/**
 * Type define for QHY290M*/
#define DEVICETYPE_QHY290M      4028

/**
 * Type define for QHY290C*/
#define DEVICETYPE_QHY290C      4029

/**
 * Type define for QHY5III236*/
#define DEVICETYPE_QHY5III236   4030

/**
 * Type define for QHY5III236C*/
#define DEVICETYPE_QHY5III236C  4031

/**
 * Type define for QHY5III290M*/
#define DEVICETYPE_QHY5III236M  4032

/**
 * Type define for QHY236*/
#define DEVICETYPE_QHY236       4033

/**
 * Type define for QHY236M*/
#define DEVICETYPE_QHY236M      4034

/**
 * Type define for QHY236C*/
#define DEVICETYPE_QHY236C      4035

/**
 * Type define for GSENSE400*/
#define DEVICETYPE_QHY5IIIG400M 4036

/**
 * Type define for QHY163*/
#define DEVICETYPE_QHY163       4037

/**
 * Type define for QHY163M*/
#define DEVICETYPE_QHY163M      4038

/**
 * Type define for QHY163C*/
#define DEVICETYPE_QHY163C      4039

/**
 * Type define for QHY165*/
#define DEVICETYPE_QHY165       4040

/**
 * Type define for QHY165C*/
#define DEVICETYPE_QHY165C      4041

/**
 * Type define for QHY367*/
#define DEVICETYPE_QHY367       4042

/**
 * Type define for QHY367C*/
#define DEVICETYPE_QHY367C      4043

/**
 * Type define for QHY183*/
#define DEVICETYPE_QHY183       4044

/**
 * Type define for QHY183C*/
#define DEVICETYPE_QHY183C      4045

/**
 * Type define for QHY-DevelopDev*/
#define DEVICETYPE_QHY5IIICOMMON 4046

/**
 * Type define for QHY247*/
#define DEVICETYPE_QHY247       4047

/**
 * Type define for QHY247C*/
#define DEVICETYPE_QHY247C      4048

/**
 * Type define for MINICAM6F*/
#define DEVICETYPE_MINICAM6F    4049

/**
 * Type define for QHY168C*/
#define DEVICETYPE_QHY168       4050

#define DEVICETYPE_QHY168C      4051

/**
 * Type define for QHY128C*/
#define DEVICETYPE_QHY128       4052

#define DEVICETYPE_QHY128C      4053

/**
 * Type define for QHY5IIIEND*/
#define DEVICETYPE_QHY5IIIEND   4999

/**
 * Type define for QHY16 */
#define DEVICETYPE_QHY16        16

/**
 * Type define for QHY6 */
#define DEVICETYPE_QHY6         60

/**
 * Type define for QHY7 */
#define DEVICETYPE_QHY7         70

/**
 * Type define for QHY2PRO */
#define DEVICETYPE_QHY2PRO      221

/**
 * Type define for IMG2P */
#define DEVICETYPE_IMG2P        220

/**
 * Type define for QHY8 */
#define DEVICETYPE_QHY8         400

/**
 * Type define for QHY8PRO */
#define DEVICETYPE_QHY8PRO      453

/**
 * Type define for QHY16000 */
#define DEVICETYPE_QHY16000     361

/**
 * Type define for QHY12 */
#define DEVICETYPE_QHY12        613

/**
 * Type define for IC8300 */
#define DEVICETYPE_IC8300       890

/**
 * Type define for QHY9S */
#define DEVICETYPE_QHY9S        892

/**
 * Type define for QHY10 */
#define DEVICETYPE_QHY10        893

/**
 * Type define for QHY8L */
#define DEVICETYPE_QHY8L        891

/**
 * Type define for QHY11 */
#define DEVICETYPE_QHY11        894

/**
 * Type define for QHY21 */
#define DEVICETYPE_QHY21        895

/**
 * Type define for QHY22 */
#define DEVICETYPE_QHY22        896

/**
 * Type define for QHY23 */
#define DEVICETYPE_QHY23        897

/**
 * Type define for QHY15 */
#define DEVICETYPE_QHY15        898

/**
 * Type define for QHY27 */
#define DEVICETYPE_QHY27        899


/**
 * Type define for QHY28 */
#define DEVICETYPE_QHY28        902

/**
 * Type define for QHY9T */
#define DEVICETYPE_QHY9T        905

/**
 * Type define for QHY29 */
#define DEVICETYPE_QHY29        907

/**
 * Type define for SOLAR1600 */
#define DEVICETYPE_SOLAR1600    908

/* QHYA seires */

/**
 * Type define for QHY90A/IC90A */
#define DEVICETYPE_90A          900

/**
 * Type define for QHY16200A/IC16200A */
#define DEVICETYPE_16200A       901

/**
 * Type define for QHY814A/IC814A */
#define DEVICETYPE_814A         903

/**
 * Type define for 16803 */
#define DEVICETYPE_16803        906

/**
 * Type define for 695A*/
#define DEVICETYPE_695A         916


/**
 * Type define for QHY15GIGAE */
#define DEVICETYPE_QHY15G       9000

/**
 * Type define for SOLAR800G */
#define DEVICETYPE_SOLAR800G    9001

#define DEVICETYPE_A0340G       9003

#define DEVICETYPE_QHY08050G    9004

#define DEVICETYPE_QHY694G      9005

#define DEVICETYPE_QHY27G       9006

#define DEVICETYPE_QHY23G       9007

#define DEVICETYPE_QHY16000G    9008

#define DEVICETYPE_QHY160002AD  9009

#define DEVICETYPE_QHY814G      9010

#define DEVICETYPE_QHY45GX      9011

#define DEVICETYPE_QHY10_FOCUS  9012

/**
 * Type define for UNKNOW */
#define DEVICETYPE_UNKNOW       -1

#endif
