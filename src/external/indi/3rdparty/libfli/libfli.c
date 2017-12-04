/*

  Copyright (c) 2000, 2002 Finger Lakes Instrumentation (FLI), L.L.C.
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "libfli-libfli.h"
#include "libfli-mem.h"
#include "libfli-debug.h"

static long devalloc(flidev_t *dev);
static long devfree(flidev_t dev);
static long fli_open(flidev_t *dev, char *name, long domain);
static long fli_close(flidev_t dev);
static long fli_freelist(char **names);

flidevdesc_t *devices[MAX_OPEN_DEVICES] = {NULL,};

#define SHOWFUNCTIONS

const char* version = \
"FLI Software Development Library for " __SYSNAME__ " " __LIBFLIVER__;

static long devalloc(flidev_t *dev)
{
  int i;

  if (dev == NULL)
    return -EINVAL;

  for (i = 0; i < MAX_OPEN_DEVICES; i++)
    if (devices[i] == NULL)
      break;

  if (i == MAX_OPEN_DEVICES)
    return -ENODEV;

  if ((devices[i] =
       (flidevdesc_t *)xcalloc(1, sizeof(flidevdesc_t))) == NULL)
    return -ENOMEM;

  *dev = i;

  return 0;
}

static long devfree(flidev_t dev)
{
  CHKDEVICE(dev);

  if (DEVICE->io_data != NULL)
  {
    debug(FLIDEBUG_WARN, "close didn't free io_data (not NULL)");
    xfree(DEVICE->io_data);
    DEVICE->io_data = NULL;
  }
  if (DEVICE->device_data != NULL)
  {
    debug(FLIDEBUG_WARN, "close didn't free device_data (not NULL)");
    xfree(DEVICE->device_data);
    DEVICE->device_data = NULL;
  }
  if (DEVICE->sys_data != NULL)
  {
    debug(FLIDEBUG_WARN, "close didn't free sys_data (not NULL)");
    xfree(DEVICE->sys_data);
    DEVICE->sys_data = NULL;
  }

  if (DEVICE->name != NULL)
  {
    xfree(DEVICE->name);
    DEVICE->name = NULL;
  }

  xfree(DEVICE);
  DEVICE = NULL;

  return 0;
}

static long fli_open(flidev_t *dev, char *name, long domain)
{
  int retval;

  debug(FLIDEBUG_INFO, "Trying to open file <%s> in domain %d.",
	name, domain);

  if ((retval = devalloc(dev)) != 0)
  {
    debug(FLIDEBUG_WARN, "error devalloc() %d [%s]",
	  retval, strerror(-retval));
    return retval;
  }

  debug(FLIDEBUG_INFO, "Got device index %d", *dev);

  if ((retval = fli_connect(*dev, name, domain)) != 0)
  {
    debug(FLIDEBUG_WARN, "fli_connect() error %d [%s]",
	  retval, strerror(-retval));
    devfree(*dev);
    return retval;
  }

  if ((retval = devices[*dev]->fli_open(*dev)) != 0)
  {
    debug(FLIDEBUG_WARN, "fli_open() error %d [%s]",
	  retval, strerror(-retval));
    fli_disconnect(*dev);
    devfree(*dev);
    return retval;
  }

  return retval;
}

static long fli_close(flidev_t dev)
{
  CHKDEVICE(dev);
  CHKFUNCTION(DEVICE->fli_close);

	debug(FLIDEBUG_INFO, "Closing device index: %d ", dev);

	DEVICE->fli_close(dev);
  fli_disconnect(dev);
  devfree(dev);

  return 0;
}

static long fli_freelist(char **names)
{
  int i;

  if (names == NULL)
    return 0;

  for (i = 0; names[i] != NULL; i++)
    xfree(names[i]);
  xfree(names);

  return 0;
}

/* This is for FLI INTERNAL USE ONLY */
#ifdef _WIN32
long usb_bulktransfer(flidev_t dev, int ep, void *buf, long *len);
#else
long linux_bulktransfer(flidev_t dev, int ep, void *buf, long *len);
#define usb_bulktransfer linux_bulktransfer
#endif

LIBFLIAPI FLIStartVideoMode(flidev_t dev)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_START_VIDEO_MODE, 0);
}

LIBFLIAPI FLIStopVideoMode(flidev_t dev)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_STOP_VIDEO_MODE, 0);
}

LIBFLIAPI FLIGrabVideoFrame(flidev_t dev, void *buff, size_t size)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GRAB_VIDEO_FRAME, 2, buff, &size);
}

LIBFLIAPI FLIUsbBulkIO(flidev_t dev, int ep, void *buf, long *len)
{
	return usb_bulktransfer(dev, ep, buf, len);
}

/* This function is not implemented */
LIBFLIAPI FLIGrabFrame(flidev_t dev, void* buff,
		       size_t buffsize, size_t* bytesgrabbed)
{
	return -EINVAL;
}

/**
   Cancel an exposure for a given camera.  This function cancels an
   exposure in progress by closing the shutter.

   @param dev Camera to cancel the exposure of.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIExposeFrame
   @see FLIGetExposureStatus
   @see FLISetExposureTime
*/
LIBFLIAPI FLICancelExposure(flidev_t dev)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_CANCEL_EXPOSURE, 0);
}

