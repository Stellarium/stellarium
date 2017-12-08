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
//#include <winsock.h>
#else
#include <unistd.h>
#include <sys/param.h>
#include <netinet/in.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>

#include "libfli-libfli.h"
#include "libfli-mem.h"
#include "libfli-debug.h"
#include "libfli-filter-focuser.h"

//#define SHOWFUNCTIONS

extern double dconvert(void *buf); /* From libfli-camera-usb.c */

static long fli_homedevice(flidev_t dev, long block);
static long fli_getstepperstatus(flidev_t dev, flistatus_t *status);

/*
	Array of filterwheel info
	Pos = # of filters
	Off = Offset of 0 filter from magnetic stop,
	X - y = number of steps from filter x to filter y
*/
static const wheeldata_t wheeldata[] =
{
/* POS OFF   0-1 1-2 2-3 3-4 4-5 5-6 6-7 7-8 8-9 9-A A-B B-C C-D D-E F-F F-0 */
  { 3, 48, { 80, 80, 80, 80,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0} }, /* Index 0 */
  { 5,  0, { 48, 48, 48, 48, 48,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0} }, /* Index 1 */
  { 7, 14, { 34, 34, 35, 34, 34, 35, 35,  0,  0,  0,  0,  0,  0,  0,  0,  0} }, /* Index 2 */
  { 8, 18, { 30, 30, 30, 30, 30, 30, 30, 30,  0,  0,  0,  0,  0,  0,  0,  0} }, /* Index 3 */
  {10,  0, { 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,  0,  0,  0,  0,  0,  0} }, /* Index 4 */
  {12,  6, { 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,  0,  0,  0,  0} }, /* Index 5 */
  {15,  0, { 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48} }, /* Index 6 */
	{ 7, 14, { 52, 52, 52, 52, 52, 52, 52, 52,  0,  0,  0,  0,  0,  0,  0,  0} }, /* Index 7 */
	{20, 494, { 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29} }, /* Index 7 */
	{12, 35, { 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 0,  0,  0,  0, 0,  0,  0,  0, 0} }, /* Index 7 */
};

#define FLI_BLOCK (1)
#define FLI_NON_BLOCK (0)

static long fli_stepmotor(flidev_t dev, long steps, long block);
static long fli_getsteppos(flidev_t dev, long *pos);
static long fli_setfilterpos(flidev_t dev, long pos);
static long fli_getstepsremaining(flidev_t dev, long *pos);
static long fli_focuser_getfocuserextent(flidev_t dev, long *extent);
static long fli_focuser_readtemperature(flidev_t dev, flichannel_t channel, double *temperature);

long fli_filter_focuser_probe(flidev_t dev)
{
  int err = 0;
  long rlen, wlen;
  unsigned short buf[16];

	CHKDEVICE(dev);

  DEVICE->io_timeout = 200;

  wlen = 2;
  rlen = 2;
  buf[0] = htons(0x8000);
  IO(dev, buf, &wlen, &rlen);
  if (ntohs(buf[0]) != 0x8000)
  {
    debug(FLIDEBUG_WARN, "Invalid echo, no FLI serial device found.");
    err = -ENODEV;
  }

	return err;
}

