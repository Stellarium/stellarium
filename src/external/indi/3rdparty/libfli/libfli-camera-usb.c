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

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/param.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include "libfli-libfli.h"
#include "libfli-debug.h"
#include "libfli-mem.h"
#include "libfli-camera.h"
#include "libfli-camera-usb.h"
#include "libfli-usb.h"

double dconvert(void *buf)
{
  unsigned char *fnum = (unsigned char *) buf;
  double sign, exponent, mantissa, result;

  sign = (double) ((fnum[3] & 0x80)?(-1):(1));
  exponent = (double) ((fnum[3] & 0x7f) << 1 | ((fnum[2] & 0x80)?1:0));

  mantissa = 1.0 +
    ((double) ((fnum[2] & 0x7f) << 16 | fnum[1] << 8 | fnum[0]) /
     pow(2, 23));

  result = sign * (double) pow(2, (exponent - 127.0)) * mantissa;

  return result;
}

long fli_camera_usb_open(flidev_t dev)
{
	flicamdata_t *cam;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
//	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	cam = DEVICE->device_data;

#ifdef __linux__
	/* Linux needs this page aligned, hopefully this is 512 byte aligned too... */
	cam->max_usb_xfer = (USB_READ_SIZ_MAX / getpagesize()) * getpagesize();
	cam->gbuf_siz = 2 * cam->max_usb_xfer;

	if ((cam->gbuf = xmemalign(getpagesize(), cam->gbuf_siz)) == NULL)
		return -ENOMEM;
#else
	/* Just 512 byte align it... */
	cam->max_usb_xfer = (USB_READ_SIZ_MAX & 0xfffffe00);
	cam->gbuf_siz = 2 * cam->max_usb_xfer;

	if ((cam->gbuf = xmalloc(cam->gbuf_siz)) == NULL)
		return -ENOMEM;
#endif

	if ((DEVICE->devinfo.devid >= 0x0100) && (DEVICE->devinfo.devid < 0x0110))
			DEVICE->devinfo.devid = FLIUSB_PROLINE_ID;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			short camtype;

			IOWRITE_U16(buf, 0, FLI_USBCAM_HARDWAREREV);
			rlen = 2; wlen = 2;
			IO(dev, buf, &wlen, &rlen);
			IOREAD_U16(buf, 0, DEVICE->devinfo.hwrev);

			IOWRITE_U16(buf, 0, FLI_USBCAM_DEVICEID);
			rlen = 2; wlen = 2;
			IO(dev, buf, &wlen, &rlen);
			IOREAD_U16(buf, 0, camtype);

			IOWRITE_U16(buf, 0, FLI_USBCAM_SERIALNUM);
			rlen = 2; wlen = 2;
			IO(dev, buf, &wlen, &rlen);
			IOREAD_U16(buf, 0, DEVICE->devinfo.serno);

			/* The following devices need information downloaded to them */
			if (DEVICE->devinfo.fwrev < 0x0201)
			{
				int id;

				for (id = 0; knowndev[id].index != 0; id++)
					if (knowndev[id].index == camtype)
						break;

				if (knowndev[id].index == 0)
					return -ENODEV;

				cam->ccd.pixelwidth = knowndev[id].pixelwidth;
				cam->ccd.pixelheight = knowndev[id].pixelheight;

				wlen = 14; rlen = 0;
				IOWRITE_U16(buf, 0, FLI_USBCAM_DEVINIT);
				IOWRITE_U16(buf, 2, (unsigned short) knowndev[id].array_area.lr.x);
				IOWRITE_U16(buf, 4, (unsigned short) knowndev[id].array_area.lr.y);
				IOWRITE_U16(buf, 6, (unsigned short) (knowndev[id].visible_area.lr.x -
								knowndev[id].visible_area.ul.x));
				IOWRITE_U16(buf, 8, (unsigned short) (knowndev[id].visible_area.lr.y -
								knowndev[id].visible_area.ul.y));
				IOWRITE_U16(buf, 10, (unsigned short) knowndev[id].visible_area.ul.x);
				IOWRITE_U16(buf, 12, (unsigned short) knowndev[id].visible_area.ul.y);
				IO(dev, buf, &wlen, &rlen);

				DEVICE->devinfo.model = xstrndup(knowndev[id].model, 31);

				switch(DEVICE->devinfo.fwrev & 0xff00)
				{
					case 0x0100:
						cam->tempslope = (70.0 / 215.75);
						cam->tempintercept = (-52.5681);
						break;

					case 0x0200:
						cam->tempslope = (100.0 / 201.1);
						cam->tempintercept = (-61.613);
						break;

					default:
						cam->tempslope = 1e-12;
						cam->tempintercept = 0;
				}
			}
			/* Here, all the parameters are stored on the camera */
			else if (DEVICE->devinfo.fwrev >= 0x0201)
			{
				rlen = 64; wlen = 2;
				IOWRITE_U16(buf, 0, FLI_USBCAM_READPARAMBLOCK);
				IO(dev, buf, &wlen, &rlen);

				IOREAD_LF(buf, 31, cam->ccd.pixelwidth);
				IOREAD_LF(buf, 35, cam->ccd.pixelheight);
				IOREAD_LF(buf, 23, cam->tempslope);
				IOREAD_LF(buf, 27, cam->tempintercept);
			}

			rlen = 32; wlen = 2;
			IOWRITE_U16(buf, 0, FLI_USBCAM_DEVICENAME);
			IO(dev, buf, &wlen, &rlen);

			/* Hack to make old software happy */
			DEVICE->devinfo.devnam = xcalloc(1, 32);
			DEVICE->devinfo.model = xcalloc(1, 32);
			strncpy(DEVICE->devinfo.devnam, (char *) buf, 30);
			strncpy(DEVICE->devinfo.model, (char *) buf, 30);

			rlen = 4; wlen = 2;
			IOWRITE_U16(buf, 0, FLI_USBCAM_ARRAYSIZE);
			IO(dev, buf, &wlen, &rlen);
			cam->ccd.array_area.ul.x = 0;
			cam->ccd.array_area.ul.y = 0;
			IOREAD_U16(buf, 0, cam->ccd.array_area.lr.x);
			IOREAD_U16(buf, 2, cam->ccd.array_area.lr.y);

			rlen = 4; wlen = 2;
			IOWRITE_U16(buf, 0, FLI_USBCAM_IMAGEOFFSET);
			IO(dev, buf, &wlen, &rlen);
			IOREAD_U16(buf, 0, cam->ccd.visible_area.ul.x);
			IOREAD_U16(buf, 2, cam->ccd.visible_area.ul.y);

			rlen = 4; wlen = 2;
			IOWRITE_U16(buf, 0, FLI_USBCAM_IMAGESIZE);
			IO(dev, buf, &wlen, &rlen);
			IOREAD_U16(buf, 0, cam->ccd.visible_area.lr.x);
			cam->ccd.visible_area.lr.x += cam->ccd.visible_area.ul.x;
			IOREAD_U16(buf, 2, cam->ccd.visible_area.lr.y);
			cam->ccd.visible_area.lr.y += cam->ccd.visible_area.ul.y;

			/* This is added as a hack to allow for overscan of CCD
			 * this should be moved somewhere else */
#ifdef _WIN32
			/* Check the registry to determine if we are overriding any settings */
			{
				HKEY hKey;
				DWORD overscan_x = 0, overscan_y = 0;
				DWORD whole_array = 0;
				DWORD len;

				if (RegOpenKey(HKEY_LOCAL_MACHINE,
					"SOFTWARE\\Finger Lakes Instrumentation\\libfli",
					&hKey) == ERROR_SUCCESS)
				{
					/* Check for overscan data */

					len = sizeof(DWORD);
					if (RegQueryValueEx(hKey, "overscan_x", NULL, NULL, (LPBYTE) &overscan_x, &len) == ERROR_SUCCESS)
					{
						debug(FLIDEBUG_INFO, "Found a request for horizontal overscan of %d pixels.", overscan_x);
					}

					len = sizeof(DWORD);
					if (RegQueryValueEx(hKey, "overscan_y", NULL, NULL, (LPBYTE) &overscan_y, &len) == ERROR_SUCCESS)
					{
						debug(FLIDEBUG_INFO, "Found a request for vertical overscan of %d pixels.", overscan_y);
					}

					len = sizeof(DWORD);
					RegQueryValueEx(hKey, "whole_array", NULL, NULL, (LPBYTE) &whole_array, &len);

					cam->ccd.array_area.ul.x = 0;
					cam->ccd.array_area.ul.y = 0;
					cam->ccd.array_area.lr.x += overscan_x;
					cam->ccd.array_area.lr.y += overscan_y;

					if (whole_array == 0)
					{
						cam->ccd.visible_area.lr.x += overscan_x;
						cam->ccd.visible_area.lr.y += overscan_y;
					}
					else
					{
						cam->ccd.visible_area.ul.x = 0;
						cam->ccd.visible_area.ul.y = 0;
						cam->ccd.visible_area.lr.x = cam->ccd.array_area.lr.x;
						cam->ccd.visible_area.lr.y = cam->ccd.array_area.lr.y;
					}
					RegCloseKey(hKey);
				}
				else
				{
					debug(FLIDEBUG_INFO, "Could not find registry key.");
				}
			}
#endif

			/* Initialize all variables to something */

			cam->vflushbin = 4;
			cam->hflushbin = 4;
			cam->vbin = 1;
			cam->hbin = 1;
			cam->image_area.ul.x = cam->ccd.visible_area.ul.x;
			cam->image_area.ul.y = cam->ccd.visible_area.ul.y;
			cam->image_area.lr.x = cam->ccd.visible_area.lr.x;
			cam->image_area.lr.y = cam->ccd.visible_area.lr.y;
			cam->exposure = 100;
			cam->frametype = FLI_FRAME_TYPE_NORMAL;
			cam->flushes = 0;
			cam->bitdepth = FLI_MODE_16BIT;
			cam->exttrigger = 0;
			cam->exttriggerpol = 0;
			cam->background_flush = 1;

			cam->grabrowwidth =
				(cam->image_area.lr.x - cam->image_area.ul.x) / cam->hbin;
			cam->grabrowcount = 1;
			cam->grabrowcounttot = cam->grabrowcount;
			cam->grabrowindex = 0;
			cam->grabrowbatchsize = 1;
			cam->grabrowbufferindex = cam->grabrowcount;
			cam->flushcountbeforefirstrow = 0;
			cam->flushcountafterlastrow = 0;

#ifdef _SETUPDEFAULTS
			/* Now to set up the camera defaults */
			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETFRAMEOFFSET);
			IOWRITE_U16(buf, 2, cam->image_area.ul.x);
			IOWRITE_U16(buf, 4, cam->image_area.ul.y);
			IO(dev, buf, &wlen, &rlen);

			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETBINFACTORS);
			IOWRITE_U16(buf, 2, cam->hbin);
			IOWRITE_U16(buf, 4, cam->vbin);
			IO(dev, buf, &wlen, &rlen);

			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETFLUSHBINFACTORS);
			IOWRITE_U16(buf, 2, cam->hflushbin);
			IOWRITE_U16(buf, 4, cam->vflushbin);
			IO(dev, buf, &wlen, &rlen);