/**
   Close a handle to a FLI device.

   @param dev The device handle to be closed.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIOpen
*/
LIBFLIAPI FLIClose(flidev_t dev)
{
	long r;

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering %s", __FUNCTION__);
#endif

	r = fli_close(dev);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Exiting %s", __FUNCTION__);
#endif

	return r;
}

/**
   Get the array area of the given camera.  This function finds the
   \emph{total} area of the CCD array for camera \texttt{dev}.  This
   area is specified in terms of a upper-left point and a lower-right
   point.  The upper-left x-coordinate is placed in \texttt{ul_x}, the
   upper-left y-coordinate is placed in \texttt{ul_y}, the lower-right
   x-coordinate is placed in \texttt{lr_x}, and the lower-right
   y-coordinate is placed in \texttt{lr_y}.

   @param dev Camera to get the array area of.

   @param ul_x Pointer to where the upper-left x-coordinate is to be
   placed.

   @param ul_y Pointer to where the upper-left y-coordinate is to be
   placed.

   @param lr_x Pointer to where the lower-right x-coordinate is to be
   placed.

   @param lr_y Pointer to where the lower-right y-coordinate is to be
   placed.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetVisibleArea
   @see FLISetImageArea
*/
LIBFLIAPI FLIGetArrayArea(flidev_t dev, long* ul_x, long* ul_y,
			  long* lr_x, long* lr_y)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_ARRAY_AREA, 4,
			     ul_x, ul_y, lr_x, lr_y);
}

/**
   Flush rows of a given camera.  This function flushes \texttt{rows}
   rows of camera \texttt{dev} \texttt{repeat} times.

   @param dev Camera to flush rows of.

   @param rows Number of rows to flush.

   @param repeat Number of times to flush each row.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLISetNFlushes
*/
LIBFLIAPI FLIFlushRow(flidev_t dev, long rows, long repeat)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_FLUSH_ROWS, 2, &rows, &repeat);
}

/**
   Get firmware revision of a given device.

   @param dev Device to find the firmware revision of.

   @param fwrev Pointer to a long which will receive the firmware
   revision.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetModel
   @see FLIGetHWRevision
   @see FLIGetSerialNum
*/
LIBFLIAPI FLIGetFWRevision(flidev_t dev, long *fwrev)
{
  CHKDEVICE(dev);

  *fwrev = DEVICE->devinfo.fwrev;
  return 0;
}

/**
   Get the hardware revision of a given device.

   @param dev Device to find the hardware revision of.

   @param hwrev Pointer to a long which will receive the hardware
   revision.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetModel
   @see FLIGetFWRevision
   @see FLIGetSerialNum
*/
LIBFLIAPI FLIGetHWRevision(flidev_t dev, long *hwrev)
{
  CHKDEVICE(dev);

  *hwrev = DEVICE->devinfo.hwrev;
  return 0;
}

/**
   Get the device type.

   @param dev Device to find the hardware revision of.

   @param devid Pointer to a long which will receive the device ID. Expected values are listed in libfli-libfli.h (FLIUSB_PROLINEID..).

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetModel
   @see FLIGetFWRevision
   @see FLIGetHWRevision
*/
LIBFLIAPI FLIGetDeviceID(flidev_t dev, long *devid)
{
  CHKDEVICE(dev);

  *devid = DEVICE->devinfo.devid;
  return 0;
}

/**
   Get the serial number of a given device.

   @param dev Device to find the hardware revision of.

   @param serno Pointer to a long which will receive the serial number.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetModel
   @see FLIGetFWRevision
   @see FLIGetHWRevision
*/
LIBFLIAPI FLIGetSerialNum(flidev_t dev, long *serno)
{
  CHKDEVICE(dev);

  *serno = DEVICE->devinfo.serno;
  return 0;
}

/**
   Get name of a given device.

   @param dev Device to find the hardware revision of.

   @param devnam Pointer to a char pointer, which will point to device name.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetModel
   @see FLIGetFWRevision
   @see FLIGetHWRevision
*/

LIBFLIAPI FLIGetDeviceName(flidev_t dev, const char **devnam)
{
  CHKDEVICE(dev);

  *devnam = DEVICE->devinfo.devnam;
  return 0;
}

/**
   Get the current library version.  This function copies up to
   \texttt{len - 1} characters of the current library version string
   followed by a terminating \texttt{NULL} character into the buffer
   pointed to by \texttt{ver}.

   @param ver Pointer to a character buffer where the library version
   string is to be placed.

   @param len The size in bytes of the buffer pointed to by
   \texttt{ver}.

   @return Zero on success.
   @return Non-zero on failure.
*/
LIBFLIAPI FLIGetLibVersion(char* ver, size_t len)
{
  if (len > 0 && ver == NULL)
    return -EINVAL;

  if ((size_t) snprintf(ver, len, "%s", version) >= len)
    return -EOVERFLOW;
  else
    return 0;
}

/**
   Get the model of a given device.  This function copies up to
   \texttt{len - 1} characters of the model string for device
   \texttt{dev}, followed by a terminating \texttt{NULL} character
   into the buffer pointed to by \texttt{model}.

   @param dev Device to find model of.

   @param model Pointer to a character buffer where the model string
   is to be placed.

   @param len The size in bytes of buffer pointed to by
   \texttt{model}.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetHWRevision
   @see FLIGetFWRevision
   @see FLIGetSerialNum
*/
LIBFLIAPI FLIGetModel(flidev_t dev, char* model, size_t len)
{
  if (model == NULL)
    return -EINVAL;

  CHKDEVICE(dev);

  if (DEVICE->devinfo.model == NULL)
  {
    model[0] = '\0';
    return 0;
  }

  if ((size_t) snprintf(model, len, "%s", DEVICE->devinfo.model) >= len)
    return -EOVERFLOW;
  else
    return 0;
}