long fli_filter_focuser_open(flidev_t dev)
{
  int err = 0;
  long rlen, wlen;
  unsigned short buf[16];
  flifilterfocuserdata_t *fdata = NULL;

  CHKDEVICE(dev);

  DEVICE->io_timeout = 200;

  wlen = 2;
  rlen = 2;
  buf[0] = htons(0x8000);
  IO(dev, buf, &wlen, &rlen);
  if (ntohs(buf[0]) != 0x8000)
  {
    debug(FLIDEBUG_WARN, "Invalid echo, device not recognized, got %04x instead of %04x.", ntohs(buf[0]), 0x8000);
    err = -ENODEV;
    goto done;
  }

  wlen = 2;
  rlen = 2;
  buf[0] = htons(0x8001);
  IO(dev, buf, &wlen, &rlen);
  DEVICE->devinfo.fwrev = ntohs(buf[0]);
  if ((DEVICE->devinfo.fwrev & 0xf000) != 0x8000)
  {
    debug(FLIDEBUG_WARN, "Invalid echo, device not recognized.");
    err = -ENODEV;
    goto done;
  }

  if ((DEVICE->device_data = xmalloc(sizeof(flifilterfocuserdata_t))) == NULL)
  {
    err = -ENOMEM;
    goto done;
  }

	fdata = DEVICE->device_data;
  fdata->tableindex = -1;
  fdata->stepspersec = 100;
  fdata->currentslot = -1;

  if (DEVICE->devinfo.fwrev == 0x8001)  /* Old level of firmware */
  {
    if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
    {
			debug(FLIDEBUG_INFO, "Device detected is not filterwheel, old firmware?");
      err = -ENODEV;
      goto done;
    }

    debug(FLIDEBUG_INFO, "Device is old fashioned filter wheel.");
    fdata->tableindex = 1;

    // FIX: should model info be set first?
    return 0;
  }

  debug(FLIDEBUG_INFO, "New version of hardware found.");
	debug(FLIDEBUG_INFO, "Internal FW Rev: 0x%04x", DEVICE->devinfo.fwrev);

	wlen = 2;
  rlen = 2;
  buf[0] = htons(0x8002);
  IO(dev, buf, &wlen, &rlen);
  fdata->hwtype = ntohs(buf[0]);

  if ((fdata->hwtype & 0xff00) != 0x8000)
  {
    err = -ENODEV;
    goto done;
  }

	/* ndev is either jumper settings or FW return code */
	fdata->hwtype &= 0x00ff;
  switch (fdata->hwtype)
  {
		case 0x00:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 1;
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
			break;

		case 0x01:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 0;
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
			break;

		case 0x02:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 2;
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
			break;

		case 0x03:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 3;
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
			break;

		case 0x04:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 6;
			fdata->stepspersec= 16; // 1 /.06
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
			break;

		case 0x05:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 5;
			fdata->stepspersec= 16; //   1/.06
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
	    break;

		case 0x06:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 4;
			fdata->stepspersec= 16; //   1/.06
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
			break;

		case 0x07:
			if (DEVICE->devinfo.type != FLIDEVICE_FOCUSER)
			{
				err = -ENODEV;
				goto done;
			}
			if ((DEVICE->devinfo.fwrev & 0x00ff) < 0x30)
			{
				fdata->extent = 2100;
				fdata->numtempsensors = 0;
			}
			else if ((DEVICE->devinfo.fwrev & 0x00ff) == 0x30)
			{
				fdata->extent = 7000;
				fdata->numtempsensors = 1;
			}
			else
			{
				fdata->extent = 7000;
				fdata->numtempsensors = 2;
			}

			debug(FLIDEBUG_INFO, "Extent: %d Steps/sec: %d Temp Sensors: %d", fdata->extent, fdata->stepspersec, fdata->numtempsensors);
			break;

		case 0x08:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 7;
			fdata->stepspersec= 20;
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
			break;

		case 0x09:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 8;
			fdata->stepspersec= 20;
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
			break;

		case 0x0a:
			if (DEVICE->devinfo.type != FLIDEVICE_FILTERWHEEL)
			{
				err = -ENODEV;
				goto done;
			}
			fdata->tableindex = 9;
			fdata->stepspersec= 20;
			fdata->numslots = wheeldata[fdata->tableindex].n_pos;
			break;

		case 0xff:
			/* This is a newer FLI filter or focuser wheen PCB */
			if (DEVICE->devinfo.type == FLIDEVICE_FILTERWHEEL)
			{
				/* Get the number of filters */
				wlen = 2; rlen = 2;
				buf[0] = htons(0x8008);
				IO(dev, buf, &wlen, &rlen);
				fdata->numslots = ntohs(buf[0]) & 0x00ff;
			}
			else if (DEVICE->devinfo.type == FLIDEVICE_FOCUSER)
			{
				iobuf_t _buf[IOBUF_MAX_SIZ];

				/* Pre-Atlas */
				if ((DEVICE->devinfo.fwrev & 0x00ff) < 0x40)
				{
					wlen = 2; rlen = 2;
					IOWRITE_U16(_buf, 0, 0x8006);
					IO(dev, _buf, &wlen, &rlen);
					IOREAD_U16(_buf, 0, fdata->extent);
				}
				else /* Post-Atlas */
				{
					wlen = 4; rlen = 4;
					IOWRITE_U16(_buf, 0, 0x8006);
					IOWRITE_U16(_buf, 2, 0);
					IO(dev, _buf, &wlen, &rlen);
					IOREAD_U32(_buf, 0, fdata->extent);
				}

				/* Get the number of temperature sensors */
				wlen = 2; rlen = 2;
				buf[0] = htons(0x800a);
				IO(dev, buf, &wlen, &rlen);
				fdata->numtempsensors = ntohs(buf[0]) & 0x00ff;
			}
			else
			{
				err = -ENODEV;
				goto done;
			}

			/* Step rate */
			wlen = 2; rlen = 2;
			buf[0] = htons(0x8009);
			IO(dev, buf, &wlen, &rlen);
			fdata->stepspersec = ntohs(buf[0]) & 0x0fff;

			debug(FLIDEBUG_INFO, "Extent: %d Steps/sec: %d Temp Sensors: %d", fdata->extent, fdata->stepspersec, fdata->numtempsensors);

			fdata->tableindex = (-1);
			break;

		default:
		  debug(FLIDEBUG_FAIL, "Unknown device %d attached.", fdata->hwtype);
			err = -ENODEV;
			goto done;
  }

	/* Now get the model name, either construct it or get it from device. */
	if (err == 0)
	{
		if (fdata->hwtype < 0xfe) /* Older style hardware */
		{
			if (DEVICE->devinfo.type == FLIDEVICE_FILTERWHEEL)
			{
				if ((DEVICE->devinfo.fwrev & 0x00ff) <= 0x30)
				{
					if (xasprintf(&DEVICE->devinfo.model, "Filter Wheel (%ld position)",
							fdata->numslots) < 0)
					{
						debug(FLIDEBUG_WARN, "Could not allocate memory for model information.");
					}
				}

				if ((DEVICE->devinfo.fwrev & 0x00ff) >= 0x31)
				{
					if ((DEVICE->devinfo.model = (char *) xmalloc(33)) == NULL)
					{
						debug(FLIDEBUG_WARN, "Could not allocate memory for model information.");
					}
					else
					{
						memset(DEVICE->devinfo.model, '\0', 33);
						wlen = 2; rlen = 32;
						DEVICE->devinfo.model[0] = 0x80;
						DEVICE->devinfo.model[1] = 0x03;
						IO(dev, DEVICE->devinfo.model, &wlen, &rlen);
					}
				}
			}

			if (DEVICE->devinfo.type == FLIDEVICE_FOCUSER)
			{
				if (xasprintf(&DEVICE->devinfo.model, "FLI Focuser") < 0)
				{
					debug(FLIDEBUG_WARN, "Could not allocate memory for model information.");
				}
			}
		}
		else /* Newer style hardware */
		{
			if ((DEVICE->devinfo.model = (char *) xmalloc(33)) == NULL)
			{
				debug(FLIDEBUG_WARN, "Could not allocate memory for model information.");
			}
			else
			{
				memset(DEVICE->devinfo.model, '\0', 33);
				wlen = 2; rlen = 32;
				DEVICE->devinfo.model[0] = 0x80;
				DEVICE->devinfo.model[1] = 0x03;
				IO(dev, DEVICE->devinfo.model, &wlen, &rlen);
			}
		}
	}

 done:
  if (err)
  {
    if (DEVICE->devinfo.model != NULL)
    {
      xfree(DEVICE->devinfo.model);
      DEVICE->devinfo.model = NULL;
    }

    if (DEVICE->device_data != NULL)
    {
      xfree(DEVICE->device_data);
      DEVICE->device_data = NULL;
    }

    return err;
  }

  debug(FLIDEBUG_INFO, "Found '%s'", DEVICE->devinfo.model);
  return 0;
}