#endif

		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			DEVICE->devinfo.devid = FLIUSB_PROLINE_ID;

			/* Let's get information about the hardware */
			wlen = 2; rlen = 6;
			IOWRITE_U16(buf, 0, PROLINE_GET_HARDWAREINFO);
			IO(dev, buf, &wlen, &rlen);
			IOREAD_U16(buf, 0, DEVICE->devinfo.hwrev);
			IOREAD_U16(buf, 2, DEVICE->devinfo.serno);
			IOREAD_U16(buf, 4, rlen);

			/* Configuration data from ProLine is little endian, I can't believe
			 * that I did this oh well, I'll deal with it! (Well, SDCC did it...)
			 */

			if (DEVICE->devinfo.hwrev >= 0x0100)
			{
				wlen = 2;
				IOWRITE_U16(buf, 0, PROLINE_GET_CAMERAINFO);
				IO(dev, buf, &wlen, &rlen);

				cam->ccd.array_area.ul.x = 0;
				cam->ccd.array_area.ul.y = 0;
				cam->ccd.array_area.lr.x = (buf[1] << 8) + buf[0];
				cam->ccd.array_area.lr.y = (buf[3] << 8) + buf[2];

				cam->ccd.visible_area.ul.x = (buf[9] << 8) + buf[8];
				cam->ccd.visible_area.ul.y = (buf[11] << 8) + buf[10];
				cam->ccd.visible_area.lr.x = (buf[5] << 8) + buf[4] + cam->ccd.visible_area.ul.x;
				cam->ccd.visible_area.lr.y = (buf[7] << 8) + buf[6] + cam->ccd.visible_area.ul.y;

				cam->ccd.pixelwidth = dconvert(&buf[12]);
				cam->ccd.pixelheight = dconvert(&buf[16]);

				cam->capabilities = buf[21] + (buf[22] << 8) + (buf[23] << 16) + (buf[24] << 24);

				rlen = 64; wlen = 2;
				IOWRITE_U16(buf, 0, PROLINE_GET_DEVICESTRINGS);
				IO(dev, buf, &wlen, &rlen);
				DEVICE->devinfo.devnam = xstrndup((char *) &buf[0], 32);
				DEVICE->devinfo.model = xstrndup((char *) &buf[32], 32);
			}

			/* FW dependent capabilities */
			if (DEVICE->devinfo.fwrev >= 0x0110)
			{
				cam->capabilities |= CAPABILITY_TDI;
				cam->capabilities |= CAPABILITY_BGFLUSH;
			}

			debug(FLIDEBUG_INFO, "Device has following capabilities:");

			if SUPPORTS_VIDEO(DEVICE) debug(FLIDEBUG_INFO, "   SUPPORTS_VIDEO");
			if SUPPORTS_TDI(DEVICE) debug(FLIDEBUG_INFO, "   SUPPORTS_TDI");
			if SUPPORTS_BGFLUSH(DEVICE) debug(FLIDEBUG_INFO, "   SUPPORTS_BGFLUSH");
			if SUPPORTS_END_EXPOSURE(DEVICE) debug(FLIDEBUG_INFO, "   SUPPORTS_END_EXPOSURE");
			if SUPPORTS_SOFTWARE_TRIGGER(DEVICE) debug(FLIDEBUG_INFO, "   SUPPORTS_SOFTWARE_TRIGGER");

			/* Initialize all varaibles to something */
			cam->vflushbin = 0;
			cam->hflushbin = 0;
			cam->vbin = 1;
			cam->hbin = 1;
			cam->image_area.ul.x = cam->ccd.visible_area.ul.x;
			cam->image_area.ul.y = cam->ccd.visible_area.ul.y;
			cam->image_area.lr.x = cam->ccd.visible_area.lr.x;
			cam->image_area.lr.y = cam->ccd.visible_area.lr.y;
			cam->exposure = 100;
			cam->frametype = FLI_FRAME_TYPE_NORMAL;
			cam->flushes = 0;
			cam->bitdepth = FLI_MODE_16BIT;
			cam->exttrigger = 0;
			cam->exttriggerpol = 0;
			cam->background_flush = 1;
			cam->tempslope = 1.0;
			cam->tempintercept = 0.0;

			cam->grabrowwidth =
				(cam->image_area.lr.x - cam->image_area.ul.x) / cam->hbin;
			cam->grabrowcount = 1;
			cam->grabrowcounttot = cam->grabrowcount;
			cam->grabrowindex = 0;
			cam->grabrowbatchsize = 1;
			cam->grabrowbufferindex = cam->grabrowcount;
			cam->flushcountbeforefirstrow = 0;
			cam->flushcountafterlastrow = 0;

			/* Now to set up the camera defaults */
		}
		break;

		default:
			return -ENODEV;
	}

	debug(FLIDEBUG_INFO, "DeviceID %d", DEVICE->devinfo.devid);
	debug(FLIDEBUG_INFO, "SerialNum %d", DEVICE->devinfo.serno);
	debug(FLIDEBUG_INFO, "HWRev %04x", DEVICE->devinfo.hwrev);
	debug(FLIDEBUG_INFO, "FWRev %04x", DEVICE->devinfo.fwrev);

	debug(FLIDEBUG_INFO, "     Name: %s", DEVICE->devinfo.devnam);
	debug(FLIDEBUG_INFO, "    Array: (%4d,%4d),(%4d,%4d)",
	cam->ccd.array_area.ul.x,
	cam->ccd.array_area.ul.y,
	cam->ccd.array_area.lr.x,
	cam->ccd.array_area.lr.y);
	debug(FLIDEBUG_INFO, "  Visible: (%4d,%4d),(%4d,%4d)",
	cam->ccd.visible_area.ul.x,
	cam->ccd.visible_area.ul.y,
	cam->ccd.visible_area.lr.x,
	cam->ccd.visible_area.lr.y);

	debug(FLIDEBUG_INFO, " Pix Size: (%g, %g)", cam->ccd.pixelwidth, cam->ccd.pixelheight);
	debug(FLIDEBUG_INFO, "    Temp.: T = AD x %g + %g", cam->tempslope, cam->tempintercept);

	return 0;
}

