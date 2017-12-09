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
 * @file qhyccd.h
 * @brief QHYCCD SDK interface for programmers
 */
#include "qhyccderr.h"
#include "qhyccdcamdef.h"
#include "qhyccdstruct.h"
#include "stdint.h"

#ifdef WIN32
#include "cyapi.h"
#endif

#ifndef __QHYCCD_H__
#define __QHYCCD_H__

#if defined (WIN32)
typedef CCyUSBDevice qhyccd_handle;
#elif defined (LINUX)
typedef struct libusb_device_handle qhyccd_handle;
#endif

/** \fn uint32_t InitQHYCCDResource()
      \brief initialize QHYCCD SDK resource
      \return 
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_INITRESOURCE if the initialize failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL InitQHYCCDResource(void);

/** \fn uint32_t ReleaseQHYCCDResource()
      \brief release QHYCCD SDK resource
      \return 
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_RELEASERESOURCE if the release failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ReleaseQHYCCDResource(void);

/** \fn uint32_t ScanQHYCCD()
      \brief scan the connected cameras
	  \return
	  on success,return the number of connected cameras \n
	  QHYCCD_ERROR_NO_DEVICE,if no camera connect to computer
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ScanQHYCCD(void);

/** \fn uint32_t GetQHYCCDId(uint32_t index,char *id)
      \brief get the id from camera
	  \param index sequence number of the connected cameras
	  \param id the id for camera,each camera has only id
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDId(uint32_t index,char *id);

/** \fn uint32_t GetQHYCCDModel(char *id, char *model)
      \brief get camera model name by id
          \param id the id of the camera
          \param model the camera model
          \return
          on success,return QHYCCD_SUCCESS \n
          another QHYCCD_ERROR code in failure
  */
EXPORTC uint32_t STDCALL GetQHYCCDModel(char *id, char *model);

