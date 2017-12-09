/*

  Copyright (c) 2002 Finger Lakes Instrumentation (FLI), L.L.C.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

        Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

        Redistributions in binary form must reproduce the above
        copyright notice, this list of conditions and the following
        disclaimer in the documentation and/or other materials
        provided with the distribution.

        Neither the name of Finger Lakes Instrumentation (FLI), LLC
        nor the names of its contributors may be used to endorse or
        promote products derived from this software without specific
        prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

  ======================================================================

  Finger Lakes Instrumentation, L.L.C. (FLI)
  web: http://www.fli-cam.com
  email: support@fli-cam.com

*/

#ifndef _LIBFLI_CAMERA_USB_H_
#define _LIBFLI_CAMERA_USB_H_

#define FLI_USBCAM_DEVICENAME 0x01
#define FLI_USBCAM_DEVICEMFG 0x02
#define FLI_USBCAM_VERSION 0x03
#define FLI_USBCAM_DEVICEID 0x04
#define FLI_USBCAM_SERIALNUM 0x05
#define FLI_USBCAM_HARDWAREREV 0x06
#define FLI_USBCAM_DEVINIT 0x07
#define FLI_USBCAM_READPARAMBLOCK 0x08

#define FLI_USBCAM_ARRAYSIZE 0x100
#define FLI_USBCAM_IMAGEOFFSET 0x102
#define FLI_USBCAM_IMAGESIZE 0x103
#define FLI_USBCAM_TEMPERATURE 0x104
#define FLI_USBCAM_SETFRAMEOFFSET 0x105
#define FLI_USBCAM_SETBINFACTORS 0x106
#define FLI_USBCAM_SETFLUSHBINFACTORS 0x107
#define FLI_USBCAM_SETEXPOSURE 0x108
#define FLI_USBCAM_STARTEXPOSURE 0x109
#define FLI_USBCAM_ABORTEXPOSURE 0x10a
#define FLI_USBCAM_EXPOSURESTATUS 0x10b
#define FLI_USBCAM_FLUSHROWS 0x10c
#define FLI_USBCAM_SENDROW 0x10d
#define FLI_USBCAM_SETDAC	0x10e
#define FLI_USBCAM_SHUTTER 0x10f
#define FLI_USBCAM_WRITEIO 0x110
#define FLI_USBCAM_READIO 0x111
#define FLI_USBCAM_WRITEDIR 0x112
#define FLI_USBCAM_BGFLUSH 0x114

#define PROLINE_GET_HARDWAREINFO							(0x0001)
#define PROLINE_GET_DEVICESTRINGS							(0x0002)
#define PROLINE_GET_CAMERAINFO								(0x0003)
#define PROLINE_COMMAND_GET_ROW								(0x0004)
#define PROLINE_COMMAND_EXPOSE								(0x0005)
#define PROLINE_COMMAND_GET_EXPOSURE_STATUS		(0x0006)
#define PROLINE_COMMAND_CANCEL_EXPOSURE				(0x0007)
#define PROLINE_COMMAND_GET_TEMPERATURE				(0x0008)
#define PROLINE_COMMAND_SET_TEMPERATURE				(0x0009)
#define PROLINE_COMMAND_SET_SHUTTER						(0x000a)
#define PROLINE_COMMAND_SET_BGFLUSH						(0x000b)
#define PROLINE_COMMAND_GET_STATUS						(0x000c)
#define PROLINE_COMMAND_GET_CURRENT_MODE			(0x000d)
#define PROLINE_COMMAND_GET_MODE_STRING				(0x000e)
#define PROLINE_COMMAND_SET_MODE							(0x000f)
#define PROLINE_COMMAND_CONFIGURE_IOPORT			(0x0010)
#define PROLINE_COMMAND_WRITE_IOPORT					(0x0011)
#define PROLINE_COMMAND_READ_IOPORT						(0x0012)
#define PROLINE_COMMAND_SET_TDI_MODE					(0x0013)
#define PROLINE_COMMAND_UPDATE_EXPOSURE			  (0x0015)
#define PROLINE_COMMAND_SET_FAN_SPEED					(0x0016)

long fli_camera_usb_open(flidev_t dev);
long fli_camera_usb_get_array_area(flidev_t dev, long *ul_x, long *ul_y,
				   long *lr_x, long *lr_y);
long fli_camera_usb_get_visible_area(flidev_t dev, long *ul_x, long *ul_y,
				     long *lr_x, long *lr_y);
long fli_camera_usb_set_exposure_time(flidev_t dev, unsigned long exptime);
long fli_camera_usb_set_image_area(flidev_t dev, long ul_x, long ul_y,
				   long lr_x, long lr_y);
long fli_camera_usb_set_hbin(flidev_t dev, long hbin);
long fli_camera_usb_set_vbin(flidev_t dev, long vbin);
long fli_camera_usb_get_exposure_status(flidev_t dev, long *timeleft);
long fli_camera_usb_cancel_exposure(flidev_t dev);
long fli_camera_usb_set_temperature(flidev_t dev, double temperature);
long fli_camera_usb_get_temperature(flidev_t dev, double *temperature);
long fli_camera_usb_grab_row(flidev_t dev, void *buff, size_t width);
long fli_camera_usb_expose_frame(flidev_t dev);
long fli_camera_usb_flush_rows(flidev_t dev, long rows, long repeat);
long fli_camera_usb_set_bit_depth(flidev_t dev, flibitdepth_t bitdepth);
long fli_camera_usb_read_ioport(flidev_t dev, long *ioportset);
long fli_camera_usb_write_ioport(flidev_t dev, long ioportset);
long fli_camera_usb_configure_ioport(flidev_t dev, long ioportset);
long fli_camera_usb_control_shutter(flidev_t dev, long shutter);
long fli_camera_usb_control_bgflush(flidev_t dev, long bgflush);
long fli_camera_usb_set_dac(flidev_t dev, unsigned long dacset);
long fli_camera_usb_get_cooler_power(flidev_t dev, double *power);
long fli_camera_usb_get_camera_status(flidev_t dev, long *camera_status);
long fli_camera_usb_get_camera_mode_string(flidev_t dev, flimode_t camera_mode, char *dest, size_t siz);
long fli_camera_usb_set_camera_mode(flidev_t dev, flimode_t camera_mode);
long fli_camera_usb_get_camera_mode(flidev_t dev, flimode_t *camera_mode);
long fli_camera_usb_read_temperature(flidev_t dev, flichannel_t channel, double *temperature);
long fli_camera_usb_set_tdi(flidev_t dev, flitdirate_t rate, flitdiflags_t flags);
long fli_camera_usb_start_video_mode(flidev_t dev);
long fli_camera_usb_stop_video_mode(flidev_t dev);
long fli_camera_usb_start_video_mode(flidev_t dev);
long fli_camera_usb_grab_video_frame(flidev_t dev, void *buff, size_t size);
long fli_camera_usb_end_exposure(flidev_t dev);
long fli_camera_usb_trigger_exposure(flidev_t dev);
long fli_camera_usb_set_fan_speed(flidev_t dev, long fan_speed);

#endif /* _LIBFLI_CAMERA_USB_H_ */