long fli_camera_usb_get_array_area(flidev_t dev, long *ul_x, long *ul_y,
				   long *lr_x, long *lr_y)
{
  flicamdata_t *cam = DEVICE->device_data;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{


		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{


		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	*ul_x = cam->ccd.array_area.ul.x;
	*ul_y = cam->ccd.array_area.ul.y;
	*lr_x = cam->ccd.array_area.lr.x;
	*lr_y = cam->ccd.array_area.lr.y;
	return 0;
}

long fli_camera_usb_get_visible_area(flidev_t dev, long *ul_x, long *ul_y,
				     long *lr_x, long *lr_y)
{
  flicamdata_t *cam = DEVICE->device_data;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{


		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{


		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  *ul_x = cam->ccd.visible_area.ul.x;
  *ul_y = cam->ccd.visible_area.ul.y;
  *lr_x = cam->ccd.visible_area.lr.x;
  *lr_y = cam->ccd.visible_area.lr.y;

  return 0;
}

long fli_camera_usb_set_exposure_time(flidev_t dev, unsigned long exptime)
{
  flicamdata_t *cam = DEVICE->device_data;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			long rlen, wlen;
			iobuf_t buf[8];
			rlen = 0; wlen = 8;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETEXPOSURE);
			IOWRITE_U32(buf, 4, exptime);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{

		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  cam->exposure = exptime;

	return 0;
}

long fli_camera_usb_set_image_area(flidev_t dev, long ul_x, long ul_y,
				   long lr_x, long lr_y)
{
  flicamdata_t *cam = DEVICE->device_data;
	int r = 0;

	if( (DEVICE->devinfo.fwrev < 0x0300) &&
			((DEVICE->devinfo.hwrev & 0xff00) == 0x0100) &&
			(DEVICE->devinfo.devid != FLIUSB_PROLINE_ID))
	{
		if( (lr_x > (cam->ccd.visible_area.lr.x * cam->hbin)) ||
				(lr_y > (cam->ccd.visible_area.lr.y * cam->vbin)) )
		{
			debug(FLIDEBUG_WARN,
				"Area out of bounds: (%4d,%4d),(%4d,%4d)",
				ul_x, ul_y, lr_x, lr_y);
			return -EINVAL;
		}
	}

	if( (ul_x < 0) ||
			(ul_y < 0) )
	{
		debug(FLIDEBUG_FAIL,
			"Area out of bounds: (%4d,%4d),(%4d,%4d)",
			ul_x, ul_y, lr_x, lr_y);
		return -EINVAL;
	}

	debug(FLIDEBUG_INFO,
		"Setting image area to: (%4d,%4d),(%4d,%4d)", ul_x, ul_y, lr_x, lr_y);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			long rlen, wlen;
			iobuf_t buf[IOBUF_MAX_SIZ];

			memset(buf, 0x00, IOBUF_MAX_SIZ);

			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETFRAMEOFFSET);
			IOWRITE_U16(buf, 2, ul_x);
			IOWRITE_U16(buf, 4, ul_y);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			/* JIM! perform some bounds checking... */

			/* Remember TDI imaging does not have a limit on vertical height */
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	if (r == 0)
	{
		cam->image_area.ul.x = ul_x;
	  cam->image_area.ul.y = ul_y;
		cam->image_area.lr.x = lr_x;
		cam->image_area.lr.y = lr_y;
		cam->grabrowwidth = (cam->image_area.lr.x - cam->image_area.ul.x) / cam->hbin;
	}

  return 0;
}

long fli_camera_usb_set_hbin(flidev_t dev, long hbin)
{
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
//	long r = 0;
	flicamdata_t *cam = DEVICE->device_data;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			if ((hbin < 1) || (hbin > 16))
				return -EINVAL;

			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETBINFACTORS);
			IOWRITE_U16(buf, 2, hbin);
			IOWRITE_U16(buf, 4, cam->vbin);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			if ((hbin < 1) || (hbin > 255))
				return -EINVAL;

			/* We do nothing here, h_bin is sent with start exposure command
			   this may be a bug, TDI imaging will require this */
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  cam->hbin = hbin;
  cam->grabrowwidth =
    (cam->image_area.lr.x - cam->image_area.ul.x) / cam->hbin;

  return 0;
}

long fli_camera_usb_set_vbin(flidev_t dev, long vbin)
{
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
//	long r = 0;

	flicamdata_t *cam = DEVICE->device_data;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			if ((vbin < 1) || (vbin > 16))
				return -EINVAL;

			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETBINFACTORS);
			IOWRITE_U16(buf, 2, cam->hbin);
			IOWRITE_U16(buf, 4, vbin);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			/* We do nothing here, h_bin is sent with start exposure command
			   this may be a bug, TDI imaging will require this */

			if ((vbin < 1) || (vbin > 255))
				return -EINVAL;


		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  cam->vbin = vbin;
  return 0;
}

long fli_camera_usb_get_exposure_status(flidev_t dev, long *timeleft)
{
	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			long rlen, wlen;
			iobuf_t buf[4];
			rlen = 4; wlen = 2;
			IOWRITE_U16(buf, 0, FLI_USBCAM_EXPOSURESTATUS);
			IO(dev, buf, &wlen, &rlen);
			IOREAD_U32(buf, 0, *timeleft);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			long rlen, wlen;
			iobuf_t buf[IOBUF_MAX_SIZ];

			rlen = 4; wlen = 2;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_GET_EXPOSURE_STATUS);
			IO(dev, buf, &wlen, &rlen);

			*timeleft = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return 0;
}

long fli_camera_usb_cancel_exposure(flidev_t dev)
{
	flicamdata_t *cam = DEVICE->device_data;

	cam->tdirate = 0;
	cam->tdiflags = 0;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			long rlen, wlen;
			iobuf_t buf[IOBUF_MAX_SIZ];

			rlen = 0; wlen = 4;
			IOWRITE_U16(buf, 0, FLI_USBCAM_ABORTEXPOSURE);
			IO(dev, buf, &wlen, &rlen);

			/* MaxCam (bug in firmware prevents shutter closing), so issue quick exposure... */
			rlen = 0; wlen = 8; /* Bias frame */
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETEXPOSURE);
			IOWRITE_U32(buf, 4, 10);
			IO(dev, buf, &wlen, &rlen);

			/* Expose the bias frame */
			rlen = 0; wlen = 4;
			IOWRITE_U16(buf, 0, FLI_USBCAM_STARTEXPOSURE);
			IOWRITE_U16(buf, 2, 0);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			long rlen = 2, wlen = 2;
			iobuf_t buf[IOBUF_MAX_SIZ];

			IOWRITE_U16(buf, 0, PROLINE_COMMAND_CANCEL_EXPOSURE);
			IO(dev, buf, &wlen, &rlen);

			cam->video_mode = VIDEO_MODE_OFF;
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return 0;
}


long fli_camera_usb_set_temperature(flidev_t dev, double temperature)
{
  flicamdata_t *cam = DEVICE->device_data;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			unsigned short ad;
			long rlen, wlen;
			iobuf_t buf[4];

			if(DEVICE->devinfo.fwrev < 0x0200)
				return 0;

			if(cam->tempslope == 0.0)
				ad = 255;
			else
				ad = (unsigned short) ((temperature - cam->tempintercept) / cam->tempslope);

			debug(FLIDEBUG_INFO, "Temperature slope, intercept, AD val, %f %f %f %d", temperature, cam->tempslope, cam->tempintercept, ad);

			rlen = 0; wlen = 4;
			IOWRITE_U16(buf, 0, FLI_USBCAM_TEMPERATURE);
			IOWRITE_U16(buf, 2, ad);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			long rlen, wlen;
			iobuf_t buf[IOBUF_MAX_SIZ];

			unsigned short a;

			short s_temp;

			s_temp = (short) (temperature * 256.0);
			rlen = 2; wlen = 4;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_SET_TEMPERATURE);
			IOWRITE_U16(buf, 2, s_temp);
			IO(dev, buf, &wlen, &rlen);

			a = (buf[0] << 8) + buf[1];
			debug(FLIDEBUG_INFO, "Got %d from camera.", a);
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return 0;
}

long fli_camera_usb_read_temperature(flidev_t dev, flichannel_t channel, double *temperature)
{
	flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			if (channel == 0)
			{
				rlen = 2; wlen = 2;
				IOWRITE_U16(buf, 0, FLI_USBCAM_TEMPERATURE);
				IO(dev, buf, &wlen, &rlen);
				*temperature = cam->tempslope * ((double) buf[1]) +
					cam->tempintercept;
			}
			else
			{
				*temperature = (0.0);
			}
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			double base, ccd;

			rlen = 14; wlen = 2;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_GET_TEMPERATURE);
			IO(dev, buf, &wlen, &rlen);

			ccd = (double) ((signed char) buf[0]) + ((double) buf[1] / 256);
			base = (double) ((signed char) buf[2]) + ((double) buf[3] / 256);

			switch (channel)
			{
				case FLI_TEMPERATURE_CCD:
					*temperature = ccd;
				break;

				case FLI_TEMPERATURE_BASE:
					*temperature = base;
				break;

				default:
					r = -EINVAL;
				break;
			}
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_get_temperature(flidev_t dev, double *temperature)
{
	return fli_camera_usb_read_temperature(dev, 0, temperature);
}