long fli_filter_focuser_close(flidev_t dev)
{
  CHKDEVICE(dev);

  if (DEVICE->devinfo.model != NULL)
  {
    xfree(DEVICE->devinfo.model);
    DEVICE->devinfo.model = NULL;
  }

  if (DEVICE->device_data != NULL)
  {
    xfree(DEVICE->device_data);
    DEVICE->device_data = NULL;
  }

  return 0;
}

long fli_filter_command(flidev_t dev, int cmd, int argc, ...)
{
  flifilterfocuserdata_t *fdata;
  long r;
  va_list ap;

  va_start(ap, argc);
  CHKDEVICE(dev);
  fdata = DEVICE->device_data;

  switch (cmd)
  {
		case FLI_SET_FILTER_POS:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long pos;

				pos = *va_arg(ap, long *);
				r = fli_setfilterpos(dev, pos);
			}
			break;

		case FLI_GET_FILTER_POS:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long *cslot;

				cslot = va_arg(ap, long *);
				*cslot = fdata->currentslot;
				r = 0;
			}
			break;

		case FLI_GET_FILTER_COUNT:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long *nslots;

				nslots = va_arg(ap, long *);
				*nslots = fdata->numslots;
				r = 0;
			}
			break;

		case FLI_STEP_MOTOR:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long *steps;

				steps = va_arg(ap, long *);
				r = fli_stepmotor(dev, *steps, FLI_BLOCK);
			}
			break;

		case FLI_STEP_MOTOR_ASYNC:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long *steps;

				steps = va_arg(ap, long *);
				r = fli_stepmotor(dev, *steps, FLI_NON_BLOCK);
			}
			break;

  case FLI_GET_STEPPER_POS:
    if (argc != 1)
      r = -EINVAL;
    else
    {
      long *pos;

      pos = va_arg(ap, long *);
      r = fli_getsteppos(dev, pos);
    }
    break;

  case FLI_GET_STEPS_REMAINING:
    if (argc != 1)
      r = -EINVAL;
    else
    {
      long *pos;

      pos = va_arg(ap, long *);
      r = fli_getstepsremaining(dev, pos);
    }
    break;

		case FLI_HOME_DEVICE:
		if (argc != 0)
			r = -EINVAL;
		else
			r =  fli_homedevice(dev, FLI_NON_BLOCK);
		break;

	  case FLI_GET_STATUS:
    if (argc != 1)
      r = -EINVAL;
    else
    {
      flistatus_t *status;

      status = va_arg(ap, flistatus_t *);
      r = fli_getstepperstatus(dev, status);
    }
    break;

  default:
    r = -EINVAL;
  }

  va_end(ap);

  return r;
}