/** \fn qhyccd_handle *OpenQHYCCD(char *id)
      \brief open camera by camera id
	  \param id the id for camera,each camera has only id
      \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC qhyccd_handle * STDCALL OpenQHYCCD(char *id);

/** \fn uint32_t CloseQHYCCD(qhyccd_handle *handle)
      \brief close camera by handle
	  \param handle camera handle
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL CloseQHYCCD(qhyccd_handle *handle);

/**
 @fn uint32_t SetQHYCCDStreamMode(qhyccd_handle *handle,uint8_t mode)
 @brief Set the camera's mode to chose the way reading data from camera
 @param handle camera control handle
 @param mode the stream mode \n
 0x00:default mode,single frame mode \n
 0x01:live mode \n
 @return
 on success,return QHYCCD_SUCCESS \n
 another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL SetQHYCCDStreamMode(qhyccd_handle *handle,uint8_t mode);

/** \fn uint32_t InitQHYCCD(qhyccd_handle *handle)
      \brief initialization specified camera by camera handle
	  \param handle camera control handle
      \return
	  on success,return QHYCCD_SUCCESS \n
	  on failed,return QHYCCD_ERROR_INITCAMERA \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL InitQHYCCD(qhyccd_handle *handle);

/** @fn uint32_t IsQHYCCDControlAvailable(qhyccd_handle *handle,CONTROL_ID controlId)
    @brief check the camera has the queried function or not
    @param handle camera control handle
    @param controlId function type
    @return
	  on have,return QHYCCD_SUCCESS \n
	  on do not have,return QHYCCD_ERROR_NOTSUPPORT \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL IsQHYCCDControlAvailable(qhyccd_handle *handle,CONTROL_ID controlId);

/** \fn uint32_t SetQHYCCDParam(qhyccd_handle *handle,CONTROL_ID controlId,double value)
      \brief set params to camera
      \param handle camera control handle
      \param controlId function type
	  \param value value to camera
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_NOTSUPPORT,if the camera do not have the function \n
	  QHYCCD_ERROR_SETPARAMS,if set params to camera failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL SetQHYCCDParam(qhyccd_handle *handle,CONTROL_ID controlId, double value);

/** \fn double GetQHYCCDParam(qhyccd_handle *handle,CONTROL_ID controlId)
      \brief get the params value from camera
      \param handle camera control handle
      \param controlId function type
	  \return
	  on success,return the value\n
	  QHYCCD_ERROR_NOTSUPPORT,if the camera do not have the function \n
	  QHYCCD_ERROR_GETPARAMS,if get camera params'value failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC double STDCALL GetQHYCCDParam(qhyccd_handle *handle,CONTROL_ID controlId);

/** \fn uint32_t GetQHYCCDParamMinMaxStep(qhyccd_handle *handle,CONTROL_ID controlId,double *min,double *max,double *step)
      \brief get the params value from camera
      \param handle camera control handle
      \param controlId function type
	  \param *min the pointer to the function's min value
	  \param *max the pointer to the function's max value
	  \param *step the pointer to the function's single step value
	  \return
	  on success,return QHYCCD_SUCCESS\n
	  QHYCCD_ERROR_NOTSUPPORT,if the camera do not have the function \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDParamMinMaxStep(qhyccd_handle *handle,CONTROL_ID controlId,double *min,double *max,double *step);

/** @fn uint32_t SetQHYCCDResolution(qhyccd_handle *handle,uint32_t x,uint32_t y,uint32_t xsize,uint32_t ysize)
    @brief set camera ouput resolution
    @param handle camera control handle
    @param x the top left position x
    @param y the top left position y
    @param xsize the image width
    @param ysize the image height
    @return
        on success,return QHYCCD_SUCCESS\n
        another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL SetQHYCCDResolution(qhyccd_handle *handle,uint32_t x,uint32_t y,uint32_t xsize,uint32_t ysize);

/** \fn uint32_t GetQHYCCDMemLength(qhyccd_handle *handle)
      \brief get the minimum memory space for image data to save(byte)
      \param handle camera control handle
      \return 
	  on success,return the total memory space for image data(byte) \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDMemLength(qhyccd_handle *handle);

/** \fn uint32_t ExpQHYCCDSingleFrame(qhyccd_handle *handle)
      \brief start to expose one frame
      \param handle camera control handle
      \return
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_EXPOSING,if the camera is exposing \n
	  QHYCCD_ERROR_EXPFAILED,if start failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ExpQHYCCDSingleFrame(qhyccd_handle *handle);

/**
   @fn uint32_t GetQHYCCDSingleFrame(qhyccd_handle *handle,uint32_t *w,uint32_t *h,uint32_t *bpp,uint32_t *channels,uint8_t *imgdata)
   @brief get live frame data from camera
   @param handle camera control handle
   @param *w pointer to width of ouput image
   @param *h pointer to height of ouput image
   @param *bpp pointer to depth of ouput image
   @param *channels pointer to channels of ouput image
   @param *imgdata image data buffer
   @return
   on success,return QHYCCD_SUCCESS \n
   QHYCCD_ERROR_GETTINGFAILED,if get data failed \n
   another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL GetQHYCCDSingleFrame(qhyccd_handle *handle,uint32_t *w,uint32_t *h,uint32_t *bpp,uint32_t *channels,uint8_t *imgdata);

/**
  @fn uint32_t CancelQHYCCDExposing(qhyccd_handle *handle)
  @param handle camera control handle
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL CancelQHYCCDExposing(qhyccd_handle *handle);

/** 
  @fn uint32_t CancelQHYCCDExposingAndReadout(qhyccd_handle *handle)
  @brief stop the camera exposing and readout
  @param handle camera control handle
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL CancelQHYCCDExposingAndReadout(qhyccd_handle *handle);

/** \fn uint32_t BeginQHYCCDLive(qhyccd_handle *handle)
      \brief start continue exposing
	  \param handle camera control handle
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL BeginQHYCCDLive(qhyccd_handle *handle);

/**   
      @fn uint32_t GetQHYCCDLiveFrame(qhyccd_handle *handle,uint32_t *w,uint32_t *h,uint32_t *bpp,uint32_t *channels,uint8_t *imgdata)
      @brief get live frame data from camera
	  @param handle camera control handle
	  @param *w pointer to width of ouput image
	  @param *h pointer to height of ouput image
      @param *bpp pointer to depth of ouput image
      @param *channels pointer to channels of ouput image
      @param *imgdata image data buffer
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_GETTINGFAILED,if get data failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDLiveFrame(qhyccd_handle *handle,uint32_t *w,uint32_t *h,uint32_t *bpp,uint32_t *channels,uint8_t *imgdata);

/** \fn uint32_t StopQHYCCDLive(qhyccd_handle *handle)
      \brief stop the camera continue exposing
	  \param handle camera control handle
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL StopQHYCCDLive(qhyccd_handle *handle);

/** \
  @fn uint32_t SetQHYCCDBinMode(qhyccd_handle *handle,uint32_t wbin,uint32_t hbin)
  @brief set camera's bin mode for ouput image data
  @param handle camera control handle
  @param wbin width bin mode
  @param hbin height bin mode
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL SetQHYCCDBinMode(qhyccd_handle *handle,uint32_t wbin,uint32_t hbin);

/**
   @fn uint32_t SetQHYCCDBitsMode(qhyccd_handle *handle,uint32_t bits)
   @brief set camera's depth bits for ouput image data
   @param handle camera control handle
   @param bits image depth
   @return
   on success,return QHYCCD_SUCCESS \n
   another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL SetQHYCCDBitsMode(qhyccd_handle *handle,uint32_t bits);

/** \fn uint32_t ControlQHYCCDTemp(qhyccd_handle *handle,double targettemp)
      \brief This is a auto temprature control for QHYCCD cameras. \n
             To control this,you need call this api every single second
          \param handle camera control handle
          \param targettemp the target control temprature
          \return
          on success,return QHYCCD_SUCCESS \n
          another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ControlQHYCCDTemp(qhyccd_handle *handle,double targettemp);

/** \fn uint32_t ControlQHYCCDGuide(qhyccd_handle *handle,uint32_t direction,uint16_t duration)
      \brief control the camera' guide port
	  \param handle camera control handle
	  \param direction direction \n
           0: EAST RA+   \n
           3: WEST RA-   \n
           1: NORTH DEC+ \n 
           2: SOUTH DEC- \n
	  \param duration duration of the direction
	  \return 
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ControlQHYCCDGuide(qhyccd_handle *handle,uint32_t direction,uint16_t duration);

	 /** 
	  @fn uint32_t SendOrder2QHYCCDCFW(qhyccd_handle *handle,char *order,uint32_t length)
      @brief control color filter wheel port 
      @param handle camera control handle
	  @param order order send to color filter wheel
	  @param length the order string length
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
    */