long fli_camera_usb_grab_row(flidev_t dev, void *buff, size_t width)
{
	flicamdata_t *cam = DEVICE->device_data;
	int abort = 0;

	if(width > (size_t) (cam->image_area.lr.x - cam->image_area.ul.x))
	{
		debug(FLIDEBUG_FAIL, "Requested row too wide, truncating.");
		debug(FLIDEBUG_FAIL, "  Requested width: %d", width);
		debug(FLIDEBUG_FAIL, "  Set width: %d",
			cam->image_area.lr.x - cam->image_area.ul.x);

		width = cam->image_area.lr.x - cam->image_area.ul.x;
	}

	if (cam->gbuf == NULL)
		return -ENOMEM;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			long x;
			long r;

			if (cam->flushcountbeforefirstrow > 0)
			{
				debug(FLIDEBUG_INFO, "Flushing %d rows before image download.", cam->flushcountbeforefirstrow);
				if ((r = fli_camera_usb_flush_rows(dev, cam->flushcountbeforefirstrow, 1)))
					return r;

				cam->flushcountbeforefirstrow = 0;
			}

			if (cam->grabrowbufferindex >= cam->grabrowbatchsize)
			{
				/* We don't have the row in memory */
				long rlen, wlen;

				/* Do we have less than GrabRowBatchSize rows to grab? */
				if (cam->grabrowbatchsize > (cam->grabrowcounttot - cam->grabrowindex))
				{
					cam->grabrowbatchsize = cam->grabrowcounttot - cam->grabrowindex;

					if (cam->grabrowbatchsize < 1)
						cam->grabrowbatchsize = 1;
				}

				debug(FLIDEBUG_INFO, "Grabbing %d rows of width %d.", cam->grabrowbatchsize, cam->grabrowwidth);
				rlen = cam->grabrowwidth * 2 * cam->grabrowbatchsize;
				wlen = 6;
				cam->gbuf[0] = htons(FLI_USBCAM_SENDROW);
				cam->gbuf[1] = htons((unsigned short) cam->grabrowwidth);
				cam->gbuf[2] = htons((unsigned short) cam->grabrowbatchsize);
				IO(dev, cam->gbuf, &wlen, &rlen);

				for (x = 0; x < (cam->grabrowwidth * cam->grabrowbatchsize); x++)
				{
					if ((DEVICE->devinfo.hwrev & 0xff00) == 0x0100)
					{
						cam->gbuf[x] = ntohs(cam->gbuf[x]) + 32768;
					}
					else
					{
						cam->gbuf[x] = ntohs(cam->gbuf[x]);
					}
				}
				cam->grabrowbufferindex = 0;
			}

			for (x = 0; x < (long)width; x++)
			{
				((unsigned short *)buff)[x] =
					cam->gbuf[x + (cam->grabrowbufferindex * cam->grabrowwidth)];
			}

			cam->grabrowbufferindex++;
			cam->grabrowindex++;

			if (cam->grabrowcount > 0)
			{
				cam->grabrowcount--;
				if (cam->grabrowcount == 0)
				{
					if (cam->flushcountafterlastrow > 0)
					{
						debug(FLIDEBUG_INFO, "Flushing %d rows after image download.", cam->flushcountafterlastrow);
						if ((r = fli_camera_usb_flush_rows(dev, cam->flushcountafterlastrow, 1)))
							return r;
					}

					cam->flushcountafterlastrow = 0;
					cam->grabrowbatchsize = 1;
				}
			}


		}
		break;