/**
   Find the dimensions of a pixel in the array of the given device.

   @param dev Device to find the pixel size of.

   @param pixel_x Pointer to a double which will receive the size (in
   microns) of a pixel in the x direction.

   @param pixel_y Pointer to a double which will receive the size (in
   microns) of a pixel in the y direction.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetArrayArea
   @see FLIGetVisibleArea
*/
LIBFLIAPI FLIGetPixelSize(flidev_t dev, double *pixel_x, double *pixel_y)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_PIXEL_SIZE, 2,
				   pixel_x, pixel_y);
}


/**
   Get the visible area of the given camera.  This function finds the
   \emph{visible} area of the CCD array for the camera \texttt{dev}.
   This area is specified in terms of a upper-left point and a
   lower-right point.  The upper-left x-coordinate is placed in
   \texttt{ul_x}, the upper-left y-coordinate is placed in
   \texttt{ul_y}, the lower-right x-coordinate is placed in
   \texttt{lr_x}, the lower-right y-coordinate is placed in
   \texttt{lr_y}.

   @param dev Camera to get the visible area of.

   @param ul_x Pointer to where the upper-left x-coordinate is to be
   placed.

   @param ul_y Pointer to where the upper-left y-coordinate is to be
   placed.

   @param lr_x Pointer to where the lower-right x-coordinate is to be
   placed.

   @param lr_y Pointer to where the lower-right y-coordinate is to be
   placed.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetArrayArea
   @see FLISetImageArea
*/
LIBFLIAPI FLIGetVisibleArea(flidev_t dev, long* ul_x, long* ul_y,
			    long* lr_x, long* lr_y)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_VISIBLE_AREA, 4,
			     ul_x, ul_y, lr_x, lr_y);
}

/**
   Get a handle to an FLI device. This function requires the filename
   and domain of the requested device. Valid device filenames can be
   obtained using the \texttt{FLIList()} function. An application may
   use any number of handles associated with the same physical
   device. When doing so, it is important to lock the appropriate
   device to ensure that multiple accesses to the same device do not
   occur during critical operations.

   @param dev Pointer to where a device handle will be placed.

   @param name Pointer to a string where the device filename to be
   opened is stored.  For parallel port devices that are not probed by
   \texttt{FLIList()} (Windows 95/98/Me), place the address of the
   parallel port in a string in ascii form ie: "0x378".

   @param domain Domain to apply to \texttt{name} for device opening.
   This is a bitwise ORed combination of interface method and device
   type.  Valid interfaces include \texttt{FLIDOMAIN_PARALLEL_PORT},
   \texttt{FLIDOMAIN_USB}, \texttt{FLIDOMAIN_SERIAL}, and
   \texttt{FLIDOMAIN_INET}.  Valid device types include
   \texttt{FLIDEVICE_CAMERA}, \texttt{FLIDOMAIN_FILTERWHEEL}, and
   \texttt{FLIDOMAIN_FOCUSER}.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIList
   @see FLIClose
   @see flidomain_t
*/

LIBFLIAPI FLIOpen(flidev_t *dev, char *name, flidomain_t domain)
{
	long r;

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering %s", __FUNCTION__);
#endif

  r = fli_open(dev, name, domain);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Exiting %s", __FUNCTION__);
#endif

	return r;
}

/**
   Enable debugging of API operations and communications. Use this
   function in combination with FLIDebug to assist in diagnosing
   problems that may be encountered during programming.

   When usings Microsoft Windows operating systems, creating an empty file
   C:\\FLIDBG.TXT will override this option. All debug output will
   then be directed to this file.

   @param host Name of the file to send debugging information to.
   This parameter is ignored under Linux where \texttt{syslog(3)} is
   used to send debug messages (see \texttt{syslog.conf(5)} for how to
   configure syslogd).

   @param level Debug level.  A value of \texttt{FLIDEBUG_NONE} disables
     debugging.  Values of \texttt{FLIDEBUG_FAIL}, \texttt{FLIDEBUG_WARN}, and
     \texttt{FLIDEBUG_INFO} enable progressively more verbose debug messages.

   @return Zero on success.
   @return Non-zero on failure.
*/
LIBFLIAPI FLISetDebugLevel(char *host, flidebug_t level)
{
  setdebuglevel(host, level);
  return 0;
}

/**
   Set the exposure time for a camera.  This function sets the
   exposure time for the camera \texttt{dev} to \texttt{exptime} msec.

   @param dev Camera to set the exposure time of.

   @param exptime Exposure time in msec.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIExposeFrame
   @see FLICancelExposure
   @see FLIGetExposureStatus
*/
LIBFLIAPI FLISetExposureTime(flidev_t dev, long exptime)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_EXPOSURE_TIME, 1, &exptime);
}

/**
   Set the horizontal bin factor for a given camera.  This function
   sets the horizontal bin factor for the camera \texttt{dev} to
   \texttt{hbin}.  The valid range of the \texttt{hbin} parameter is
   from 1 to 16.

   @param dev Camera to set horizontal bin factor of.

   @param hbin Horizontal bin factor.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLISetVBin
   @see FLISetImageArea
*/
LIBFLIAPI FLISetHBin(flidev_t dev, long hbin)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_HBIN, 1, &hbin);
}