EXPORTC uint32_t STDCALL SendOrder2QHYCCDCFW(qhyccd_handle *handle,char *order,uint32_t length);

	 /** 
	  @fn 	uint32_t GetQHYCCDCFWStatus(qhyccd_handle *handle,char *status)
      @brief control color filter wheel port 
      @param handle camera control handle
	  @param status the color filter wheel position status
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
    */
EXPORTC	uint32_t STDCALL GetQHYCCDCFWStatus(qhyccd_handle *handle,char *status);

	 /** 
	  @fn 	uint32_t IsQHYCCDCFWPlugged(qhyccd_handle *handle)
      @brief control color filter wheel port 
      @param handle camera control handle
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
    */
EXPORTC	uint32_t STDCALL IsQHYCCDCFWPlugged(qhyccd_handle *handle);

     /** 
      \fn   uint32_t SetQHYCCDTrigerMode(qhyccd_handle *handle,uint32_t trigerMode)
      \brief set camera triger mode
      \param handle camera control handle
      \param trigerMode triger mode
      \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL SetQHYCCDTrigerMode(qhyccd_handle *handle,uint32_t trigerMode);

/** \fn void Bits16ToBits8(qhyccd_handle *h,uint8_t *InputData16,uint8_t *OutputData8,uint32_t imageX,uint32_t imageY,uint16_t B,uint16_t W)
      \brief turn 16bits data into 8bits
      \param h camera control handle
      \param InputData16 for 16bits data memory
      \param OutputData8 for 8bits data memory
      \param imageX image width
      \param imageY image height
      \param B for stretch balck
      \param W for stretch white
  */
EXPORTC void STDCALL Bits16ToBits8(qhyccd_handle *h,uint8_t *InputData16,uint8_t *OutputData8,uint32_t imageX,uint32_t imageY,uint16_t B,uint16_t W);

/**
   @fn void HistInfo192x130(qhyccd_handle *h,uint32_t x,uint32_t y,uint8_t *InBuf,uint8_t *OutBuf)
   @brief make the hist info
   @param h camera control handle
   @param x image width
   @param y image height
   @param InBuf for the raw image data
   @param OutBuf for 192x130 8bits 3 channels image
  */
