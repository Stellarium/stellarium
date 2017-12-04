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

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include "libfli-libfli.h"
#include "libfli-debug.h"
#include "libfli-mem.h"
#include "libfli-camera.h"
#include "libfli-camera-parport.h"
#include "libfli-camera-usb.h"

const fliccdinfo_t knowndev[] = {
  /* id model           array_area              visible_area */
  {1,  "KAF-0260C0-2",  {{0, 0}, {534,  520}},  {{12, 4},  {524,   516}}, 1.0, 20.0, 20.0},
  {2,  "KAF-0400C0-2",  {{0, 0}, {796,  520}},  {{14, 4},  {782,   516}}, 1.0, 20.0, 20.0},
  {3,  "KAF-1000C0-2",  {{0, 0}, {1042, 1032}}, {{8,  4},  {1032, 1028}}, 1.0, 24.0, 24.0},
  {4,  "KAF-1300C0-2",  {{0, 0}, {1304, 1028}}, {{4,  2},  {1284, 1026}}, 1.0, 20.0, 20.0},
  {5,  "KAF-1400C0-2",  {{0, 0}, {1348, 1037}}, {{14,14},  {782,   526}}, 1.0, 20.0, 20.0},
  {6,  "KAF-1600C0-2",  {{0, 0}, {1564, 1032}}, {{14, 4},  {1550, 1028}}, 1.0, 20.0, 20.0},
  {7,  "KAF-4200C0-2",  {{0, 0}, {2060, 2048}}, {{25, 2},  {2057, 2046}}, 1.0, 20.0, 20.0},
  {8,  "SITe-502S",     {{0, 0}, {527,  512}},  {{15, 0},  {527,   512}}, 1.0, 20.0, 20.0},
  {9,  "TK-1024",       {{0, 0}, {1124, 1024}}, {{50, 0},  {1074, 1024}}, 1.0, 24.0, 24.0},
  {10, "TK-512",        {{0, 0}, {563,  512}},  {{51, 0},  {563,   512}}, 1.0, 24.0, 24.0},
  {11, "SI-003A",       {{0, 0}, {1056, 1024}}, {{16, 0},  {1040, 1024}}, 1.0, 24.0, 24.0},
  {12, "KAF-6300",      {{0, 0}, {3100, 2056}}, {{16, 4},  {3088, 2052}}, 1.0,  9.0,  9.0},
  {13, "KAF-3200",      {{0, 0}, {2267, 1510}}, {{46,34},  {2230, 1506}}, 1.0,  6.8,  6.8},
  {14, "SI424A",        {{0, 0}, {2088, 2049}}, {{20, 0},  {2068, 2049}}, 1.0,  6.8,  6.8},
  {15, "CCD47-10",      {{0, 0}, {1072, 1027}}, {{8,  0},  {1064, 1027}}, 0.0,  0.0,  0.0},
  {16, "CCD77",         {{0, 0}, {527,   512}}, {{15, 0},  {527,   512}}, 0.0,  0.0,  0.0},
  {17, "CCD42-40",      {{0, 0}, {2148, 2048}}, {{50, 0},  {2098, 2048}}, 1.0, 13.5, 13.5},
  {18, "KAF-4300",      {{0, 0}, {2102, 2092}}, {{8,  4},  {2092, 2088}}, 1.0, 24.0, 24.0},
  {19, "KAF-16801",     {{0, 0}, {4145, 4128}}, {{44,29},  {4124, 4109}}, 1.0,  9.0,  9.0},
	{0,  "Unknown Model", {{0, 0}, {0,    0}},    {{0,  0},  {0,    0}}, 0.0, 0.0, 0.0}
};

/* Common camera routines */
static long fli_camera_get_pixel_size(flidev_t dev,
				      double *pixel_x, double *pixel_y);
#define fli_camera_parport_get_pixel_size fli_camera_get_pixel_size
#define fli_camera_usb_get_pixel_size fli_camera_get_pixel_size

static long fli_camera_set_frame_type(flidev_t dev, fliframe_t frametype);
#define fli_camera_parport_set_frame_type fli_camera_set_frame_type
#define fli_camera_usb_set_frame_type fli_camera_set_frame_type

static long fli_camera_set_flushes(flidev_t dev, long nflushes);
#define fli_camera_parport_set_flushes fli_camera_set_flushes
#define fli_camera_usb_set_flushes fli_camera_set_flushes