/**
   Set the frame type for a given camera.  This function sets the frame type
   for camera \texttt{dev} to \texttt{frametype}.  The \texttt{frametype}
   parameter is either \texttt{FLI_FRAME_TYPE_NORMAL} for a normal frame
   where the shutter opens or \texttt{FLI_FRAME_TYPE_DARK} for a dark frame
   where the shutter remains closed.

   @param cam Camera to set the frame type of.

   @param frametype Frame type: \texttt{FLI_FRAME_TYPE_NORMAL} or \texttt{FLI_FRAME_TYPE_DARK}.

   @return Zero on success.
   @return Non-zero on failure.

   @see fliframe_t
   @see FLIExposeFrame
*/
LIBFLIAPI FLISetFrameType(flidev_t dev, fliframe_t frametype)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_FRAME_TYPE, 1, &frametype);
}

/**
   Get the cooler power level. The function places the current cooler
	 power in percent in the
   location pointed to by \texttt{power}.

   @param dev Camera to find the cooler power of.

   @param timeleft Pointer to where the cooler power (in percent) will be placed.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLISetTemperature
   @see FLIGetTemperature
*/
LIBFLIAPI FLIGetCoolerPower(flidev_t dev, double *power)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_COOLER_POWER, 1, power);
}

LIBFLIAPI FLIGetCameraModeString(flidev_t dev, flimode_t mode_index, char *mode_string, size_t siz)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_CAMERA_MODE_STRING, 3, mode_index, mode_string, siz);
}

LIBFLIAPI FLIGetCameraMode(flidev_t dev, flimode_t *mode_index)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_CAMERA_MODE, 1, mode_index);
}

LIBFLIAPI FLISetCameraMode(flidev_t dev, flimode_t mode_index)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_CAMERA_MODE, 1, mode_index);
}

LIBFLIAPI FLIGetDeviceStatus(flidev_t dev, long *camera_status)
{
  CHKDEVICE(dev);

	*camera_status = 0xffffffff;
  return DEVICE->fli_command(dev, FLI_GET_STATUS, 1, camera_status);
}

/**
   Set the image area for a given camera.  This function sets the
   image area for camera \texttt{dev} to an area specified in terms of
   a upper-left point and a lower-right point.  The upper-left
   x-coordinate is \texttt{ul_x}, the upper-left y-coordinate is
   \texttt{ul_y}, the lower-right x-coordinate is \texttt{lr_x}, and
   the lower-right y-coordinate is \texttt{lr_y}.  Note that the given
   lower-right coordinate must take into account the horizontal and
   vertical bin factor settings, but the upper-left coordinate is
   absolute.  In other words, the lower-right coordinate used to set
   the image area is a virtual point $(lr_x', lr_y')$ determined by:

   \[ lr_x' = ul_x + (lr_x - ul_x) / hbin \]
   \[ lr_y' = ul_y + (lr_y - ul_y) / vbin \]

   Where $(lr_x', lr_y')$ is the coordinate to pass to the
   \texttt{FLISetImageArea} function, $(ul_x, ul_y)$ and $(lr_x,
   lr_y)$ are the absolute coordinates of the desired image area,
   $hbin$ is the horizontal bin factor, and $vbin$ is the vertical bin
   factor.

   @param dev Camera to set image area of.

   @param ul_x Upper-left x-coordinate of image area.

   @param ul_y Upper-left y-coordinate of image area.

   @param lr_x Lower-right x-coordinate of image area ($lr_x'$ from
   above).

   @param lr_y Lower-right y-coordinate of image area ($lr_y'$ from
   above).

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetVisibleArea
   @see FLIGetArrayArea
*/
LIBFLIAPI FLISetImageArea(flidev_t dev, long ul_x, long ul_y,
			  long lr_x, long lr_y)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_IMAGE_AREA, 4,
				   &ul_x, &ul_y, &lr_x, &lr_y);
}

/**
   Set the vertical bin factor for a given camera.  This function sets
   the vertical bin factor for the camera \texttt{dev} to
   \texttt{vbin}.  The valid range of the \texttt{vbin} parameter is
   from 1 to 16.

   @param dev Camera to set vertical bin factor of.

   @param vbin Vertical bin factor.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLISetHBin
   @see FLISetImageArea
*/
LIBFLIAPI FLISetVBin(flidev_t dev, long vbin)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_VBIN, 1, &vbin);
}

/**
   Find the remaining exposure time of a given camera.  This functions
   places the remaining exposure time (in milliseconds) in the
   location pointed to by \texttt{timeleft}.

   @param dev Camera to find the remaining exposure time of.

   @param timeleft Pointer to where the remaining exposure time (in milliseonds) will be placed.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIExposeFrame
   @see FLICancelExposure
   @see FLISetExposureTime
*/
LIBFLIAPI FLIGetExposureStatus(flidev_t dev, long *timeleft)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_EXPOSURE_STATUS, 1, timeleft);
}

/**
   Set the temperature of a given camera.  This function sets the
   temperature of the CCD camera \texttt{dev} to \texttt{temperature}
   degrees Celsius.  The valid range of the \texttt{temperature}
   parameter is from -55 C to 45 C.

   @param dev Camera device to set the temperature of.

   @param temperature Temperature in Celsius to set CCD camera cold finger to.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetTemperature
*/
LIBFLIAPI FLISetTemperature(flidev_t dev, double temperature)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_TEMPERATURE, 1,
				   &temperature);
}