EXPORTC void  STDCALL HistInfo192x130(qhyccd_handle *h,uint32_t x,uint32_t y,uint8_t *InBuf,uint8_t *OutBuf);


/** 
    @fn uint32_t OSXInitQHYCCDFirmware(char *path)
    @brief download the firmware to camera.(this api just need call in OSX system)
    @param path path to HEX file
  */
EXPORTC uint32_t STDCALL OSXInitQHYCCDFirmware(char *path);



/** @fn uint32_t GetQHYCCDChipInfo(qhyccd_handle *h,double *chipw,double *chiph,uint32_t *imagew,uint32_t *imageh,double *pixelw,double *pixelh,uint32_t *bpp)
      @brief get the camera's ccd/cmos chip info
      @param h camera control handle
      @param chipw chip size width
      @param chiph chip size height
      @param imagew chip output image width
      @param imageh chip output image height
      @param pixelw chip pixel size width
      @param pixelh chip pixel size height
      @param bpp chip pixel depth
  */
EXPORTC uint32_t STDCALL GetQHYCCDChipInfo(qhyccd_handle *h,double *chipw,double *chiph,uint32_t *imagew,uint32_t *imageh,double *pixelw,double *pixelh,uint32_t *bpp);

/** @fn uint32_t GetQHYCCDEffectiveArea(qhyccd_handle *h,uint32_t *startX, uint32_t *startY, uint32_t *sizeX, uint32_t *sizeY)
      @brief get the camera's ccd/cmos chip info
      @param h camera control handle
      @param startX the Effective area x position
      @param startY the Effective area y position
      @param sizeX the Effective area x size
      @param sizeY the Effective area y size
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDEffectiveArea(qhyccd_handle *h,uint32_t *startX, uint32_t *startY, uint32_t *sizeX, uint32_t *sizeY);

/** @fn uint32_t GetQHYCCDOverScanArea(qhyccd_handle *h,uint32_t *startX, uint32_t *startY, uint32_t *sizeX, uint32_t *sizeY)
      @brief get the camera's ccd/cmos chip info
      @param h camera control handle
      @param startX the OverScan area x position
      @param startY the OverScan area y position
      @param sizeX the OverScan area x size
      @param sizeY the OverScan area y size
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDOverScanArea(qhyccd_handle *h,uint32_t *startX, uint32_t *startY, uint32_t *sizeX, uint32_t *sizeY);


/** @fn uint32_t SetQHYCCDFocusSetting(qhyccd_handle *h,uint32_t focusCenterX, uint32_t focusCenterY)
      @brief Set the camera on focus mode
      @param h camera control handle
      @param focusCenterX
      @param focusCenterY
      @return
	  on success,return QHYCCD_SUCCESS \n

	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL SetQHYCCDFocusSetting(qhyccd_handle *h,uint32_t focusCenterX, uint32_t focusCenterY);

/** @fn uint32_t GetQHYCCDExposureRemaining(qhyccd_handle *h)
      @brief Get remaining ccd/cmos expose time
      @param h camera control handle
      @return
      100 or less 100,it means exposoure is over \n
      another is remaining time
 */
EXPORTC uint32_t STDCALL GetQHYCCDExposureRemaining(qhyccd_handle *h);