long fli_camera_open(flidev_t dev)
{
  int r;

  CHKDEVICE(dev);

  if ((DEVICE->device_data = xcalloc(1, sizeof(flicamdata_t))) == NULL)
    return -ENOMEM;

//	load_camera_defaults();

  switch (DEVICE->domain)
  {
		case FLIDOMAIN_PARALLEL_PORT:
			r = fli_camera_parport_open(dev);
			break;

		case FLIDOMAIN_USB:
			r = fli_camera_usb_open(dev);
			break;

		default:
			r = -EINVAL;
  }

  if (r)
  {
    xfree(DEVICE->device_data);
    DEVICE->device_data = NULL;
  }

  return r;
}

long fli_camera_close(flidev_t dev)
{
  flicamdata_t *cam;

  CHKDEVICE(dev);

  cam = DEVICE->device_data;

  if (cam->gbuf != NULL)
  {
    xfree(cam->gbuf);
    cam->gbuf = NULL;
  }

	 if (cam->ibuf != NULL)
  {
    xfree(cam->ibuf);
    cam->ibuf = NULL;
  }


  if (DEVICE->devinfo.model != NULL)
  {
    xfree(DEVICE->devinfo.model);
    DEVICE->devinfo.model = NULL;
  }

  if (DEVICE->devinfo.devnam != NULL)
  {
    xfree(DEVICE->devinfo.devnam);
    DEVICE->devinfo.devnam = NULL;
  }

  if (DEVICE->device_data != NULL)
  {
    xfree(DEVICE->device_data);
    DEVICE->device_data = NULL;
  }

  return 0;
}