/**
   Get the temperature of a given camera.  This function places the
   temperature of the CCD camera cold finger of device \texttt{dev} in
   the location pointed to by \texttt{temperature}.

   @param dev Camera device to get the temperature of.

   @param temperature Pointer to where the temperature will be placed.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLISetTemperature
*/
LIBFLIAPI FLIGetTemperature(flidev_t dev, double *temperature)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_TEMPERATURE, 1, temperature);
}

/**
   Grab a row of an image.  This function grabs the next available row
   of the image from camera device \texttt{dev}.  The row of width
   \texttt{width} is placed in the buffer pointed to by \texttt{buff}.
   The size of the buffer pointed to by \texttt{buff} must take into
   account the bit depth of the image, meaning the buffer size must be
   at least \texttt{width} bytes for an 8-bit image, and at least
   2*\texttt{width} for a 16-bit image.

   @param dev Camera whose image to grab the next available row from.

   @param buff Pointer to where the next available row will be placed.

   @param width Row width in pixels.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGrabFrame
*/
LIBFLIAPI FLIGrabRow(flidev_t dev, void *buff, size_t width)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GRAB_ROW, 2, buff, &width);
}

/**
   Expose a frame for a given camera.  This function exposes a frame
   according to the settings (image area, exposure time, bit depth,
   etc.) of camera \texttt{dev}.  The settings of \texttt{dev} must be
   valid for the camera device.  They are set by calling the
   appropriate set library functions.  This function returns after the
   exposure has started.

   @param dev Camera to expose the frame of.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLISetExposureTime
   @see FLISetFrameType
   @see FLISetImageArea
   @see FLISetHBin
   @see FLISetVBin
   @see FLISetNFlushes
   @see FLISetBitDepth
   @see FLIGrabFrame
   @see FLICancelExposure
   @see FLIGetExposureStatus
*/
LIBFLIAPI FLIExposeFrame(flidev_t dev)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_EXPOSE_FRAME, 0);
}

/**
   Set the gray-scale bit depth for a given camera.  This function
   sets the gray-scale bit depth of camera \texttt{dev} to
   \texttt{bitdepth}.  The \texttt{bitdepth} parameter is either
   \texttt{FLI_MODE_8BIT} for 8-bit mode or \texttt{FLI_MODE_16BIT}
   for 16-bit mode. Many cameras do not support this mode.

   @param dev Camera to set the bit depth of.

   @param bitdepth Gray-scale bit depth: \texttt{FLI_MODE_8BIT} or
   \texttt{FLI_MODE_16BIT}.

   @return Zero on success.
   @return Non-zero on failure.

   @see flibitdepth_t
   @see FLIExposeFrame
*/
LIBFLIAPI FLISetBitDepth(flidev_t dev, flibitdepth_t bitdepth)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_BIT_DEPTH, 1, &bitdepth);
}

/**
   Set the number of flushes for a given camera.  This function sets
   the number of times the CCD array of camera \texttt{dev} is flushed by
   the FLIExposeFrame \emph{before} exposing a frame
   to \texttt{nflushes}.  The valid range of the \texttt{nflushes}
   parameter is from 0 to 16. Some FLI cameras support background flushing.
   Background flushing continuously flushes the CCD eliminating the need for
   pre-exposure flushing.

   @param dev Camera to set the number of flushes of.

   @param nflushes Number of times to flush CCD array before an
   exposure.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIFlushRow
   @see FLIExposeFrame
   @see FLIControlBackgroundFlush
*/
LIBFLIAPI FLISetNFlushes(flidev_t dev, long nflushes)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_FLUSHES, 1, &nflushes);
}

/**
   Read the I/O port of a given camera.  This function reads the I/O
   port on camera \texttt{dev} and places the value in the location
   pointed to by \texttt{ioportset}.

   @param dev Camera to read the I/O port of.

   @param ioportset Pointer to where the I/O port data will be stored.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIWriteIOPort
   @see FLIConfigureIOPort
*/
LIBFLIAPI FLIReadIOPort(flidev_t dev, long *ioportset)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_READ_IOPORT, 1, ioportset);
}

/**
   Write to the I/O port of a given camera.  This function writes the
   value \texttt{ioportset} to the I/O port on camera \texttt{dev}.

   @param dev Camera to write I/O port of.

   @param ioportset Data to be written to the I/O port.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIReadIOPort
   @see FLIConfigureIOPort
*/
LIBFLIAPI FLIWriteIOPort(flidev_t dev, long ioportset)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_WRITE_IOPORT, 1, &ioportset);
}

/**
   Configure the I/O port of a given camera.  This function configures
   the I/O port on camera \texttt{dev} with the value
   \texttt{ioportset}.

   The I/O configuration of each pin on a given camera is determined by the
	 value of \texttt{ioportset}.  Setting a respective I/O bit enables the
	 port bit for output while clearing an I/O bit enables to port bit for
	 input. By default, all I/O ports are configured as inputs.

   @param dev Camera to configure the I/O port of.

   @param ioportset Data to configure the I/O port with.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIReadIOPort
   @see FLIWriteIOPort
*/
LIBFLIAPI FLIConfigureIOPort(flidev_t dev, long ioportset)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_CONFIGURE_IOPORT, 1,
				   &ioportset);
}