long fli_focuser_command(flidev_t dev, int cmd, int argc, ...)
{
  long r;
  va_list ap;

  va_start(ap, argc);
  CHKDEVICE(dev);

  switch (cmd)
  {
  case FLI_STEP_MOTOR:
    if (argc != 1)
      r = -EINVAL;
    else
    {
      long *steps;

      steps = va_arg(ap, long *);
      r = fli_stepmotor(dev, *steps, FLI_BLOCK);
    }
    break;

		case FLI_STEP_MOTOR_ASYNC:
			if (argc != 1)
				r = -EINVAL;
			else
			{
				long *steps;

				steps = va_arg(ap, long *);
				r = fli_stepmotor(dev, *steps, FLI_NON_BLOCK);
			}
			break;

  case FLI_GET_STEPPER_POS:
    if (argc != 1)
      r = -EINVAL;
    else
    {
      long *pos;

      pos = va_arg(ap, long *);
      r = fli_getsteppos(dev, pos);
    }
    break;

  case FLI_GET_STEPS_REMAINING:
    if (argc != 1)
      r = -EINVAL;
    else
    {
      long *pos;

      pos = va_arg(ap, long *);
      r = fli_getstepsremaining(dev, pos);
    }
    break;

  case FLI_GET_FOCUSER_EXTENT:
    if (argc != 1)
      r = -EINVAL;
    else
    {
      long *extent;

      extent = va_arg(ap, long *);
      r = fli_focuser_getfocuserextent(dev, extent);
    }
    break;

  case FLI_HOME_FOCUSER:
    if (argc != 0)
      r = -EINVAL;
    else
      r =  fli_homedevice(dev, FLI_BLOCK);
    break;

		case FLI_HOME_DEVICE:
		if (argc != 0)
			r = -EINVAL;
		else
			r =  fli_homedevice(dev, FLI_NON_BLOCK);
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
      r = fli_focuser_readtemperature(dev, channel, temperature);
    }
    break;

	  case FLI_GET_STATUS:
    if (argc != 1)
      r = -EINVAL;
    else
    {
      flistatus_t *status;

      status = va_arg(ap, flistatus_t *);
      r = fli_getstepperstatus(dev, status);
    }
    break;


  default:
    r = -EINVAL;
  }

  va_end(ap);

  return r;
}