/** @fn uint32_t GetQHYCCDFWVersion(qhyccd_handle *h,uint8_t *buf)
      @brief Get the QHYCCD's firmware version
      @param h camera control handle
	  @param buf buffer for version info
      @return
	  on success,return QHYCCD_SUCCESS \n

	  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL GetQHYCCDFWVersion(qhyccd_handle *h,uint8_t *buf);

/** @fn uint32_t SetQHYCCDInterCamSerialParam(qhyccd_handle *h,uint32_t opt)
      @brief Set InterCam serial2 params
      @param h camera control handle
	  @param opt the param \n
	  opt: \n
	   0x00 baud rate 9600bps  8N1 \n
       0x01 baud rate 4800bps  8N1 \n
       0x02 baud rate 19200bps 8N1 \n
       0x03 baud rate 28800bps 8N1 \n
       0x04 baud rate 57600bps 8N1
      @return
	  on success,return QHYCCD_SUCCESS \n

	  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL SetQHYCCDInterCamSerialParam(qhyccd_handle *h,uint32_t opt);

/** @fn uint32_t QHYCCDInterCamSerialTX(qhyccd_handle *h,char *buf,uint32_t length)
      @brief Send data to InterCam serial2
      @param h camera control handle
	  @param buf buffer for data
	  @param length to send
      @return
	  on success,return QHYCCD_SUCCESS \n

	  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL QHYCCDInterCamSerialTX(qhyccd_handle *h,char *buf,uint32_t length);

/** @fn uint32_t QHYCCDInterCamSerialRX(qhyccd_handle *h,char *buf)
      @brief Get data from InterCam serial2
      @param h camera control handle
	  @param buf buffer for data
      @return
	  on success,return the data number \n

	  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL QHYCCDInterCamSerialRX(qhyccd_handle *h,char *buf);

	/** @fn uint32_t QHYCCDInterCamOledOnOff(qhyccd_handle *handle,uint8_t onoff)
      @brief turn off or turn on the InterCam's Oled
      @param handle camera control handle
	  @param onoff on or off the oled \n
	  1:on \n
	  0:off \n
      @return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
    */
EXPORTC uint32_t STDCALL QHYCCDInterCamOledOnOff(qhyccd_handle *handle,uint8_t onoff);

/** 
  @fn uint32_t SetQHYCCDInterCamOledBrightness(qhyccd_handle *handle,uint8_t brightness)
  @brief send data to show on InterCam's OLED
  @param handle camera control handle
  @param brightness the oled's brightness
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL SetQHYCCDInterCamOledBrightness(qhyccd_handle *handle,uint8_t brightness);

/** 
  @fn uint32_t SendFourLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messagetemp,char *messageinfo,char *messagetime,char *messagemode)
  @brief spilit the message to two line,send to camera
  @param handle camera control handle
  @param messagetemp message for the oled's 1st line
  @param messageinfo message for the oled's 2nd line
  @param messagetime message for the oled's 3rd line
  @param messagemode message for the oled's 4th line
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL SendFourLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messagetemp,char *messageinfo,char *messagetime,char *messagemode);
/** 
  @fn uint32_t SendTwoLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messageTop,char *messageBottom)
  @brief spilit the message to two line,send to camera
  @param handle camera control handle
  @param messageTop message for the oled's 1st line
  @param messageBottom message for the oled's 2nd line
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL SendTwoLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messageTop,char *messageBottom);

/** 
  @fn uint32_t SendOneLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messageTop)
  @brief spilit the message to two line,send to camera
  @param handle camera control handle
  @param messageTop message for all the oled
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/  
EXPORTC uint32_t STDCALL SendOneLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messageTop);

/** 
  @fn uint32_t GetQHYCCDCameraStatus(qhyccd_handle *h,uint8_t *buf)
  @brief Get the camera statu
  @param h camera control handle
  @param buf camera's status save space
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL GetQHYCCDCameraStatus(qhyccd_handle *h,uint8_t *buf);

 /** 
  @fn uint32_t GetQHYCCDShutterStatus(qhyccd_handle *handle)
  @brief get the camera's shutter status 
  @param handle camera control handle
  @return
  on success,return status \n
  0x00:shutter turn to right \n
  0x01:shutter from right turn to middle \n
  0x02:shutter from left turn to middle \n
  0x03:shutter turn to left \n
  0xff:IDLE \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL GetQHYCCDShutterStatus(qhyccd_handle *handle);

/** 
  @fn uint32_t ControlQHYCCDShutter(qhyccd_handle *handle,uint8_t status)
  @brief control camera's shutter
  @param handle camera control handle
  @param status the shutter status \n
  0x00:shutter turn to right \n
  0x01:shutter from right turn to middle \n
  0x02:shutter from left turn to middle \n
  0x03:shutter turn to left \n
  0xff:IDLE
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL ControlQHYCCDShutter(qhyccd_handle *handle,uint8_t status);

/** 
  @fn uint32_t GetQHYCCDHumidity(qhyccd_handle *handle,double *hd)
  @brief query cavity's humidity 
  @param handle control handle
  @param hd the humidity value
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures 
*/
EXPORTC uint32_t STDCALL GetQHYCCDHumidity(qhyccd_handle *handle,double *hd);