/**
   Lock a specified device.  This function establishes an exclusive
   lock (mutex) on the given device to prevent access to the device by
   any other function or process.

   @param dev Device to lock.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIUnlockDevice
*/
LIBFLIAPI FLILockDevice(flidev_t dev)
{
  CHKDEVICE(dev);

  return DEVICE->fli_lock(dev);
}

/**
   Unlock a specified device.  This function releases a previously
   established exclusive lock (mutex) on the given device to allow
   access to the device by any other function or process.

   @param dev Device to unlock.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLILockDevice
*/
LIBFLIAPI FLIUnlockDevice(flidev_t dev)
{
  CHKDEVICE(dev);

  return DEVICE->fli_unlock(dev);
}

/**
   Control the shutter on a given camera.  This function controls the
   shutter function on camera \texttt{dev} according to the
   \texttt{shutter} parameter.

   @param dev Device to control the shutter of.

   @param shutter How to control the shutter.  A value of
   \texttt{FLI_SHUTTER_CLOSE} closes the shutter and
   \texttt{FLI_SHUTTER_OPEN} opens the shutter.
	 \texttt{FLI_SHUTTER_EXTERNAL_TRIGGER_LOW}, \texttt{FLI_SHUTTER_EXTERNAL_TRIGGER}
	 causes the exposure to begin
	 only when a logic LOW is detected on I/O port bit 0.
	 \texttt{FLI_SHUTTER_EXTERNAL_TRIGGER_HIGH} causes the exposure to begin
	 only when a logic HIGH is detected on I/O port bit 0. This setting
	 may not be available on all cameras.

   @return Zero on success.
   @return Non-zero on failure.

   @see flishutter_t
*/
LIBFLIAPI FLIControlShutter(flidev_t dev, flishutter_t shutter)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_CONTROL_SHUTTER, 1, &shutter);
}

/* The following function is for internal use only, improper
use of this function can lead to permanent damage of the attached
device. */

LIBFLIAPI FLISetDAC(flidev_t dev, unsigned long dacset)
{
	CHKDEVICE(dev);

	return DEVICE->fli_command(dev, FLI_SET_DAC, 1, &dacset);
}

/**
   Enables background flushing of CCD array.  This function enables the
	 background flushing of the CCD array camera \texttt{dev} according to the
   \texttt{bgflush} parameter. Note that this function may not succeed
	 on all FLI products as this feature may not be available.

   @param dev Device to control the background flushing of.

   @param bgflush Enables or disables background flushing. A value of
   \texttt{FLI_BGFLUSH_START} begins background flushing. It is important to
   note that background flushing is stopped whenever \texttt{FLIExposeFrame()}
	 or \texttt{FLIControlShutter()} are called. \texttt{FLI_BGFLUSH_STOP} stops all
	 background flush activity.

   @return Zero on success.
   @return Non-zero on failure.

   @see flibgflush_t
*/
LIBFLIAPI FLIControlBackgroundFlush(flidev_t dev, flibgflush_t bgflush)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_CONTROL_BGFLUSH, 1, &bgflush);
}

/**
   List available devices.  This function returns a pointer to a NULL
   terminated list of device names.  The pointer should be freed later
   with \texttt{FLIFreeList()}.  Each device name in the returned list
   includes the filename needed by \texttt{FLIOpen()}, a separating
   semicolon, followed by the model name or user assigned device name.

   @param domain Domain to list the devices of.  This is a bitwise
   ORed combination of interface method and device type.  Valid
   interfaces include \texttt{FLIDOMAIN_PARALLEL_PORT},
   \texttt{FLIDOMAIN_USB}, \texttt{FLIDOMAIN_SERIAL}, and
   \texttt{FLIDOMAIN_INET}.  Valid device types include
   \texttt{FLIDEVICE_CAMERA}, \texttt{FLIDOMAIN_FILTERWHEEL}, and
   \texttt{FLIDOMAIN_FOCUSER}.

   @param names Pointer to where the device name list will be placed.

   @return Zero on success.
   @return Non-zero on failure.

   @see flidomain_t
   @see FLIFreeList
   @see FLIOpen
*/
LIBFLIAPI FLIList(flidomain_t domain, char ***names)
{
  debug(FLIDEBUG_INFO, "FLIList() domain %04x", domain);
  return fli_list(domain, names);
}

/**
   Free a previously generated device list.  Use this function after
   \texttt{FLIList()} to free the list of device names.

   @param names Pointer to the list.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIList
*/
LIBFLIAPI FLIFreeList(char **names)
{
  return fli_freelist(names);
}

/**
   Set the filter wheel position of a given device.  Use this function
   to set the filter wheel position of \texttt{dev} to
   \texttt{filter}.

   @param dev Filter wheel device handle.

   @param filter Desired filter wheel position.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetFilterPos
*/
LIBFLIAPI FLISetFilterPos(flidev_t dev, long filter)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_SET_FILTER_POS, 1, &filter);
}

/**
   Get the filter wheel position of a given device.  Use this function
   to get the filter wheel position of \texttt{dev}.

   @param dev Filter wheel device handle.

   @param filter Pointer to where the filter wheel position will be
   placed.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLISetFilterPos
*/
LIBFLIAPI FLIGetFilterPos(flidev_t dev, long *filter)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_FILTER_POS, 1, filter);
}

