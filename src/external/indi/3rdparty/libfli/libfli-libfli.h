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

#ifndef _LIBFLI_LIBFLI_H_
#define _LIBFLI_LIBFLI_H_

//#define OLDUSBDRIVER

#include <string.h>

#ifdef DEFINELONG
#define LIBFLIAPI long __stdcall
#endif

#ifdef _WIN32
#ifndef LIBFLIAPI
#define LIBFLIAPI __declspec(dllexport) long __stdcall
#endif
#define stricmp _stricmp
#endif

#include "libfli.h"
#include "libfli-sys.h"
#include "libfli-debug.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define __STRINGIFY(x) ___STRINGIFY(x)
#define ___STRINGIFY(x) #x

#define __LIBFLIVER_MAJOR__ 1
#define __LIBFLIVER__ __STRINGIFY(__LIBFLIVER_MAJOR__) "."	\
  __STRINGIFY(__LIBFLI_MINOR__)

#define CHKDEVICE(xdev)				\
  do {								\
    if((xdev < 0) || (xdev >= MAX_OPEN_DEVICES))		\
    {								\
      debug(FLIDEBUG_WARN,					\
	    "[%s] Attempt to use a device out of range (%d)", __FUNCTION__, xdev);	\
      return -EINVAL;						\
    }								\
    if(devices[xdev] == NULL)					\
    {								\
      debug(FLIDEBUG_WARN,					\
	    "[%s] Attempt to use a NULL device (%d)", __FUNCTION__, xdev);		\
      return -EINVAL;						\
    }								\
  } while(0)

#define CHKFUNCTION(func)					\
  do {								\
    if(func == NULL)						\
    {								\
      debug(FLIDEBUG_WARN,					\
	    "Attempt to use a NULL function (" #func ")");	\
      return -EINVAL;						\
    }								\
  } while(0)

#define IO(dev, buf, wlen, rlen)				\
  do {								\
    int err;							\
    if((err = devices[dev]->fli_io(dev, buf, wlen, rlen)))	\
    {								\
      debug(FLIDEBUG_WARN, "Communication error: %d [%s]",	\
	    err, strerror(-err));				\
      return err;						\
    }								\
  } while(0)

#define COMMAND(function)					\
  do {								\
    int err;							\
    if((err = function))					\
    {								\
      debug(FLIDEBUG_WARN,					\
	    "Function `" #function "' failed, error: %d [%s]",	\
	    err, strerror(-err));				\
      return err;						\
    }								\
  } while(0)

#define MSW(x) ((unsigned short) ((x >> 16) & 0xffff))
#define LSW(x) ((unsigned short) (x & 0xffff))
#define MSB(x) ((unsigned char) ((x >> 8) & 0xff))
#define LSB(x) ((unsigned char) (x & 0xff))

#define IOBUF_MAX_SIZ (64)
typedef unsigned char iobuf_t;

#define IOREAD_U16L(b, i, y) { y = (*(b + i + 1) << 8) | *(b + i); }

#define IOREAD_U8(b, i, y)  { y = *(b + i); }
#define IOREAD_U16(b, i, y) { y = (*(b + i) << 8) | *(b + i + 1); }
#define IOREAD_U32(b, i, y) { y = (*(b + i) << 24) | *(b + i + 1) << 16 | \
																 *(b + i + 2) << 8 | *(b + i + 3); }
#define IOWRITE_U8(b, i, y)  { *(b + i) = (unsigned char) y; }
#define IOWRITE_U16(b, i, y) { *(b + i) = MSB(y); *(b + i + 1) = LSB(y); }
#define IOWRITE_U32(b, i, y) { *(b + i) = MSB(MSW(y)); *(b + i + 1) = LSB(MSW(y)); \
																 *(b + i + 2) = MSB(LSW(y)); *(b + i + 3) = LSB(LSW(y)); }
#define IOREAD_LF(b, i, y) { y = dconvert(b + i); }

#define FLIUSB_VENDORID 0xf18
#define FLIUSB_CAM_ID 0x02
#define FLIUSB_PROLINE_ID 0x0a
#define FLIUSB_FILTER_ID 0x07
#define FLIUSB_FOCUSER_ID 0x06
#define FLIUSB_CFW4_ID 0x0b
#define FLIUSB_PDF2_ID 0x0c
#define FLIUSB_GUIDER_ID 0x0d

#define MAX_OPEN_DEVICES (32)
#define MAX_SEARCH_LIST (16)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Common device information */
typedef struct _flidevinfo_t {
  long type;
  long fwrev;
  long hwrev;
  long devid;
  long serno;
  char *model;
  char *devnam;
	char *serial;
} flidevinfo_t;

/* A specific device instance */
typedef struct _flidevdesc_t {
  char *name;			/* The device name */
  long domain;			/* The device's domain */
  flidevinfo_t devinfo;		/* Device information */
  long io_timeout;		/* Timeout in msec for all I/O */
  void *io_data;		/* For holding I/O specific data */
  void *device_data;		/* For holding device specific data */
  void *sys_data;		/* For holding system specific data */

  /* System-specific functions */
  long (*fli_lock)(flidev_t dev);
  long (*fli_unlock)(flidev_t dev);

  /* Domain-specific functions */
  long (*fli_io)(flidev_t dev, void *buf, long *wlen, long *rlen);

  /* Device-specific functions */
  long (*fli_open)(flidev_t dev);
  long (*fli_close)(flidev_t dev);
  long (*fli_command)(flidev_t dev, int cmd, int argc, ...);
} flidevdesc_t;