/** 
  @fn uint32_t QHYCCDI2CTwoWrite(qhyccd_handle *handle,uint16_t addr,uint16_t value)
  @brief Set the value of the addr register in the camera.
  @param handle camera control handle
  @param addr the address of register
  @param value the value of the address
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL QHYCCDI2CTwoWrite(qhyccd_handle *handle,uint16_t addr,uint16_t value);
	
/** 
  @fn uint32_t QHYCCDI2CTwoRead(qhyccd_handle *handle,uint16_t addr)
  @brief Get the value of the addr register in the camera.
  @param handle camera control handle
  @param addr the address of register
  @return value of the addr register
*/
EXPORTC uint32_t STDCALL QHYCCDI2CTwoRead(qhyccd_handle *handle,uint16_t addr);

/** 
  @fn double GetQHYCCDReadingProgress(qhyccd_handle *handle)
  @brief get reading data from camera progress
  @param handle camera control handle
  @return current progress
*/
EXPORTC double STDCALL GetQHYCCDReadingProgress(qhyccd_handle *handle);

/**
  @fn void SetGetQHYCCDLogLevel(qhyccd_handle *handle)
  @param Set logger level 	LOG_LEVEL_TRACE, LOG_LEVEL_DEBUG, LOG_LEVEL_INFO, 	LOG_LEVEL_WARN, LOG_LEVEL_ERROR, LOG_LEVEL_ALARM, LOG_LEVEL_FATAL,
*/
EXPORTC void STDCALL SetQHYCCDLogLevel(uint8_t logLevel);

/**
  test pid parameters
*/
EXPORTC uint32_t STDCALL TestQHYCCDPIDParas(qhyccd_handle *h, double p, double i, double d);

EXPORTC uint32_t STDCALL SetQHYCCDTrigerFunction(qhyccd_handle *h,bool value);

EXPORTC uint32_t STDCALL DownloadFX3FirmWare(uint16_t vid,uint16_t pid,char *imgpath);

EXPORTC uint32_t STDCALL GetQHYCCDType(qhyccd_handle *h);

EXPORTC uint32_t STDCALL SetQHYCCDDebayerOnOff(qhyccd_handle *h,bool onoff);

EXPORTC uint32_t STDCALL SetQHYCCDFineTone(qhyccd_handle *h,uint8_t setshporshd,uint8_t shdloc,uint8_t shploc,uint8_t shwidth);

EXPORTC uint32_t STDCALL SetQHYCCDGPSVCOXFreq(qhyccd_handle *handle,uint16_t i);

EXPORTC uint32_t STDCALL SetQHYCCDGPSLedCalMode(qhyccd_handle *handle,uint8_t i);

EXPORTC void STDCALL SetQHYCCDGPSLedCal(qhyccd_handle *handle,uint32_t pos,uint8_t width);

EXPORTC void STDCALL SetQHYCCDGPSPOSA(qhyccd_handle *handle,uint8_t is_slave,uint32_t pos,uint8_t width);

EXPORTC void STDCALL SetQHYCCDGPSPOSB(qhyccd_handle *handle,uint8_t is_slave,uint32_t pos,uint8_t width);

EXPORTC uint32_t STDCALL SetQHYCCDGPSMasterSlave(qhyccd_handle *handle,uint8_t i);

EXPORTC void STDCALL SetQHYCCDGPSSlaveModeParameter(qhyccd_handle *handle,uint32_t target_sec,uint32_t target_us,uint32_t deltaT_sec,uint32_t deltaT_us,uint32_t expTime);

EXPORTC uint32_t STDCALL QHYCCDVendRequestWrite(qhyccd_handle *h,uint8_t req,uint16_t value,uint16_t index1,uint32_t length,uint8_t *data);

EXPORTC uint32_t STDCALL QHYCCDReadUSB_SYNC(qhyccd_handle *pDevHandle, uint8_t endpoint, uint32_t length, uint8_t *data, uint32_t timeout);

EXPORTC uint32_t STDCALL QHYCCDLibusbBulkTransfer(qhyccd_handle *pDevHandle, uint8_t endpoint, uint8_t *data, uint32_t length, int32_t *transferred, uint32_t timeout);

EXPORTC uint32_t STDCALL GetQHYCCDSDKVersion(uint32_t *year,uint32_t *month,uint32_t *day,uint32_t *subday);

#endif