/**
   Get the number of motor steps remaining. Use this function
   to determine if the stepper motor of \texttt{dev} is still moving.

   @param dev Filter wheel device handle.

   @param filter Pointer to where the number of remaning steps will be
   placed.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLISetFilterPos
*/
LIBFLIAPI FLIGetStepsRemaining(flidev_t dev, long *steps)
{
	long r;

  CHKDEVICE(dev);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering %s", __FUNCTION__);
#endif

  r = DEVICE->fli_command(dev, FLI_GET_STEPS_REMAINING, 1, steps);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Exiting %s", __FUNCTION__);
#endif

	return r;
}

/**
   Get the filter wheel filter count of a given device.  Use this
   function to get the filter count of filter wheel \texttt{dev}.

   @param dev Filter wheel device handle.

   @param filter Pointer to where the filter wheel filter count will
   be placed.

   @return Zero on success.
   @return Non-zero on failure.
*/
LIBFLIAPI FLIGetFilterCount(flidev_t dev, long *filter)
{
  CHKDEVICE(dev);

  return DEVICE->fli_command(dev, FLI_GET_FILTER_COUNT, 1, filter);
}

/**
   Step the filter wheel or focuser motor of a given device.  Use this
   function to move the focuser or filter wheel \texttt{dev} by an
   amount \texttt{steps}. This function is non-blocking.

   @param dev Filter wheel or focuser device handle.

   @param steps Number of steps to move the focuser or filter wheel.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetStepperPosition
*/
LIBFLIAPI FLIStepMotorAsync(flidev_t dev, long steps)
{
	long r;

  CHKDEVICE(dev);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering %s", __FUNCTION__);
#endif

  r = DEVICE->fli_command(dev, FLI_STEP_MOTOR_ASYNC, 1, &steps);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Exiting %s", __FUNCTION__);
#endif

	return r;
}


/**
   Step the filter wheel or focuser motor of a given device.  Use this
   function to move the focuser or filter wheel \texttt{dev} by an
   amount \texttt{steps}.

   @param dev Filter wheel or focuser device handle.

   @param steps Number of steps to move the focuser or filter wheel.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIGetStepperPosition
*/
LIBFLIAPI FLIStepMotor(flidev_t dev, long steps)
{
	long r;

	CHKDEVICE(dev);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering %s", __FUNCTION__);
#endif

	r = DEVICE->fli_command(dev, FLI_STEP_MOTOR, 1, &steps);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Exiting %s", __FUNCTION__);
#endif

	return r;
}

/**
   Get the stepper motor position of a given device.  Use this
   function to read the stepper motor position of filter wheel or
   focuser \texttt{dev}.

   @param dev Filter wheel or focuser device handle.

   @param position Pointer to where the postion of the stepper motor
   will be placed.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIStepMotor
*/
LIBFLIAPI FLIGetStepperPosition(flidev_t dev, long *position)
{
	long r;

  CHKDEVICE(dev);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering %s", __FUNCTION__);
#endif

  r = DEVICE->fli_command(dev, FLI_GET_STEPPER_POS, 1, position);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Exiting %s", __FUNCTION__);
#endif
	
	return r;
}

/**
   Home focuser \texttt{dev}. The home position is closed as far as mechanically possiable.

   @param dev Focuser device handle.

   @return Zero on success.
   @return Non-zero on failure.
*/
LIBFLIAPI FLIHomeFocuser(flidev_t dev)
{
	long r;

	CHKDEVICE(dev);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering %s", __FUNCTION__);
#endif

	r = DEVICE->fli_command(dev, FLI_HOME_FOCUSER, 0);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Exiting %s", __FUNCTION__);
#endif

	return r;
}

/**
   Retreive the maximum extent for FLI focuser \texttt{dev}.

   @param dev Focuser device handle.
   @param extent Pointer to where the maximum extent of the focuser will be placed.

   @return Zero on success.
   @return Non-zero on failure.
*/
LIBFLIAPI FLIGetFocuserExtent(flidev_t dev, long *extent)
{
	long r;

	CHKDEVICE(dev);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering %s", __FUNCTION__);
#endif

  r = DEVICE->fli_command(dev, FLI_GET_FOCUSER_EXTENT, 1, extent);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Exiting %s", __FUNCTION__);
#endif

	return r;
}

/**
   Retreive temperature from the FLI focuser \texttt{dev}. Valid channels are
   \texttt{FLI_TEMPERATURE_INTERNAL} and \texttt{FLI_TEMPERATURE_EXTERNAL}.

   @param dev Focuser device handle.
   @param channel Channel to be read.
   @param extent Pointer to where the channel temperature will be placed.

   @return Zero on success.
   @return Non-zero on failure.
*/
LIBFLIAPI FLIReadTemperature(flidev_t dev, flichannel_t channel, double *temperature)
{
	long r;

	CHKDEVICE(dev);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering %s", __FUNCTION__);
#endif

	r = DEVICE->fli_command(dev, FLI_READ_TEMPERATURE, 2, channel, temperature);

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Exiting %s", __FUNCTION__);
#endif

	return r;
}

/* This stuff is used by the next four functions */
typedef struct list {
  char *filename;
  char *name;
  long domain;
  struct list *next;
} list_t;

static list_t *firstdevice = NULL;
static list_t *currentdevice = NULL;