#ifdef OLD_PROLINE

		/* Proline/Microline Camera */
		case FLIUSB_PROLINE_ID+1:
		{
			long rlen, rtotal;

			/* First we need to determine if the row is in memory */
			while ( (cam->grabrowcounttot < cam->grabrowwidth) && (abort == 0) )
			{
				int loadindex = 0;
				long rowsleft, bytesleft, wordsleft;

				/* Ring buffer for image download... ideally this should just
				 * swap from top to bottom as 1/2 the buffer is filled each time.
				 * For single row grabs
				 *
				 * cam->gbuf_siz -- size of the grab buffer (bytes)
				 * cam->max_usb_xfer -- size of the maximum USB transfer (bytes)
				 * cam->grabrowindex -- current row being grabbed
				 * cam->grabrowcounttot -- number of words left in buffer (words)
				 * cam->grabrowbufferindex -- location of the beginning of the row in the buffer in words
				 *
				 */

				/* Let's fill the buffer */
				rlen = (cam->gbuf_siz / 2) - (cam->grabrowbufferindex + cam->grabrowcounttot);

				/* Words to bytes */
				rlen *= 2;

				if (rlen < 0)
				{
					debug(FLIDEBUG_FAIL, "READ, rlen < 0!");
					abort = 1;
					continue;
				} else if (rlen == 0)
				{
					/* For this to be true we must have the buffer completely filled
					 * so we start back at the beginning
					 */

					rlen = cam->max_usb_xfer;
					loadindex = 0;
				} else
				{
					loadindex = cam->grabrowbufferindex + cam->grabrowcounttot;
				}

				/* At this point rlen is positive and non-zero
				 * we should constrain its limit to no more than the
				 * data we are expecting from the camera. Furthermore,
				 * we may just need to fill to the top of the buffer then
				 * wrap around...
				 */

				if (cam->tdirate == 0)
				{
					rowsleft = cam->grabrowcount - cam->grabrowindex;
					wordsleft = (rowsleft * cam->grabrowwidth) - cam->grabrowcounttot;
					bytesleft = wordsleft * 2;
				}
				else
				{
				/* For TDI imaging we only want one row at a time, must be rounded up
				 * to 512 bytes wide */
					bytesleft = (cam->grabrowwidth - cam->grabrowcounttot) * 2;

					if (bytesleft & 0x1ff)
					{
						debug(FLIDEBUG_WARN, "TDI row width must be multiple of 512 bytes!");
						abort = 1;
						continue;
					}
				}

				if (rlen > bytesleft) rlen = bytesleft;
				if (rlen > cam->max_usb_xfer) rlen = cam->max_usb_xfer;

				memset(&cam->gbuf[loadindex], 0x00, rlen);
				rtotal = rlen;

				debug(FLIDEBUG_INFO, "Transfer, Base: %p Start: %p End: %p Size: %d",
					&cam->gbuf[0], &cam->gbuf[loadindex], &cam->gbuf[loadindex + rlen / 2], rlen);

#ifdef CHECK_STATUS
				do
				{


				} while (status &

#endif

				if ((usb_bulktransfer(dev, 0x82, &cam->gbuf[loadindex], &rlen)) != 0) /* Grab the buffer */
				{
					debug(FLIDEBUG_FAIL, "Read failed...");
					abort = 1;
				}

				if ((rlen < rtotal) && (cam->grabrowindex > 0))
				{
					char b[2048];

#ifdef _WIN32
					sprintf(b, "Pad, L:%d\n", cam->grabrowindex);
					OutputDebugString(b);
#endif
					debug(FLIDEBUG_FAIL, "Transfer did not complete, padding...");
					memset(&cam->gbuf[cam->grabrowcounttot], 0x00, (rtotal - rlen));
				}
				cam->grabrowcounttot += (rlen / 2);
			}

			/* Double check that row is in memory (an IO operation could have failed.) */
			if ( (abort == 0) && (cam->grabrowcounttot >= cam->grabrowwidth) )
			{
				long l = 0;

				while (l < cam->grabrowwidth)
				{
					/* Are we at the end of the buffer? */
					if ((cam->grabrowbufferindex + cam->grabrowwidth) < ((cam->max_usb_xfer / 2) * 2) )
					{
						/* Not near end of buffer */
						while (l < cam->grabrowwidth)
						{
							if (l < width)
								((unsigned short *) buff)[l] = ((cam->gbuf[cam->grabrowbufferindex] << 8) & 0xff00) | ((cam->gbuf[cam->grabrowbufferindex] >> 8) & 0x00ff);

							cam->grabrowbufferindex ++;
							l ++;
						}
					}
					else
					{
						/* Near end of buffer */
						while (cam->grabrowbufferindex < ((cam->max_usb_xfer / 2) * 2))
						{
							if (l < width)
								((unsigned short *) buff)[l] = ((cam->gbuf[cam->grabrowbufferindex] << 8) & 0xff00) | ((cam->gbuf[cam->grabrowbufferindex] >> 8) & 0x00ff);

							cam->grabrowbufferindex ++;
							l ++;
						}
						cam->grabrowbufferindex = 0;
					}
				}

				cam->grabrowcounttot -= cam->grabrowwidth;
				cam->grabrowindex ++;
			}
		}
		break;
#endif
		/* New code */
		case FLIUSB_PROLINE_ID:
		{
			long rlen = 0, rtotal = 0;
			int index = 0;

			/*
			 * cam->gbuf_siz -- size of the grab buffer (bytes)
			 * cam->ibuf_siz -- size of image buffer (bytes)
			 * cam->max_usb_xfer -- size of the maximum USB transfer (bytes)
			 * cam->grabrowindex -- current row being grabbed
			 * cam->grabrowcounttot --
			 * cam->grabrowbufferindex --
			 * cam->bytesleft -- number of bytes left to acquire from camera
			 */

			long top = 1;
			long di = 1;
			long row_idx;
			long to, bo, lo, ro;
			long th, bh, lw, rw;
			long w;
			unsigned short *left, *right, *ibuf;

			/* Normalize the offsets */
			to = cam->top_offset - MIN(cam->top_offset, cam->bottom_offset);
			bo = cam->bottom_offset - MIN(cam->top_offset, cam->bottom_offset);
			lo = cam->left_offset - MIN(cam->left_offset, cam->right_offset);
			ro = cam->right_offset - MIN(cam->left_offset, cam->right_offset);

			/* Make these nicer to use */
			th = cam->top_height;
			bh = cam->bottom_height;
			lw = cam->left_width;
			rw = cam->right_width;
			row_idx = cam->grabrowindex;
			w = lw + rw;

			left = (unsigned short *) buff;
			right = (unsigned short *) buff + w;

			ibuf = cam->ibuf;

			/* Fix these so that data is "contiguous" */
			if (bo > th) bo = th; /* Bottom data starts immediately after top data */
			if (to > bh) to = bh; /* Top data starts immediately after bottom data */

			/* Top data is first */
			if (to == 0) /* Top is first data (bo can be zero also without a problem) */
			{
				/* Now determine bottom or top */
				if (row_idx < th) /* Top */
				{
					if (row_idx < bo) /* No Bottom Data yet */
					{
						ibuf += w * row_idx;
						di = 1;
					}
					else /* Bottom Data mixed in */
					{
						ibuf += w * bo; /* Take us to where the bottom data starts */

						if (row_idx < (bo + bh)) /* Still with bottom data around */
						{
							ibuf += w * 2 * (row_idx - bo);
							di = 2;
						}
						else /* Past bottom data */
						{
							ibuf += w * 2 * bh;
							ibuf += w * (row_idx - (bo + bh));
							di = 1;
						}
					}
				}
				else if (row_idx < (th + bh)) /* Bottom */
				{
					top = 0; /* Bottom Data */
					row_idx -= th; /* Normalize */

					ibuf = cam->ibuf + ((th + bh) * w); /* End of buffer */

					if (row_idx < ((bo + bh) - th)) /* No Top Data yet */
					{
						ibuf -= w * row_idx;
						di = 1;
					}
					else /* Top Data mixed in */
					{
						if (((bo + bh) - th) > 0)
						{
							ibuf -= w * ((bo + bh) - th); /* Take us to where the bottom data starts */
							row_idx -= ((bo + bh) - th);
						}

						if (row_idx < (to + th)) /* Still with bottom data around */
						{
							ibuf -= w * 2 * (row_idx - to);
//							ibuf ++; /* Re-align */
							di = 2;
						}
						else /* Past top data */
						{
							ibuf -= w * 2 * th;
							ibuf -= w * (row_idx - (to + th));
							di = 1;
						}
					}

					ibuf -= w * di; /* Position at the beginning of the row */
				}
				else
				{
					/* We shouldn't be here */
				}
			}
			else /* to != 0, bottom data has started */
			{
				/* Now determine bottom or top */
				if (row_idx < th) /* Top */
				{
					ibuf = cam->ibuf + w * to; /* Beginning of data */

					if (row_idx < (bh - to)) /* Bottom data intermixed */
					{
						ibuf += w * 2 * row_idx;
						di = 2;
					}
					else /* Past bottom data */
					{
						if ((bh - to) >= 0)
						{
							ibuf += w * 2 * (bh - to); /* Move past shared data */

							ibuf += w * (row_idx - (bh - to));
							di = 1;
						}
						else
						{
							ibuf += w * row_idx;
							di = 1;
						}
					}
				}
				else if (row_idx < (th + bh)) /* Bottom */
				{
					top = 0;
					row_idx -= th; /* Normalize the index in terms of top rows */

					ibuf = cam->ibuf + ((th + bh) * w); /* End of buffer */

					if (row_idx < (bh - (to + th))) /* Past Top Data */
					{
						ibuf -= w * row_idx;
						di = 1;
					}
					else if (row_idx < (bh - to)) /* Mixed Data */
					{
						if ((bh - (to + th)) > 0) /* Position ourselves */
						{
							ibuf -= w * (bh - (to + th));
							row_idx -= (bh - (to + th));
						}

						ibuf -= w * 2 * row_idx;
						di = 2;
					}
					else
					{
						if ((bh - (to + th)) > 0) /* Position ourselves */
						{
							ibuf -= w * (bh - (to + th));
							row_idx -= (bh - (to + th));
						}

						if ((bh - to) > 0)
						{
							ibuf -= w * 2 * (bh - to);
							row_idx -= (bh - to);
						}

						ibuf -= w * row_idx;

						di = 1;
					}

					ibuf -= w * di; /* Position at the beginning of the row */
				}
				else
				{
					/* We shouldn't be here */
				}
			}

			/* First we need to determine if the row is in memory */
			while ((cam->ibuf_wr_idx < (ibuf + w * di)) && (abort == 0) && (cam->bytesleft > 0))
			{
				/* Let's get some more from the camera */

				/* Not performing TDI */
				if (cam->tdirate == 0)
				{
					rlen = (long) MIN(cam->bytesleft, (size_t) cam->max_usb_xfer);
				}
				else
				/* For TDI imaging we only want one row at a time, must be rounded up
				 * to 512 bytes wide */
				{
					rlen = cam->grabrowwidth * 2;

					if (rlen & 0x1ff)
					{
						debug(FLIDEBUG_WARN, "TDI row download width must be multiple of 512 bytes!");
						abort = 1;
						continue;
					}
				}

				memset(cam->gbuf, 0x00, rlen);
				rtotal = rlen;

				if ((usb_bulktransfer(dev, 0x82, cam->gbuf, &rlen)) != 0) /* Grab the buffer */
				{
					debug(FLIDEBUG_FAIL, "Read failed...");
					abort = 1;
				}

				if (rlen < rtotal)
				{
					debug(FLIDEBUG_FAIL, "Transfer did not complete...");
				}

				if (rlen == 0x03) /* This is a special case, the camera is telling us there
													 * is no more data, something went wrong */
				{
					cam->bytesleft = 0;
				}
				else
				{
					cam->bytesleft -= rlen;
				}

				for (index = 0; index < (rlen / (long) sizeof(unsigned short)); index ++)
				{
					*cam->ibuf_wr_idx = ((cam->gbuf[index] << 8) & 0xff00) | ((cam->gbuf[index] >> 8) & 0x00ff);
					cam->ibuf_wr_idx++;
				}
			}

			memset(left, 0x00, width * sizeof(unsigned short));

			/* Double check that row is in memory (an IO operation could have failed.) */
			if (cam->ibuf_wr_idx >= (ibuf + w * di))
			{
				/* Top data only */
				if (top == 1)
				{
//					long r = row_idx;

					/* Beginning of row, left portion of data */
					while ( (ro > 0) && (left < right) )
					{
						*left = *ibuf;
						left++;

						lw--;
						ro--;
						ibuf += di;
					}

					/* Beginning of row, right portion of data */
					while ( (lo > 0) && (left < right) )
					{
						right--;
						*right = *ibuf;

						rw--;
						lo--;
						ibuf += di;
					}

					/* Both portions of data, middle of the row */
					while ( ((rw > 0) && (lw > 0)) && (left < right) )
					{
						*left = *ibuf;
						left ++;
						lw --;
						ibuf += di;

						--right;
						*right = *ibuf;
						rw --;
						ibuf += di;
					}

					/* Remaining left data */
					while ( (lw > 0) && (left < right) )
					{
						*left = *ibuf;
						left++;

						lw--;
						ibuf += di;
					}

					/* Remaining right data */
					while ( (rw > 0) && (left < right) )
					{
						right--;
						*right = *ibuf;

						rw--;
						ibuf += di;
					}
				}
				else /* Bottom Data */
				{
//					long r = row_idx;

					 /* Re-align */
					if (di == 2)
					{
						ibuf ++;
						di = 2;
					}

					/* Beginning of row, left portion of data */
					while ( (ro > 0) && (left < right) )
					{
						*left = *ibuf;
						left++;

						lw--;
						ro--;
						ibuf += di;
					}

					/* Beginning of row, right portion of data */
					while ( (lo > 0) && (left < right) )
					{
						right--;
						*right = *ibuf;

						rw--;
						lo--;
						ibuf += di;
					}

					/* Both portions of data, middle of the row */
					while ( ((rw > 0) && (lw > 0)) && (left < right) )
					{
						*left = *ibuf;
						left ++;
						lw --;
						ibuf += di;

						--right;
						*right = *ibuf;
						rw --;
						ibuf += di;
					}

					/* Remaining left data */
					while ( (lw > 0) && (left < right) )
					{
						*left = *ibuf;
						left++;

						lw--;
						ibuf += di;
					}

					/* Remaining right data */
					while ( (rw > 0) && (left < right) )
					{
						right--;
						*right = *ibuf;

						rw--;
						ibuf += di;
					}
				}
			}
			cam->grabrowindex ++;
		}
		break;


		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}
	/* return IO error if the reading failed */
	if (abort) return -EIO;
	return 0;
}