static long fli_stepmotor(flidev_t dev, long steps, long block)
{
  flifilterfocuserdata_t *fdata;
  long dir, timeout, move, stepsleft;
  long rlen, wlen;
  unsigned short buf[16];
  clock_t begin;

  fdata = DEVICE->device_data;

/* Support HALT operation when steps == 0 */
	if (steps == 0)
	{
    rlen = 2;
    wlen = 2;
    buf[0] = htons((unsigned short) 0xa000);
    IO(dev, buf, &wlen, &rlen);
    if ((ntohs(buf[0]) & 0xf000) != 0xa000)
    {
			debug(FLIDEBUG_WARN, "Invalid echo.");
			return -EIO;
    }
		return 0;
	}

  dir = steps;
  steps = abs(steps);
  while (steps > 0)
  {
    if (steps > 4095)
      move = 4095;
    else
      move = steps;

    steps -= move;
    timeout = (move / fdata->stepspersec) + 2;

    rlen = 2;
    wlen = 2;
    if (dir < 0)
    {
      buf[0] = htons((unsigned short) (0xa000 | (unsigned short) move));
      IO(dev, buf, &wlen, &rlen);
      if ((ntohs(buf[0]) & 0xf000) != 0xa000)
      {
				debug(FLIDEBUG_WARN, "Invalid echo.");
				return -EIO;
      }
    }
    else
    {
      buf[0] = htons((unsigned short) (0x9000 | (unsigned short) move));
      IO(dev, buf, &wlen, &rlen);
      if ((ntohs(buf[0]) & 0xf000) != 0x9000)
      {
				debug(FLIDEBUG_WARN, "Invalid echo.");
				return -EIO;
      }
    }

    begin = clock();
    stepsleft = 0;
    while ( (stepsleft != 0x7000) && (block != 0) )
    {
#ifdef _WIN32
			Sleep(100);
#else
			usleep(100000);
#endif
			buf[0] = htons(0x7000);
      IO(dev, buf, &wlen, &rlen);
      stepsleft = ntohs(buf[0]);

      if (((clock() - begin) / CLOCKS_PER_SEC) > timeout)
      {
				debug(FLIDEBUG_WARN, "A device timeout has occurred.");
				return -EIO;
      }
    }
  }

  return 0;
}

static long fli_getsteppos(flidev_t dev, long *pos)
{
  long poslow, poshigh;
  long rlen, wlen;
  unsigned short buf[16];

	/* Pre-Atlas */
	if ( (DEVICE->devinfo.fwrev & 0x00ff) < 0x40)
	{
		rlen = 2; wlen = 2;
		buf[0] = htons(0x6000);
		IO(dev, buf, &wlen, &rlen);
		poslow = ntohs(buf[0]);
		if ((poslow & 0xf000) != 0x6000)
			return -EIO;

		buf[0] = htons(0x6001);
		IO(dev, buf, &wlen, &rlen);
		poshigh = ntohs(buf[0]);
		if ((poshigh & 0xf000) != 0x6000)
			return -EIO;

		if ((poshigh & 0x0080) > 0)
		{
			*pos = ((~poslow) & 0xff) + 1;
			*pos += (256 * ((~poshigh) & 0xff));
			*pos = -(*pos);
		}
		else
		{
			*pos = (poslow & 0xff) + 256 * (poshigh & 0xff);
		}
	}
	else /* Post-Atlas */
	{
		iobuf_t _buf[IOBUF_MAX_SIZ];

		wlen = 4; rlen = 4;
		IOWRITE_U16(_buf, 0, 0x6000);
		IOWRITE_U16(_buf, 2, 0);
		IO(dev, _buf, &wlen, &rlen);
		IOREAD_U32(_buf, 0, *pos);
	}

  return 0;
}

static long fli_getstepsremaining(flidev_t dev, long *pos)
{
  long rlen = 2, wlen = 2;
  unsigned short buf[16];

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering " __FUNCTION__);
#endif

	if ( (DEVICE->devinfo.fwrev & 0x00ff) < 0x40)
	{
		buf[0] = htons(0x7000);
		IO(dev, buf, &wlen, &rlen);
		*pos = ntohs(buf[0]) & 0x0fff;
	}
	else
	{
		iobuf_t _buf[IOBUF_MAX_SIZ];

		wlen = 4; rlen = 4;
		IOWRITE_U16(_buf, 0, 0x7000);
		IOWRITE_U16(_buf, 2, 0);
		IO(dev, _buf, &wlen, &rlen);
		IOREAD_U32(_buf, 0, *pos);
		*pos &= 0x0fffffff;
	}

	return 0;
}