/**
   Creates a list of all devices within a specified
	 \texttt{domain}. Use \texttt{FLIDeleteList()} to delete the list
	 created with this function. This function is the first called begin
	 the iteration through the list of current FLI devices attached.

   @param domain Domain to search for devices, set to zero to search all domains.
	 This parameter must contain the device type.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLIDeleteList
   @see FLIListFirst
   @see FLIListNext
*/
LIBFLIAPI FLICreateList(flidomain_t domain)
{
  char **list;
  flidomain_t domord[5];
  int i, j, k;

  for (i = 0; i < 5; i++)
  {
    domord[i] = 0;
  }

  if (firstdevice != NULL)
  {
    FLIDeleteList();
  }
  currentdevice = NULL;

  if ((domain & 0x00ff) != 0)
  {
    domord[0] = domain;
  }
  else
  {
    domord[0] = domain | FLIDOMAIN_PARALLEL_PORT;
    domord[1] = domain | FLIDOMAIN_USB;
    domord[2] = domain | FLIDOMAIN_SERIAL;
  }

  i = 0;
  while (domord[i] != 0)
  {
    debug(FLIDEBUG_INFO, "Searching for domain 0x%04x.", domord[i]);
    FLIList(domord[i], &list);
    if (list != NULL)
    {
      j = 0;
      while (list[j] != NULL)
      {
	if (firstdevice == NULL)
	{
	  firstdevice = (list_t *)xmalloc(sizeof(list_t));
	  if (firstdevice == NULL)
	    return -ENOMEM;
	  currentdevice = firstdevice;
	}
	else
	{
	  currentdevice->next = (list_t *) xmalloc(sizeof(list_t));
	  if (currentdevice->next == NULL)
	    return -ENOMEM;
	  currentdevice = currentdevice->next;
	}
	currentdevice->next = NULL;
	currentdevice->domain = domord[i];
	currentdevice->filename = NULL;
	currentdevice->name = NULL;

	k = 0;
	while (k < (int) strlen(list[j]))
	{
	  if (list[j][k] == ';')
	  {
	    currentdevice->filename = (char *) xmalloc(k+1);
	    if (currentdevice->filename != NULL)
	    {
	      strncpy(currentdevice->filename, list[j], k);
	      currentdevice->filename[k] = '\0';
	    }
	    currentdevice->name = (char *) xmalloc(strlen(&list[j][k+1]) + 1);
	    if (currentdevice->name != NULL)
	    {
	      strcpy(currentdevice->name, &list[j][k+1]);
	    }
	    break;
	  }
	  k++;
	}
	j++;
      }
      FLIFreeList(list);
    }
    i++;
  }
  return 0;
}

/**
   Deletes a list of devices created by \texttt{FLICreateList()}.

   @return Zero on success.
   @return Non-zero on failure.

   @see FLICreateList
   @see FLIListFirst
   @see FLIListNext
*/
LIBFLIAPI FLIDeleteList(void)
{
  list_t *dev = firstdevice;
  list_t *last;

  while (dev != NULL)
  {
    if (dev->filename != NULL)
      xfree(dev->filename);
    if (dev->name != NULL)
      xfree(dev->name);
    last = dev;
    dev = dev->next;
    xfree(last);
  }

  firstdevice = NULL;
  currentdevice = NULL;

  return 0;
}


/**
  Obtains the first device in the list. Use this function to
	get the first \texttt{domain}, \texttt{filename} and \texttt{name}
	from the list of attached FLI devices created using
	the function \texttt{FLICreateList()}. Use
	 \texttt{FLIListNext()} to obtain more found devices.

   @param domain Pointer to where to domain of the device will be placed.

   @param filename Pointer to where the filename of the device will be placed.

	 @param fnlen Length of the supplied buffer to hold the filename.

	 @param name Pointer to where the name of the device will be placed.

	 @param namelen Length of the supplied buffer to hold the name.

   @return Zero on success.
   @return Non-zero on failure.

	 @see FLICreateList
   @see FLIDeleteList
   @see FLIListNext
*/
LIBFLIAPI FLIListFirst(flidomain_t *domain, char *filename,
		       size_t fnlen, char *name, size_t namelen)
{
  currentdevice = firstdevice;
  return FLIListNext(domain, filename, fnlen, name, namelen);
}



/**
  Obtains the next device in the list. Use this function to
	get the next \texttt{domain}, \texttt{filename} and \texttt{name}
	from the list of attached FLI devices created using
	the function \texttt{FLICreateList()}.

  @param domain Pointer to where to domain of the device will be placed.

	@param filename Pointer to where the filename of the device will be placed.

	@param fnlen Length of the supplied buffer to hold the filename.

	@param name Pointer to where the name of the device will be placed.

	@param namelen Length of the supplied buffer to hold the name.

  @return Zero on success.
  @return Non-zero on failure.

	@see FLICreateList
  @see FLIDeleteList
  @see FLIListFirst
*/
LIBFLIAPI FLIListNext(flidomain_t *domain, char *filename,
		      size_t fnlen, char *name, size_t namelen)
{
  if (currentdevice == NULL)
  {
    *domain = 0;
    filename[0] = '\0';
    name[0] = '\0';
    return -EBADF;
  }

  *domain = currentdevice->domain;
  strncpy(filename, currentdevice->filename, fnlen);
  filename[fnlen-1] = '\0';
  strncpy(name, currentdevice->name, namelen);
  name[namelen-1] = '\0';

  currentdevice = currentdevice->next;
  return 0;
}

LIBFLIAPI FLISetFanSpeed(flidev_t dev, long fan_speed)
{
	long r;

	CHKDEVICE(dev);

	r = DEVICE->fli_command(dev, FLI_SET_FAN_SPEED, 1, &fan_speed);

	return r;
}