long fli_camera_usb_stop_video_mode(flidev_t dev)
{
  flicamdata_t *cam = DEVICE->device_data;

	/* This function only works on specific prolines, use this function to
	 * determine if video mode is available on the camera. If it succeeds, then
	 * video mode is supported, if it fails, then it isn't. */
	if (!SUPPORTS_VIDEO(DEVICE))
	{
		debug(FLIDEBUG_FAIL, "Video mode not supported.");
		return -EINVAL;
	}

	if (cam->video_mode == VIDEO_MODE_OFF)
	{
		debug(FLIDEBUG_WARN, "Video mode not started.");
	}

	return fli_camera_usb_cancel_exposure(dev);
}

long fli_camera_usb_start_video_mode(flidev_t dev)
{
  flicamdata_t *cam = DEVICE->device_data;

	/* This function only works on specific prolines */
	if (!SUPPORTS_VIDEO(DEVICE))
	{
		debug(FLIDEBUG_FAIL, "Video mode not supported.");
		return -EINVAL;
	}

	if (cam->video_mode != VIDEO_MODE_OFF)
	{
		debug(FLIDEBUG_WARN, "Video mode already started, restarting...");
		fli_camera_usb_stop_video_mode(dev);
	}

	cam->video_mode = VIDEO_MODE_BEGIN;

	return fli_camera_usb_expose_frame(dev);
}

//long fli_camera_usb_prepare_video_frame(flidev_t dev)
//{
//  flicamdata_t *cam = DEVICE->device_data;
//
//	/* This function only works on specific cameras */
//	if (!SUPPORTS_VIDEO(cam))
//	{
//		debug(FLIDEBUG_FAIL, "Video mode not supported.");
//		return -EINVAL;
//	}
//
//	if (cam->video_mode != VIDEO_MODE_ON)
//	{
//		debug(FLIDEBUG_FAIL, "Video mode not started.");
//		return -EINVAL;
//	}
//
//	debug(FLIDEBUG_INFO, "Prepare Video Frame.");
//
//	/* Video mode is supported and started, for now implement with FLIGrabRow(),
//		this may not be fast enough in the future... */
//
//	/* Since this is proline/microline only, this is hacked from fli_camera_usb_expose_frame()
//	 * in the proline section, this is done only so that I can use
//	 * fli_camera_usb_grab_row() (YES! this is a hack!) */
//
//	cam->grabrowcount = cam->image_area.lr.y - cam->image_area.ul.y; // Rows High
//	cam->grabrowwidth = cam->image_area.lr.x - cam->image_area.ul.x; // Pixels Wide
//	cam->flushcountbeforefirstrow = cam->image_area.ul.y; // Vertical Offset
//	cam->grabrowindex = 0;
//	cam->grabrowbatchsize = 0;
//	cam->grabrowcounttot = 0;
//	cam->grabrowbufferindex = 0;
//	cam->flushcountafterlastrow = 0;
//	cam->ibuf_wr_idx = cam->ibuf;
//	cam->bytesleft = (cam->top_height + cam->bottom_height) *
//	(cam->left_width + cam->right_width) * sizeof(unsigned short);
//
//	return 0;
//}

long fli_camera_usb_grab_video_frame(flidev_t dev, void *buff, size_t size)
{
  flicamdata_t *cam = DEVICE->device_data;
	long y = 0;
	long status = 0;

	/* This function only works on specific cameras */
	if (!SUPPORTS_VIDEO(DEVICE))
	{
		debug(FLIDEBUG_FAIL, "Video mode not supported.");
		return -EINVAL;
	}

	if (cam->video_mode != VIDEO_MODE_ON)
	{
		debug(FLIDEBUG_FAIL, "Video mode not started.");
		return -EINVAL;
	}

	debug(FLIDEBUG_INFO, "Grab Video Frame.");

	/* Video mode is supported and started, for now implement with FLIGrabRow(),
		this may not be fast enough in the future... */

	/* Since this is proline/microline only, this is hacked from fli_camera_usb_expose_frame()
	 * in the proline section, this is done only so that I can use
	 * fli_camera_usb_grab_row() (YES! this is a hack!) */

	cam->grabrowcount = cam->image_area.lr.y - cam->image_area.ul.y; // Rows High
	cam->grabrowwidth = cam->image_area.lr.x - cam->image_area.ul.x; // Pixels Wide
	cam->flushcountbeforefirstrow = cam->image_area.ul.y; // Vertical Offset
	cam->grabrowindex = 0;
	cam->grabrowbatchsize = 0;
	cam->grabrowcounttot = 0;
	cam->grabrowbufferindex = 0;
	cam->flushcountafterlastrow = 0;
	cam->ibuf_wr_idx = cam->ibuf;
	cam->bytesleft = (cam->top_height + cam->bottom_height) *
		(cam->left_width + cam->right_width) * sizeof(unsigned short);

	if (size < (cam->grabrowcount * cam->grabrowwidth * sizeof(unsigned short)))
	{
		debug(FLIDEBUG_FAIL, "Buffer not large enough to receive frame.");
		return -ENOMEM;
	}

	status = 0;
  while ((status == 0) && (y < cam->grabrowcount))
	{
//		debug(FLIDEBUG_INFO, "Grabbing row %d of %d of width %d.", y, cam->grabrowcount, cam->grabrowwidth);

		status = fli_camera_usb_grab_row(dev, buff, cam->grabrowwidth);
//		((unsigned short *) buff) += cam->grabrowwidth;
		buff = ((unsigned short *) buff) + cam->grabrowwidth;

		y++;
	}

	return status;
}

long fli_camera_usb_set_tdi(flidev_t dev, flitdirate_t rate, flitdiflags_t flags)
{
  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	/* Some of these don't support TDI */
	if ( !SUPPORTS_TDI(DEVICE) || (rate < 0) )
	{
		return -EINVAL;
	}

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			/* These cameras don't support TDI */
			r = -EINVAL;
		}
		break;

		case FLIUSB_PROLINE_ID:
		{
			cam->tdirate = rate;
			cam->tdiflags = flags;

			rlen = 2; wlen = 6;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_SET_TDI_MODE);

			/* TDI Rate */
			IOWRITE_U32(buf, 2, cam->tdirate);

			/* Enable */
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return r;
}