static long fli_homedevice(flidev_t dev, long block)
{
  flifilterfocuserdata_t *fdata;
  long rlen, wlen;
  unsigned short buf[16];

  fdata = DEVICE->device_data;

	/* Older hardware */
	if (fdata->hwtype < 0xfe)
	{
		debug(FLIDEBUG_INFO, "Home filter wheel/focuser.");
		if (DEVICE->devinfo.type == FLIDEVICE_FILTERWHEEL)
		{
			switch (fdata->numslots)
			{
				case 12:
				case 10:
					DEVICE->io_timeout = 120000;
					break;

				case 15:
					DEVICE->io_timeout = 200000;
					break;

				default:
					DEVICE->io_timeout = 5000;
					break;
			}
		}
		else
		{
			DEVICE->io_timeout = 30000;
		}

		wlen = 2;
		rlen = 2;
		buf[0] = htons(0xf000);
		IO(dev, buf, &wlen, &rlen);
		if (ntohs(buf[0]) != 0xf000)
			return -EIO;

		/* Reduce overall timeout to speed operations with serial port */
		DEVICE->io_timeout = 200;

		/* This is required to prevent offsetting the focuser */
		if (DEVICE->devinfo.type != FLIDEVICE_FOCUSER)
		{
			debug(FLIDEBUG_INFO, "Moving %d steps to home position.",
				wheeldata[fdata->tableindex].n_offset);

			COMMAND(fli_stepmotor(dev, - (wheeldata[fdata->tableindex].n_offset), FLI_BLOCK));
			fdata->currentslot = 0;
		}
	}
	else /* New HW */
	{
		clock_t begin;
		unsigned short stepsleft;

		rlen = 2; wlen = 2;
		buf[0] = htons((unsigned short) 0xf000);
		IO(dev, buf, &wlen, &rlen);
		if ((ntohs(buf[0]) & 0xf000) != 0xf000)
		{
			debug(FLIDEBUG_WARN, "Invalid echo.");
			return -EIO;
		}
		begin = clock();
		stepsleft = 0x04;
		while ( ((stepsleft & 0x04) != 0) && (block != 0) )
		{
#ifdef _WIN32
			Sleep(100);
#else
			usleep(100000);
#endif
		buf[0] = htons(0xb000);
    IO(dev, buf, &wlen, &rlen);
    stepsleft = ntohs(buf[0]);
	  }
		fdata->currentslot = 0;
	}

	return 0;
}

static long fli_getstepperstatus(flidev_t dev, flistatus_t *status)
{
  flifilterfocuserdata_t *fdata;
  long rlen, wlen, r = 0;
  unsigned char buf[16];

  fdata = DEVICE->device_data;

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering " __FUNCTION__);
#endif

	/* Older hardware */
	if (fdata->hwtype < 0xfe)
	{
		long pos;
		if ((r = fli_getstepsremaining(dev, &pos)) == 0)
		{
			*status = FLI_FOCUSER_STATUS_LEGACY;
			if (pos != 0)
			{
				/* We can't be sure of the direction */
				*status |= FLI_FOCUSER_STATUS_MOVING_IN | FLI_FOCUSER_STATUS_MOVING_OUT;
			}
		}
		else
		{
			*status = FLI_FOCUSER_STATUS_UNKNOWN;
		}
	}
	else /* New HW */
	{
		wlen = 2; rlen = 2;
		buf[0] = 0xb0;
		buf[1] = 0x00;
    IO(dev, buf, &wlen, &rlen);
    *status = ((long) buf[1]) & 0xff;
	}

	return r;
}