long fli_camera_command(flidev_t dev, int cmd, int argc, ...)
{
  long r = 0;
  va_list ap;

  va_start(ap, argc);
  CHKDEVICE(dev);

  switch (cmd)
  {
		case FLI_GET_PIXEL_SIZE:
			if (argc != 2)
				r = -EINVAL;
			else
			{
				double *pixel_x, *pixel_y;

				pixel_x = va_arg(ap, double *);
				pixel_y = va_arg(ap, double *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_get_pixel_size(dev, pixel_x, pixel_y);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_get_pixel_size(dev, pixel_x, pixel_y);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_GET_ARRAY_AREA:
			if (argc != 4)
				r = -EINVAL;
			else
			{
				long *ul_x, *ul_y, *lr_x, *lr_y;

				ul_x = va_arg(ap, long *);
				ul_y = va_arg(ap, long *);
				lr_x = va_arg(ap, long *);
				lr_y = va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_get_array_area(dev, ul_x, ul_y, lr_x, lr_y);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_get_array_area(dev, ul_x, ul_y, lr_x, lr_y);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_GET_VISIBLE_AREA:
			if (argc != 4)
				r = -EINVAL;
			else
			{
				long *ul_x, *ul_y, *lr_x, *lr_y;

				ul_x = va_arg(ap, long *);
				ul_y = va_arg(ap, long *);
				lr_x = va_arg(ap, long *);
				lr_y = va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_get_visible_area(dev, ul_x, ul_y, lr_x, lr_y);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_get_visible_area(dev, ul_x, ul_y, lr_x, lr_y);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_EXPOSURE_TIME:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				unsigned long exptime;

				exptime = *va_arg(ap, unsigned long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_set_exposure_time(dev, exptime);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_exposure_time(dev, exptime);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_IMAGE_AREA:
			if (argc != 4)
				r = -EINVAL;
			else
			{
				long ul_x, ul_y, lr_x, lr_y;

				ul_x = *va_arg(ap, long *);
				ul_y = *va_arg(ap, long *);
				lr_x = *va_arg(ap, long *);
				lr_y = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_set_image_area(dev, ul_x, ul_y, lr_x, lr_y);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_image_area(dev, ul_x, ul_y, lr_x, lr_y);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_HBIN:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long hbin;

				hbin = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_set_hbin(dev, hbin);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_hbin(dev, hbin);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_VBIN:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long vbin;

				vbin = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_set_vbin(dev, vbin);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_vbin(dev, vbin);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_FRAME_TYPE:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long frametype;

				frametype = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_set_frame_type(dev, frametype);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_frame_type(dev, frametype);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_CANCEL_EXPOSURE:
			if (argc != 0)
				r = -EINVAL;
			else
			{
				flicamdata_t *cam;

				cam = DEVICE->device_data;

				cam->grabrowcount = 1;
				cam->grabrowcounttot = cam->grabrowcount;
				cam->grabrowindex = 0;
				cam->grabrowbatchsize = 1;
				cam->grabrowbufferindex = cam->grabrowcount;
				cam->flushcountbeforefirstrow = 0;
				cam->flushcountafterlastrow = 0;
			
				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = DEVICE->fli_command(dev, FLI_CONTROL_SHUTTER, (long) FLI_SHUTTER_CLOSE);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_cancel_exposure(dev);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_GET_EXPOSURE_STATUS:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long *timeleft;

				timeleft = va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_get_exposure_status(dev, timeleft);
					break;
					case FLIDOMAIN_USB:
						r = fli_camera_usb_get_exposure_status(dev, timeleft);
					break;
					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_FAN_SPEED:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long fan_speed;

				fan_speed = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_fan_speed(dev, fan_speed);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_TEMPERATURE:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				double temperature;

				temperature = *va_arg(ap, double *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_set_temperature(dev, temperature);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_temperature(dev, temperature);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_READ_TEMPERATURE:
			if (argc != 2)
				r = -EINVAL;
			else
			{
				double *temperature;
				flichannel_t channel;

				channel = va_arg(ap, flichannel_t);
				temperature = va_arg(ap, double *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_get_temperature(dev, temperature);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_read_temperature(dev, channel, temperature);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_GET_TEMPERATURE:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				double *temperature;

				temperature = va_arg(ap, double *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_get_temperature(dev, temperature);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_get_temperature(dev, temperature);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_TDI:
			if (argc != 2)
				r = -EINVAL;
			else
			{
				flitdirate_t rate;
				flitdiflags_t flags;

				rate = *va_arg(ap, flitdirate_t *);
				flags = *va_arg(ap, flitdiflags_t *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_tdi(dev, rate, flags);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_GRAB_ROW:
			if (argc != 2)
				r = -EINVAL;
			else
			{
				void *buf;
				size_t width;

				buf = va_arg(ap, void *);
				width = *va_arg(ap, size_t *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_grab_row(dev, buf, width);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_grab_row(dev, buf, width);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_EXPOSE_FRAME:
			if (argc != 0)
				r = -EINVAL;
			else
			{
				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_expose_frame(dev);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_expose_frame(dev);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_START_VIDEO_MODE:
			if (argc != 0)
				r = -EINVAL;
			else
			{
				switch (DEVICE->domain)
				{
					case FLIDOMAIN_USB:
						r = fli_camera_usb_start_video_mode(dev);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_STOP_VIDEO_MODE:
			if (argc != 0)
				r = -EINVAL;
			else
			{
				switch (DEVICE->domain)
				{
					case FLIDOMAIN_USB:
						r = fli_camera_usb_stop_video_mode(dev);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_GRAB_VIDEO_FRAME:
			if (argc != 2)
				r = -EINVAL;
			else
			{
				void *buf;
				size_t size;

				buf = va_arg(ap, void *);
				size = *va_arg(ap, size_t *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_USB:
						r = fli_camera_usb_grab_video_frame(dev, buf, size);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;


		case FLI_FLUSH_ROWS:
			if (argc != 2)
				r = -EINVAL;
			else
			{
				long rows, repeat;

				rows = *va_arg(ap, long *);
				repeat = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_flush_rows(dev, rows, repeat);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_flush_rows(dev, rows, repeat);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_FLUSHES:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long nflushes;

				nflushes = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_set_flushes(dev, nflushes);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_flushes(dev, nflushes);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_SET_BIT_DEPTH:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				flibitdepth_t bitdepth;

				bitdepth = *va_arg(ap, flibitdepth_t *);

				switch (DEVICE->domain)
				{
				case FLIDOMAIN_PARALLEL_PORT:
					r = fli_camera_parport_set_bit_depth(dev, bitdepth);
					break;

				case FLIDOMAIN_USB:
					r = fli_camera_usb_set_bit_depth(dev, bitdepth);
					break;

				default:
					r = -EINVAL;
				}
			}
			break;

		case FLI_READ_IOPORT:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long *ioportset;

				ioportset = va_arg(ap, long *);

				switch (DEVICE->domain)
				{
				case FLIDOMAIN_PARALLEL_PORT:
					r = fli_camera_parport_read_ioport(dev, ioportset);
					break;

				case FLIDOMAIN_USB:
					r = fli_camera_usb_read_ioport(dev, ioportset);
					break;

				default:
					r = -EINVAL;
				}
			}
			break;

		case FLI_WRITE_IOPORT:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long ioportset;

				ioportset = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
				case FLIDOMAIN_PARALLEL_PORT:
					r = fli_camera_parport_write_ioport(dev, ioportset);
					break;

				case FLIDOMAIN_USB:
					r = fli_camera_usb_write_ioport(dev, ioportset);
					break;

				default:
					r = -EINVAL;
				}
			}
			break;

		case FLI_CONFIGURE_IOPORT:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long ioportset;

				ioportset = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
				case FLIDOMAIN_PARALLEL_PORT:
					r = fli_camera_parport_configure_ioport(dev, ioportset);
					break;

				case FLIDOMAIN_USB:
					r = fli_camera_usb_configure_ioport(dev, ioportset);
					break;

				default:
					r = -EINVAL;
				}
			}
			break;

		case FLI_CONTROL_SHUTTER:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long shutter;
				flicamdata_t *cam;

				cam = DEVICE->device_data;

				shutter = *va_arg(ap, long *);

				if( (shutter & FLI_SHUTTER_EXTERNAL_TRIGGER_LOW) ||
					  (shutter & FLI_SHUTTER_EXTERNAL_TRIGGER_HIGH) ||
						((shutter & FLI_SHUTTER_EXTERNAL_EXPOSURE_CONTROL)) )
				{
					debug(FLIDEBUG_INFO, "External trigger: %02x", shutter);
					cam->exttrigger = 1;
					cam->exttriggerpol = (shutter & FLI_SHUTTER_EXTERNAL_TRIGGER_LOW)?0:1;
					cam->extexposurectrl = (shutter & FLI_SHUTTER_EXTERNAL_EXPOSURE_CONTROL)?1:0; 
					r = 0;
				}
				else
				{
					debug(FLIDEBUG_INFO, "No External trigger.\n");
					cam->exttrigger = 0;
					cam->extexposurectrl = 0;

					switch (DEVICE->domain)
					{
					case FLIDOMAIN_PARALLEL_PORT:
						r = fli_camera_parport_control_shutter(dev, shutter);
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_control_shutter(dev, shutter);
						break;

					default:
						r = -EINVAL;
					}
				}
			}
			break;

		case FLI_CONTROL_BGFLUSH:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long bgflush;
				flicamdata_t *cam;

				cam = DEVICE->device_data;

				bgflush = *va_arg(ap, long *);

				switch (DEVICE->domain)
				{
				case FLIDOMAIN_PARALLEL_PORT:
					r = -EFAULT;
					break;

				case FLIDOMAIN_USB:
					r = fli_camera_usb_control_bgflush(dev, bgflush);
					break;

				default:
					r = -EINVAL;
				}
			}
			break;

		case FLI_GET_COOLER_POWER:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				double *cooler_power;

				cooler_power = va_arg(ap, double *);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = -EINVAL;
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_get_cooler_power(dev, cooler_power);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_GET_STATUS:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long *camera_status;

				camera_status = va_arg(ap, long *);

				*camera_status = 0xffffffff;
				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = 0;
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_get_camera_status(dev, camera_status);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

			case FLI_GET_CAMERA_MODE:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				flimode_t *camera_mode;

				camera_mode = va_arg(ap, flimode_t *);

				*camera_mode = 0;
				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = 0;
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_get_camera_mode(dev, camera_mode);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

			case FLI_SET_CAMERA_MODE:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				flimode_t camera_mode;

				camera_mode = va_arg(ap, flimode_t);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = 0;
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_set_camera_mode(dev, camera_mode);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

			case FLI_GET_CAMERA_MODE_STRING:
			if (argc != 3)
				r = -EINVAL;
			else
			{
				flimode_t camera_mode;
				char *buf;
				size_t len;

				camera_mode = va_arg(ap, flimode_t);
				buf = va_arg(ap, char *);
				len = va_arg(ap, size_t);

				memset(buf, '\0', len);

				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						if (camera_mode == 0)
						{
							r = 0;
							strncpy(buf, "Default Mode", len - 1);
						}
						else
						{
							r = -EINVAL;
						}
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_get_camera_mode_string(dev, camera_mode, buf, len);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_TRIGGER_EXPOSURE:
			if (argc != 0)
				r = -EINVAL;
			else
			{
				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = -EINVAL;
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_trigger_exposure(dev);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		case FLI_END_EXPOSURE:
			if (argc != 0)
				r = -EINVAL;
			else
			{
				switch (DEVICE->domain)
				{
					case FLIDOMAIN_PARALLEL_PORT:
						r = -EINVAL;
						break;

					case FLIDOMAIN_USB:
						r = fli_camera_usb_end_exposure(dev);
						break;

					default:
						r = -EINVAL;
				}
			}
			break;

		default:
			r = -EINVAL;
  }

  va_end(ap);

  return r;
}

static long fli_camera_get_pixel_size(flidev_t dev,
				      double *pixel_x, double *pixel_y)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;
  *pixel_x = (double)cam->ccd.pixelwidth;
  *pixel_y = (double)cam->ccd.pixelheight;

  return 0;
}

static long fli_camera_set_frame_type(flidev_t dev, fliframe_t frametype)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;

  if (frametype & 0xfff8)
    return -EINVAL;

  cam->frametype = frametype;

  return 0;
}

static long fli_camera_set_flushes(flidev_t dev, long nflushes)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;

  if((nflushes < 0) || (nflushes > 16))
    return -EINVAL;

  cam->flushes = nflushes;

  return 0;
}