long fli_camera_usb_expose_frame(flidev_t dev)
{
  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	if (cam->video_mode == VIDEO_MODE_ON)
	{
		debug(FLIDEBUG_FAIL, "Video mode has started.");
		return -EINVAL;
	}

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			short flags = 0;

			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETFRAMEOFFSET);
			IOWRITE_U16(buf, 2, cam->image_area.ul.x);
			IOWRITE_U16(buf, 4, cam->image_area.ul.y);
			IO(dev, buf, &wlen, &rlen);

			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETBINFACTORS);
			IOWRITE_U16(buf, 2, cam->hbin);
			IOWRITE_U16(buf, 4, cam->vbin);
			IO(dev, buf, &wlen, &rlen);

			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETFLUSHBINFACTORS);
			IOWRITE_U16(buf, 2, cam->hflushbin);
			IOWRITE_U16(buf, 4, cam->vflushbin);
			IO(dev, buf, &wlen, &rlen);

			rlen = 0; wlen = 8;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETEXPOSURE);
			IOWRITE_U32(buf, 4, cam->exposure);
			IO(dev, buf, &wlen, &rlen);

			/* What flags do we need to send... */
			/* Dark Frame */
			flags |= (cam->frametype & FLI_FRAME_TYPE_DARK) ? 0x01 : 0x00;
			/* External trigger */
			flags |= (cam->exttrigger != 0) ? 0x04 : 0x00;
			flags |= (cam->exttriggerpol != 0) ? 0x08 : 0x00;

			debug(FLIDEBUG_INFO, "Exposure flags: %04x", flags);
			debug(FLIDEBUG_INFO, "Flushing %d times.", cam->flushes);

			if (cam->flushes > 0)
			{
				long r;

				if ((r = fli_camera_usb_flush_rows(dev,
									cam->ccd.array_area.lr.y - cam->ccd.array_area.ul.y,
									cam->flushes)))
					return r;
			}

			debug(FLIDEBUG_INFO, "Starting exposure...");
			rlen = 0; wlen = 4;
			IOWRITE_U16(buf, 0, FLI_USBCAM_STARTEXPOSURE);
			IOWRITE_U16(buf, 2, flags);
			IO(dev, buf, &wlen, &rlen);

			cam->grabrowcount = cam->image_area.lr.y - cam->image_area.ul.y;
			cam->grabrowcounttot = cam->grabrowcount;
			cam->grabrowwidth = cam->image_area.lr.x - cam->image_area.ul.x;
			cam->grabrowindex = 0;
			if (cam->grabrowwidth > 0){
				cam->grabrowbatchsize = USB_READ_SIZ_MAX / (cam->grabrowwidth * 2);
			}
			else
			{
				return -1;
			}

			/* Lets put some bounds on this... */
			if (cam->grabrowbatchsize > cam->grabrowcounttot)
				cam->grabrowbatchsize = cam->grabrowcounttot;

			if (cam->grabrowbatchsize > 64)
				cam->grabrowbatchsize = 64;

			/* We need to get a whole new buffer by default */
			cam->grabrowbufferindex = cam->grabrowbatchsize;

			cam->flushcountbeforefirstrow = cam->image_area.ul.y;
			cam->flushcountafterlastrow =
				(cam->ccd.array_area.lr.y - cam->ccd.array_area.ul.y) -
				((cam->image_area.lr.y - cam->image_area.ul.y) * cam->vbin) -
				cam->image_area.ul.y;

			if (cam->flushcountbeforefirstrow < 0)
				cam->flushcountbeforefirstrow = 0;

			if (cam->flushcountafterlastrow < 0)
				cam->flushcountafterlastrow = 0;
		}
		break;

		case FLIUSB_PROLINE_ID:
		{
			short h_offset;
			size_t numpix;

			cam->grabrowcount = cam->image_area.lr.y - cam->image_area.ul.y; // Rows High
			cam->grabrowwidth = cam->image_area.lr.x - cam->image_area.ul.x; // Pixels Wide

			/* Row width in bytes must be multiple of 512 (256 pixels) so that
			 * single rows can be grabbed by FLIGrabRow
			 */

			if (cam->tdirate != 0)
			{
				if ((cam->grabrowwidth % 256) != 0)
					cam->grabrowwidth += (256 - (cam->grabrowwidth % 256));
			}

			cam->flushcountbeforefirstrow = cam->image_area.ul.y; // Vertical Offset
			h_offset = cam->image_area.ul.x; // Horizontal Offset

			cam->grabrowindex = 0;
			cam->grabrowbatchsize = 0;
			cam->grabrowcounttot = 0;
			cam->grabrowbufferindex = 0;
			cam->flushcountafterlastrow = 0;

			if (cam->grabrowwidth <= 0)
				return -EINVAL;

			/* Check FW revision, >= 2.0 returns a structure defining
			 * image parameters */

			if (DEVICE->devinfo.fwrev >= 0x0200)
			{
				rlen = 64; wlen = 64;
			}
			else
			{
				rlen = 0; wlen = 32;
			}
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_EXPOSE);

			/* Number of pixels wide */
			IOWRITE_U16(buf, 2, cam->grabrowwidth);

			/* Horizontal offset */
			IOWRITE_U16(buf, 4, h_offset);

			/* Number of vertical rows to grab */
			IOWRITE_U16(buf, 6, cam->grabrowcount);

			/* Vertical offset */
			IOWRITE_U16(buf, 8, cam->flushcountbeforefirstrow);

			/* Horizontal bin */
			IOWRITE_U8(buf, 10, cam->hbin);

			/* Vertical bin */
			IOWRITE_U8(buf, 11, cam->vbin);

			/* Exposure */
			IOWRITE_U32(buf, 12, cam->exposure);

			/* Now the exposure flags (16, 17 will be for them) */
			buf[16]  = (cam->frametype & FLI_FRAME_TYPE_DARK) ? 0x01 : 0x00;
			buf[16] |= (cam->frametype & FLI_FRAME_TYPE_FLOOD) ? 0x08 : 0x00;
			buf[16] |= ((cam->exttrigger != 0) && (cam->exttriggerpol == 0)) ? 0x02 : 0x00;
			buf[16] |= ((cam->exttrigger != 0) && (cam->exttriggerpol != 0)) ? 0x04 : 0x00;
			buf[16] |= (cam->extexposurectrl != 0) ? 0x20 : 0x00;

			if ((cam->video_mode == VIDEO_MODE_BEGIN) && SUPPORTS_VIDEO(DEVICE))
			{
				buf[16] |= 0x10; /* Enable video mode */
				cam->video_mode = VIDEO_MODE_ON;
			}

			/* Perform the transation */
			IO(dev, buf, &wlen, &rlen);

			 /* Newer Proline/Microline */
			if (DEVICE->devinfo.fwrev >= 0x0200)
			{
				IOREAD_U16L(buf, 0, cam->top_height)
				IOREAD_U16L(buf, 2, cam->top_offset)
				IOREAD_U16L(buf, 44, cam->bottom_height)
				IOREAD_U16L(buf, 4, cam->bottom_offset)
				IOREAD_U16L(buf, 11, cam->left_width)
				IOREAD_U16L(buf, 13, cam->left_offset)
				IOREAD_U16L(buf, 15, cam->right_width)
				IOREAD_U16L(buf, 17, cam->right_offset)
			}
			else
			{
				cam->top_height = cam->grabrowcount;
				cam->top_offset = 0;
				cam->bottom_height = 0;
				cam->bottom_offset = cam->grabrowcount;
				cam->left_width = cam->grabrowwidth;
				cam->left_offset = 0;
				cam->right_width = 0;
				cam->right_offset =cam->grabrowwidth;
			}

			debug(FLIDEBUG_INFO, "         Grab Height: %d", cam->top_height);
			debug(FLIDEBUG_INFO, "           Top Flush: %d", cam->top_offset);
			debug(FLIDEBUG_INFO, "       Bottom Height: %d", cam->bottom_height);
			debug(FLIDEBUG_INFO, "        Bottom Flush: %d", cam->bottom_offset);
			debug(FLIDEBUG_INFO, "          Left Width: %d", cam->left_width);
			debug(FLIDEBUG_INFO, "         Left Offset: %d", cam->left_offset);
			debug(FLIDEBUG_INFO, "         Right Width: %d", cam->right_width);
			debug(FLIDEBUG_INFO, "        Right Offset: %d", cam->right_offset);

			numpix = (cam->top_height + cam->bottom_height) *
				(cam->left_width + cam->right_width);

			cam->dl_index = 0;
			cam->bytesleft = numpix * sizeof(unsigned short);

			/* Let's reallocate the image buffer if needed, this will
			 * allow us to build the entire image in memory. This is needed
			 * for top/bottom (four quadrant) detectors. */

			if (cam->ibuf_siz < (numpix * sizeof(unsigned short)))
			{
				if (cam->ibuf != NULL)
					xfree(cam->ibuf);

				cam->ibuf = NULL;
				cam->ibuf_siz = numpix * sizeof(unsigned short);

#ifdef __linux__
				/* Linux needs this page aligned, hopefully this is 512 byte aligned too... */
				cam->ibuf_siz = ((cam->ibuf_siz / getpagesize()) + 1) * getpagesize();
				if ((cam->ibuf = xmemalign(getpagesize(), cam->ibuf_siz)) == NULL)
					r = -ENOMEM;
#else
				/* Just 512 byte align it... */
				if ((cam->ibuf = xmalloc(cam->ibuf_siz)) == NULL)
					r = -ENOMEM;
#endif
				if (r != 0)
					cam->ibuf_siz = 0;
			}

			/* Initialize all the buffer pointers */
			cam->ibuf_wr_idx = cam->ibuf;
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return r;
}

long fli_camera_usb_flush_rows(flidev_t dev, long rows, long repeat)
{
  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

  if (rows < 0)
    return -EINVAL;

  if (rows == 0)
    return 0;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			rlen = 0; wlen = 6;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SETFLUSHBINFACTORS);
			IOWRITE_U16(buf, 2, cam->hflushbin);
			IOWRITE_U16(buf, 4, cam->vflushbin);
			IO(dev, buf, &wlen, &rlen);

			while (repeat > 0)
			{
				debug(FLIDEBUG_INFO, "Flushing %d rows.", rows);
				rlen = 0; wlen = 4;
				IOWRITE_U16(buf, 0, FLI_USBCAM_FLUSHROWS);
				IOWRITE_U16(buf, 2, rows);
				IO(dev, buf, &wlen, &rlen);
				repeat--;
			}
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{

		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return r;
}

long fli_camera_usb_set_bit_depth(flidev_t dev, flibitdepth_t bitdepth)
{
//	flicamdata_t *cam = DEVICE->device_data;
//	iobuf_t buf[IOBUF_MAX_SIZ];
//	long rlen = 0, wlen = 0;
	long r = 0;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			r = -EINVAL;
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			r = -EINVAL;
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return r;
}