extern const char* version;

extern flidevdesc_t *devices[MAX_OPEN_DEVICES];
#define DEVICE devices[dev]

/* Device commands, the format is FLI_COMMAND(<command name>, <number of args>) */
#define FLI_COMMANDS				\
  FLI_COMMAND(FLI_NONE, 0)			\
  FLI_COMMAND(FLI_GET_PIXEL_SIZE, 2)		\
  FLI_COMMAND(FLI_GET_ARRAY_AREA, 4)		\
  FLI_COMMAND(FLI_GET_VISIBLE_AREA, 4)		\
  FLI_COMMAND(FLI_SET_EXPOSURE_TIME, 1)		\
  FLI_COMMAND(FLI_SET_IMAGE_AREA, 4)		\
  FLI_COMMAND(FLI_SET_HBIN, 1)			\
  FLI_COMMAND(FLI_SET_VBIN, 1)			\
  FLI_COMMAND(FLI_SET_FRAME_TYPE, 1)		\
  FLI_COMMAND(FLI_CANCEL_EXPOSURE, 0)		\
  FLI_COMMAND(FLI_GET_EXPOSURE_STATUS, 1)	\
  FLI_COMMAND(FLI_SET_TEMPERATURE, 1)		\
  FLI_COMMAND(FLI_GET_TEMPERATURE, 1)		\
  FLI_COMMAND(FLI_GRAB_ROW, 2)			\
  FLI_COMMAND(FLI_EXPOSE_FRAME, 0)		\
  FLI_COMMAND(FLI_FLUSH_ROWS, 2)		\
  FLI_COMMAND(FLI_SET_FLUSHES, 1)		\
  FLI_COMMAND(FLI_SET_BIT_DEPTH, 1)		\
  FLI_COMMAND(FLI_READ_IOPORT, 1)		\
  FLI_COMMAND(FLI_WRITE_IOPORT, 1)		\
  FLI_COMMAND(FLI_CONFIGURE_IOPORT, 1)		\
  FLI_COMMAND(FLI_CONTROL_SHUTTER, 1)		\
	FLI_COMMAND(FLI_CONTROL_BGFLUSH, 1)   \
	FLI_COMMAND(FLI_SET_DAC, 1)   \
  FLI_COMMAND(FLI_SET_FILTER_POS, 1)		\
  FLI_COMMAND(FLI_GET_FILTER_POS, 1)		\
  FLI_COMMAND(FLI_GET_STEPS_REMAINING, 1)		\
  FLI_COMMAND(FLI_GET_FILTER_COUNT, 1)		\
  FLI_COMMAND(FLI_STEP_MOTOR, 1)		\
  FLI_COMMAND(FLI_STEP_MOTOR_ASYNC, 1)		\
  FLI_COMMAND(FLI_GET_STEPPER_POS, 1)		\
  FLI_COMMAND(FLI_GET_FOCUSER_EXTENT, 1)		\
  FLI_COMMAND(FLI_READ_TEMPERATURE, 2)		\
  FLI_COMMAND(FLI_HOME_FOCUSER, 0) \
  FLI_COMMAND(FLI_HOME_DEVICE, 0) \
  FLI_COMMAND(FLI_GET_COOLER_POWER, 1)		\
  FLI_COMMAND(FLI_SET_CAMERA_MODE, 1)		\
  FLI_COMMAND(FLI_GET_CAMERA_MODE_STRING, 2)		\
  FLI_COMMAND(FLI_GET_CAMERA_MODE, 1)		\
  FLI_COMMAND(FLI_GET_SERIAL_STRING, 2)		\
  FLI_COMMAND(FLI_SET_CAMERA_GAIN, 1)		\
  FLI_COMMAND(FLI_GET_CAMERA_GAIN, 1)		\
  FLI_COMMAND(FLI_SET_CAMERA_OFFSET, 1)		\
  FLI_COMMAND(FLI_GET_CAMERA_OFFSET, 1)		\
	FLI_COMMAND(FLI_SET_TDI, 2) \
	FLI_COMMAND(FLI_GET_STATUS, 1)  \
	FLI_COMMAND(FLI_START_VIDEO_MODE, 0)  \
	FLI_COMMAND(FLI_STOP_VIDEO_MODE, 0)  \
	FLI_COMMAND(FLI_GRAB_VIDEO_FRAME, 2)  \
	FLI_COMMAND(FLI_END_EXPOSURE, 0)  \
	FLI_COMMAND(FLI_TRIGGER_EXPOSURE, 0)  \
  FLI_COMMAND(FLI_SET_FAN_SPEED, 1)		\

/* Enumerate the commands */
enum _commands {
#define FLI_COMMAND(name, args) name,
  FLI_COMMANDS
#undef FLI_COMMAND
};

#endif /* _LIBFLI_LIBFLI_H_ */