static long fli_setfilterpos(flidev_t dev, long pos)
{
  flifilterfocuserdata_t *fdata;
  long rlen, wlen;
  unsigned short buf[16];
  long move, i, steps;

  fdata = DEVICE->device_data;

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering " __FUNCTION__);
#endif

	if (pos == FLI_FILTERPOSITION_HOME)
	  fdata->currentslot = FLI_FILTERPOSITION_HOME;

  if (fdata->currentslot < 0)
  {
		fli_homedevice(dev, FLI_BLOCK);
	}

  if (pos == FLI_FILTERPOSITION_HOME)
    return 0;

  if (pos >= fdata->numslots)
  {
    debug(FLIDEBUG_WARN, "Requested slot (%d) exceeds number of slots.", pos);
    return -EINVAL;
  }

	if (pos == fdata->currentslot)
		return 0;

	if (fdata->hwtype < 0xfe)
	{
		move = pos - fdata->currentslot;

		if (move < 0)
			move += fdata->numslots;

		steps = 0;
		if (fdata->numslots != 0)
			for (i=0; i < move; i++)
				steps += wheeldata[fdata->tableindex].n_steps[i % fdata->numslots];

		debug(FLIDEBUG_INFO, "Move filter wheel %d steps.", steps);

		if (steps != 0)
			COMMAND(fli_stepmotor(dev, - (steps), FLI_BLOCK));
		fdata->currentslot = pos;
	}
	else
	{
		clock_t begin;
		unsigned short stepsleft;

		rlen = 2;
		wlen = 2;
		buf[0] = htons((unsigned short) (0xc000 | (unsigned short) pos));
		IO(dev, buf, &wlen, &rlen);
		if ((ntohs(buf[0]) & 0xf000) != 0xc000)
		{
		debug(FLIDEBUG_WARN, "Invalid echo.");
		return -EIO;
		}
		begin = clock();
		stepsleft = 0;
		while (stepsleft != 0x7000)
		{
#ifdef _WIN32
			Sleep(100);
#else
			usleep(100000);
#endif
			buf[0] = htons(0x7000);
			IO(dev, buf, &wlen, &rlen);
			stepsleft = ntohs(buf[0]);
		}
		fdata->currentslot = pos;
	}
  return 0;
}

long fli_focuser_getfocuserextent(flidev_t dev, long *extent)
{
  flifilterfocuserdata_t *fdata;
  fdata = DEVICE->device_data;

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering " __FUNCTION__);
#endif

	*extent = fdata->extent;
	return 0;
}

long fli_focuser_readtemperature(flidev_t dev, flichannel_t channel, double *temperature)
{
  flifilterfocuserdata_t *fdata;
	long rlen, wlen;
	short buf[64];
	short b;
	int i;

  fdata = DEVICE->device_data;

#ifdef SHOWFUNCTIONS
	debug(FLIDEBUG_INFO, "Entering " __FUNCTION__);
#endif

	if (fdata->numtempsensors == 0)
	{
		debug(FLIDEBUG_WARN, "This device does not support temperature reading.");
		return -EINVAL;
	}

	if (channel > fdata->numtempsensors)
	{
		debug(FLIDEBUG_WARN, "Device has %d channels, %d channel requested.", fdata->numtempsensors, channel);
			return -EINVAL;
	}

	if (fdata->hwtype == 0xff)
	{
		wlen = 2;
		rlen = 2;
		buf[0] = htons(0x1000 | (unsigned short) channel);
		IO(dev, buf, &wlen, &rlen);

		*temperature = (double) ((signed char) (buf[0] & 0x00ff)) + ((double) ((buf[0] >> 8) & 0x00ff) / 256);

		debug(FLIDEBUG_INFO, "Temperature: %f", *temperature);
	}
	else if (fdata->hwtype == 0x07)
	{
		if ((DEVICE->devinfo.fwrev & 0x00ff) == 0x30)
		{
			wlen = 2;
			rlen = 2;
			buf[0] = htons(0x1000 | (unsigned short) channel);
			IO(dev, buf, &wlen, &rlen);

			b = ntohs(buf[0]);
			*temperature = (double) b / 256.0;

			if (*temperature < -45.0)
			{
				return -EINVAL;
			}
		}

		if ((DEVICE->devinfo.fwrev & 0x00ff) > 0x30)
		{
			/* Ok, some constants are sent back with each temperature reading */
			wlen = 2;
			rlen = 2 + 4 * 7;
			buf[0] = htons(0x1000 | (unsigned short) channel);
			IO(dev, buf, &wlen, &rlen);

			b = ntohs(buf[0]);
			*temperature = 0.0;
			for (i = 0; i < 7; i++)
			{
				*temperature += dconvert(((char *) buf) + (2 + i * 4)) * pow((double) b, (double) i);
			}

			if (*temperature < (-45.0))
			{
				debug(FLIDEBUG_WARN, "External sensor not plugged in.");
				return -EINVAL;
			}
		}
	}

	return 0;
}