long fli_camera_usb_read_ioport(flidev_t dev, long *ioportset)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			rlen = 1; wlen = 2;
			IOWRITE_U16(buf, 0, FLI_USBCAM_READIO);
			IO(dev, buf, &wlen, &rlen);
			IOREAD_U8(buf, 0, *ioportset);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			rlen = 2; wlen = 2;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_READ_IOPORT);
			IO(dev, buf, &wlen, &rlen);
			IOREAD_U8(buf, 1, *ioportset);
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_write_ioport(flidev_t dev, long ioportset)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			rlen = 0; wlen = 3;
			IOWRITE_U16(buf, 0, FLI_USBCAM_WRITEIO);
			IOWRITE_U8(buf, 2, ioportset);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			rlen = 2; wlen = 4;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_WRITE_IOPORT);
			IOWRITE_U16(buf, 2, ioportset);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_configure_ioport(flidev_t dev, long ioportset)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			rlen = 0; wlen = 3;
			IOWRITE_U16(buf, 0, FLI_USBCAM_WRITEDIR);
			IOWRITE_U8(buf, 2, ioportset);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			rlen = 2; wlen = 4;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_CONFIGURE_IOPORT);
			IOWRITE_U16(buf, 2, ioportset);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_control_shutter(flidev_t dev, long shutter)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			rlen = 0; wlen = 3;
			IOWRITE_U16(buf, 0, FLI_USBCAM_SHUTTER);
			IOWRITE_U8(buf, 2, shutter);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			unsigned char c = 0;

			switch (shutter)
			{
				case FLI_SHUTTER_CLOSE:
					c = 0x00;
					break;

				case FLI_SHUTTER_OPEN:
					c = 0x01;
					break;

				default:
					r = -EINVAL;
					break;
			}

			if (r != 0)
				break;

			rlen = 2; wlen = 3;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_SET_SHUTTER);
			IOWRITE_U8(buf, 2, c);

			debug(FLIDEBUG_INFO, "%s shutter.", (buf[2] == 0)? "Closing":"Opening");
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return r;
}

long fli_camera_usb_control_bgflush(flidev_t dev, long bgflush)
{
  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	if( (bgflush != FLI_BGFLUSH_STOP) &&
		(bgflush != FLI_BGFLUSH_START) )
	return -EINVAL;

	cam->background_flush = (bgflush == FLI_BGFLUSH_STOP)? 0 : 1;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			if(DEVICE->devinfo.fwrev < 0x0300)
			{
				debug(FLIDEBUG_WARN, "Background flush commanded on early firmware.");
				return -EFAULT;
			}
			rlen = 0; wlen = 4;
			IOWRITE_U16(buf, 0, FLI_USBCAM_BGFLUSH);
			IOWRITE_U16(buf, 2, bgflush);
			IO(dev, buf, &wlen, &rlen);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			if(DEVICE->devinfo.fwrev < 0x0110)
			{
				debug(FLIDEBUG_WARN, "Background flush commanded on early firmware.");
				return -EFAULT;
			}

			rlen = 2; wlen = 4;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_SET_BGFLUSH);
			IOWRITE_U16(buf, 2, bgflush);
			IO(dev, buf, &wlen, &rlen);

		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_get_cooler_power(flidev_t dev, double *power)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	*power = 0.0;
	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			r = -EFAULT;
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			short pwm;

			// Commented this block as it prevents PL9000 form working
			//if (DEVICE->devinfo.fwrev == 0x0100)
			//{
			///	r = -EFAULT;
			//}
			//else
			//{

				rlen = 14; wlen = 2;
				IOWRITE_U16(buf, 0, PROLINE_COMMAND_GET_TEMPERATURE);
				IO(dev, buf, &wlen, &rlen);

				IOREAD_U16(buf, 4, pwm);
				*power = (double) pwm;
			//}
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_get_camera_status(flidev_t dev, long *camera_status)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{

		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			if (DEVICE->devinfo.fwrev == 0x0100)
			{
				*camera_status = FLI_CAMERA_STATUS_UNKNOWN;
			}
			else
			{
				rlen = 4; wlen = 2;
				IOWRITE_U16(buf, 0, PROLINE_COMMAND_GET_STATUS);
				IO(dev, buf, &wlen, &rlen);
				IOREAD_U32(buf, 0, *camera_status);
			}
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_get_camera_mode(flidev_t dev, flimode_t *camera_mode)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			*camera_mode = 0;
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			if (DEVICE->devinfo.fwrev == 0x0100)
			{
				*camera_mode = 0;
			}
			else
			{
				rlen = 2; wlen = 2;
				IOWRITE_U16(buf, 0, PROLINE_COMMAND_GET_CURRENT_MODE);
				IO(dev, buf, &wlen, &rlen);
				IOREAD_U16(buf, 0, *camera_mode);
			}
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_set_camera_mode(flidev_t dev, flimode_t camera_mode)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			if (camera_mode > 0)
				r = -EINVAL;
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			flimode_t mode;

			if (DEVICE->devinfo.fwrev >= 0x0101)
			{
				rlen = 2; wlen = 4;
				IOWRITE_U16(buf, 0, PROLINE_COMMAND_SET_MODE);
				IOWRITE_U16(buf, 2, camera_mode);
				IO(dev, buf, &wlen, &rlen);
				IOREAD_U16(buf, 0, mode);

				if (mode != camera_mode)
				{
					debug(FLIDEBUG_FAIL, "Error setting camera mode, tried %d, performed %d.", camera_mode, mode);
					r = -EINVAL;
				}
			}
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_get_camera_mode_string(flidev_t dev, flimode_t camera_mode, char *dest, size_t siz)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			if (camera_mode > 0)
				r = -EINVAL;
			else
				strncpy((char *) dest, "Default Mode", siz - 1);
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			if (DEVICE->devinfo.fwrev == 0x0100)
			{
				if (camera_mode > 0)
					r = -EINVAL;
				else
					strncpy((char *) dest, "Default Mode", siz - 1);
			}
			else
			{
				rlen = 32; wlen = 4;
				IOWRITE_U16(buf, 0, PROLINE_COMMAND_GET_MODE_STRING);
				IOWRITE_U16(buf, 2, camera_mode);
				IO(dev, buf, &wlen, &rlen);

				strncpy((char *) dest, (char *) buf, MIN(siz - 1, 31));
				if (dest[0] == '\0')
					r = -EINVAL;
			}
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

long fli_camera_usb_end_exposure(flidev_t dev)
{
	long r = 0;
//	flicamdata_t *cam = DEVICE->device_data;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			r = -EINVAL;
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			long rlen = 4, wlen = 4;
			iobuf_t buf[IOBUF_MAX_SIZ];

			if (SUPPORTS_END_EXPOSURE(DEVICE) != 0)
			{
				IOWRITE_U16(buf, 0, PROLINE_COMMAND_UPDATE_EXPOSURE);
				IOWRITE_U16(buf, 2, 0x0001);
				IO(dev, buf, &wlen, &rlen);

				/* The camera returns status at this point, I dunno what we want to do with it
				   so nothing. */

			}
			else
			{
				r = -EINVAL;
			}
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return r;
}

long fli_camera_usb_trigger_exposure(flidev_t dev)
{
	long r = 0;
//	flicamdata_t *cam = DEVICE->device_data;

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			r = -EINVAL;
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			long rlen = 4, wlen = 4;
			iobuf_t buf[IOBUF_MAX_SIZ];

			if (SUPPORTS_SOFTWARE_TRIGGER(DEVICE) != 0)
			{
				IOWRITE_U16(buf, 0, PROLINE_COMMAND_UPDATE_EXPOSURE);
				IOWRITE_U16(buf, 2, 0x0002);
				IO(dev, buf, &wlen, &rlen);

				/* The camera returns status at this point, I dunno what we want to do with it
				   so nothing. */

			}
			else
			{
				r = -EINVAL;
			}
		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

	return r;
}

long fli_camera_usb_set_fan_speed(flidev_t dev, long fan_speed)
{
//  flicamdata_t *cam = DEVICE->device_data;
	iobuf_t buf[IOBUF_MAX_SIZ];
	long rlen, wlen;
	long r = 0;

	memset(buf, 0x00, IOBUF_MAX_SIZ);

	switch (DEVICE->devinfo.devid)
  {
		/* MaxCam and IMG cameras */
		case FLIUSB_CAM_ID:
		{
			r = -EFAULT;
		}
		break;

		/* Proline Camera */
		case FLIUSB_PROLINE_ID:
		{
			if(DEVICE->devinfo.fwrev < 0x0122)
			{
				debug(FLIDEBUG_WARN, "Fan speed control with early firmware.");
				return -EFAULT;
			}

			rlen = 2; wlen = 4;
			IOWRITE_U16(buf, 0, PROLINE_COMMAND_SET_FAN_SPEED);
			IOWRITE_U16(buf, 2, (short) fan_speed);
			IO(dev, buf, &wlen, &rlen);

		}
		break;

		default:
			debug(FLIDEBUG_WARN, "Hmmm, shouldn't be here, operation on NO camera...");
			break;
	}

  return r;
}

