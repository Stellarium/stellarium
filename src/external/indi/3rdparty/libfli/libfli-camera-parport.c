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
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "libfli-libfli.h"
#include "libfli-mem.h"
#include "libfli-debug.h"
#include "libfli-camera.h"
#include "libfli-camera-parport.h"

long fli_camera_parport_open(flidev_t dev)
{
  flicamdata_t *cam;
  long rlen, wlen;
  unsigned short buf;
  int id;

  cam = DEVICE->device_data;

	cam->removebias = 1;
	cam->biasoffset = 200;


  /* Set timeout values */
  cam->readto = 1000;
  cam->writeto = 1000;
  cam->dirto = 1000;

  rlen = 2;
  wlen = 2;
  buf = htons(C_ADDRESS(1, EPARAM_ECHO));
  IO(dev, &buf, &wlen, &rlen);
  if (buf != htons(C_ADDRESS(1, EPARAM_ECHO)))
  {
    debug(FLIDEBUG_FAIL, "Echo back from camera failed.");
    return -EIO;
  }

  rlen = 2; wlen = 2;
  buf = htons(C_ADDRESS(1, EPARAM_DEVICE));
  IO(dev, &buf, &wlen, &rlen);
  DEVICE->devinfo.hwrev = ntohs(buf) & 0x00ff;

  rlen = 2; wlen = 2;
  buf = htons(C_ADDRESS(1, EPARAM_CCDID));
  IO(dev, &buf, &wlen, &rlen);
  DEVICE->devinfo.devid = ntohs(buf) & 0x00ff;

  for (id = 0; knowndev[id].index != 0; id++)
    if (knowndev[id].index == DEVICE->devinfo.devid)
      break;

  if (knowndev[id].index == 0)
    return -ENODEV;

  cam->ccd.array_area.ul.x = knowndev[id].array_area.ul.x;
  cam->ccd.array_area.ul.y = knowndev[id].array_area.ul.y;
  cam->ccd.array_area.lr.x = knowndev[id].array_area.lr.x;
  cam->ccd.array_area.lr.y = knowndev[id].array_area.lr.y;
  cam->ccd.visible_area.ul.x = knowndev[id].visible_area.ul.x;
  cam->ccd.visible_area.ul.y = knowndev[id].visible_area.ul.y;
  cam->ccd.visible_area.lr.x = knowndev[id].visible_area.lr.x;
  cam->ccd.visible_area.lr.y = knowndev[id].visible_area.lr.y;
  cam->ccd.pixelwidth = knowndev[id].pixelwidth;
  cam->ccd.pixelheight = knowndev[id].pixelheight;

  if ((DEVICE->devinfo.model =
       (char *)xmalloc(strlen(knowndev[id].model) + 1)) == NULL)
    return -ENOMEM;
  strcpy(DEVICE->devinfo.model, knowndev[id].model);

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

  rlen = 2; wlen = 2;
  buf = htons(C_ADDRESS(1, EPARAM_SNHIGH));
  IO(dev, &buf, &wlen, &rlen);
  DEVICE->devinfo.serno = (ntohs(buf) & 0x00ff) << 8;

  rlen = 2; wlen = 2;
  buf = htons(C_ADDRESS(1, EPARAM_SNLOW));
  IO(dev, &buf, &wlen, &rlen);
  DEVICE->devinfo.serno |= (ntohs(buf) & 0x00ff);

  rlen = 2; wlen = 2;
  buf = htons(C_ADDRESS(1, EPARAM_FIRM));
  IO(dev, &buf, &wlen, &rlen);
  DEVICE->devinfo.fwrev = (ntohs(buf) & 0x00ff);

  /* Initialize all varaibles to something */
  switch(DEVICE->devinfo.hwrev)
  {
  case 0x01:
    cam->tempslope = (100.0 / 201.1);
    cam->tempintercept = (-61.613);
    break;

  case 0x02:
    cam->tempslope = (70.0 / 215.75);
    cam->tempintercept = (-52.5681);
    break;

  default:
    debug(FLIDEBUG_WARN, "Could not set temperature parameters.");
    break;
  }

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

  cam->grabrowwidth =
    (cam->image_area.lr.x - cam->image_area.ul.x) / cam->hbin;
  cam->grabrowcount = 1;
  cam->grabrowcounttot = cam->grabrowcount;
  cam->grabrowindex = 0;
  cam->grabrowbatchsize = 1;
  cam->grabrowbufferindex = cam->grabrowcount;
  cam->flushcountbeforefirstrow = 0;
  cam->flushcountafterlastrow = 0;

  return 0;
}

long fli_camera_parport_get_array_area(flidev_t dev, long *ul_x, long *ul_y,
				       long *lr_x, long *lr_y)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;

  *ul_x = cam->ccd.array_area.ul.x;
  *ul_y = cam->ccd.array_area.ul.y;
  *lr_x = cam->ccd.array_area.lr.x;
  *lr_y = cam->ccd.array_area.lr.y;

  return 0;
}

long fli_camera_parport_get_visible_area(flidev_t dev, long *ul_x, long *ul_y,
					 long *lr_x, long *lr_y)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;

  *ul_x = cam->ccd.visible_area.ul.x;
  *ul_y = cam->ccd.visible_area.ul.y;
  *lr_x = cam->ccd.visible_area.lr.x;
  *lr_y = cam->ccd.visible_area.lr.y;

  return 0;
}

long fli_camera_parport_set_exposure_time(flidev_t dev, long exptime)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;

  if (exptime < 0)
    return -EINVAL;

  cam->exposure = exptime;

  if (exptime <= 15000) /* Less than thirty seconds..., 8.192e-3 sec */
  {
    cam->expdur = 1;
    cam->expmul = (long) (((double) exptime) / 8.192);
  }
  else if (exptime <= 2000000) /* Less than one hour */
  {
    cam->expdur = (long) (1.0 / 8.192e-3);
    cam->expmul = (long) (exptime / 1000);
  }
  else
  {
    cam->expdur = (long) (10.0 / 8.192e-3);
    cam->expmul = (long) (exptime / 10000);
  }

  return 0;
}

long fli_camera_parport_set_image_area(flidev_t dev, long ul_x, long ul_y,
				       long lr_x, long lr_y)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;

  if ((ul_x < cam->ccd.visible_area.ul.x) ||
      (ul_y < cam->ccd.visible_area.ul.y) ||
      (lr_x > cam->ccd.visible_area.lr.x) ||
      (lr_y > cam->ccd.visible_area.lr.y))
    return -EINVAL;

  cam->image_area.ul.x = ul_x;
  cam->image_area.ul.y = ul_y;
  cam->image_area.lr.x = lr_x;
  cam->image_area.lr.y = lr_y;

  return 0;
}

long fli_camera_parport_set_hbin(flidev_t dev, long hbin)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;

  if ((hbin < 1) || (hbin > 16))
    return -EINVAL;

  cam->hbin = hbin;
  return 0;
}

long fli_camera_parport_set_vbin(flidev_t dev, long vbin)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;

  if ((vbin < 1) || (vbin > 16))
    return -EINVAL;

  cam->vbin = vbin;
  return 0;
}

long fli_camera_parport_get_exposure_status(flidev_t dev, long *timeleft)
{
  flicamdata_t *cam;
  long rlen, wlen;
  unsigned short buf;

  cam = DEVICE->device_data;

  rlen = 2; wlen = 2;
  buf = htons(C_SHUTTER(1,0));
  IO(dev, &buf, &wlen, &rlen);
  if ((ntohs(buf) & 0xf000) != C_SHUTTER(0,0))
  {
    debug(FLIDEBUG_FAIL, "(exposurestatus) echo back from camera failed.");
    return -EIO;
  }

  *timeleft = (long)((double)(ntohs(buf) & 0x07ff) *
		     ((double)cam->expdur * 8.192));

  return 0;
}

long fli_camera_parport_set_temperature(flidev_t dev, double temperature)
{
  flicamdata_t *cam;
  long rlen, wlen;
  unsigned short buf;

  cam = DEVICE->device_data;

  rlen = 2; wlen = 2;
  buf = (unsigned short)((temperature - cam->tempintercept) /
			 cam->tempslope);
  buf = htons((unsigned short) C_TEMP(buf));
  IO(dev, &buf, &wlen, &rlen);
  if ((ntohs(buf) & 0xf000) != C_TEMP(0))
  {
    debug(FLIDEBUG_FAIL, "(settemperature) echo back from camera failed.");
    return -EIO;
  }

  return 0;
}

long fli_camera_parport_get_temperature(flidev_t dev, double *temperature)
{
  flicamdata_t *cam;
  long rlen, wlen;
  unsigned short buf;

  cam = DEVICE->device_data;

  rlen = 2; wlen = 2;
  buf = htons(C_TEMP(0x0800));
  IO(dev, &buf, &wlen, &rlen);
  if ((ntohs(buf) & 0xf000) != C_TEMP(0))
  {
    debug(FLIDEBUG_FAIL, "(settemperature) echo back from camera failed.");
    return -EIO;
  }
  *temperature = cam->tempslope * (double)(ntohs(buf) & 0x00ff) +
    cam->tempintercept;

  return 0;
}

long fli_camera_parport_grab_row(flidev_t dev, void *buff, size_t width)
{
  flicamdata_t *cam;
  long r;
  double dTm;
  long rlen, wlen;
  unsigned short buf;
	long grabwidth;

  cam = DEVICE->device_data;

  if (cam->flushcountbeforefirstrow > 0)
  {
    if ((r = fli_camera_parport_flush_rows(dev,
					   cam->flushcountbeforefirstrow, 1)))
      return r;

    cam->flushcountbeforefirstrow = 0;
  }

  dTm = (25.0e-6) * cam->ccd.array_area.lr.x + 1e-3;
  dTm = dTm / 1e-6;
  cam->readto = (long)dTm;
  cam->writeto = (long)dTm;

	if (cam->removebias)
	{
//		grabwidth = cam->ccd.array_area.lr.x - cam->ccd.array_area.ul.x + (50 + 64) - cam->image_area.ul.x;
		grabwidth = (cam->ccd.array_area.lr.x - cam->ccd.array_area.ul.x + (5 + 64) - cam->image_area.ul.x) / cam->hbin;
	}
	else
	{
		grabwidth = cam->grabrowwidth;
	}

  rlen = 0; wlen = 2;
  buf = htons((unsigned short) C_SEND(grabwidth));
  IO(dev, &buf, &wlen, &rlen);

  if (cam->bitdepth == FLI_MODE_8BIT)
  {
    unsigned char *cbuf;
    int x;

    if ((cbuf = xmalloc(grabwidth)) == NULL)
    {
      debug(FLIDEBUG_FAIL, "Failed memory allocation during row grab.");
      return -ENOMEM;
    }

    rlen = grabwidth; wlen = 0;
    r = DEVICE->fli_io(dev, cbuf, &wlen, &rlen);
    if (r != 0)
    {
      debug(FLIDEBUG_WARN,
	    "Couldn't grab entire row (8-bit), got %d of %d bytes.",
	    r, grabwidth);
    }
    for (x = 0; x < (int)width; x++)
    {
      ((char *)buff)[x] = (((cbuf[x]) + 128) & 0x00ff);
    }
    xfree(cbuf);
  }
  else
  {
    unsigned short *sbuf;
    int x;

    if ((sbuf = xmalloc(grabwidth * sizeof(unsigned short))) == NULL)
    {
      debug(FLIDEBUG_FAIL, "Failed memory allocation during row grab.");
      return -ENOMEM;
    }

    rlen = grabwidth * sizeof(unsigned short); wlen = 0;
    r = DEVICE->fli_io(dev, sbuf, &wlen, &rlen);
    if (r != 0)
    {
      debug(FLIDEBUG_WARN,
	    "Couldn't grab entire row (16-bit), got %d of %d bytes.",
	    r, grabwidth);
    }
    for (x = 0; x < (int)width; x++)
    {
      if (DEVICE->devinfo.hwrev == 0x01) /* IMG camera */
      {
				((unsigned short *)buff)[x] = ntohs(sbuf[x]) + 32768;
      }
      else
      {
				((unsigned short *)buff)[x] = ntohs(sbuf[x]);
      }
    }

		if (cam->removebias)
		{
			for (x = grabwidth - (64 / cam->hbin); x < grabwidth; x++)
			{
				unsigned short d;

				if (DEVICE->devinfo.hwrev == 0x01) /* IMG camera */
					d = ntohs(sbuf[x]) + 32768;
				else
					d = ntohs(sbuf[x]);

				cam->pix_sum += (double) d;
				cam->pix_cnt += 1.0;
			}

			for (x = 0; x < (int)width; x++)
			{
				((unsigned short *)buff)[x] = ((unsigned short *)buff)[x] - (unsigned short) ((cam->pix_sum / cam->pix_cnt) - cam->biasoffset);
			}
			debug(FLIDEBUG_INFO, "Overscan bias average: %g (%d)", (cam->pix_sum / cam->pix_cnt), (unsigned short) ((cam->pix_sum / cam->pix_cnt) - 200.0));
		}

    xfree(sbuf);
  }

  rlen = 2; wlen = 0;
  IO(dev, &buf, &wlen, &rlen);

	if (cam->removebias)
	{
		if (ntohs(buf) != C_SEND(grabwidth))
		{
			debug(FLIDEBUG_WARN, "Width: %d, requested %d.",
			width, grabwidth * sizeof(unsigned short));
			debug(FLIDEBUG_WARN, "Got 0x%04x instead of 0x%04x.", ntohs(buf), C_SEND(grabwidth));
			debug(FLIDEBUG_WARN, "Didn't get command echo at end of row.");
		}
	}
	else
	{
  if (ntohs(buf) != C_SEND(width))
  {
    debug(FLIDEBUG_WARN, "Width: %d, requested %d.",
			width, grabwidth * sizeof(unsigned short));
    debug(FLIDEBUG_WARN, "Got 0x%04x instead of 0x%04x.", ntohs(buf), C_SEND(width));
    debug(FLIDEBUG_WARN, "Didn't get command echo at end of row.");
  }
  }

  if (cam->grabrowcount > 0)
  {
    cam->grabrowcount--;
    if (cam->grabrowcount == 0)
    {
      if ((r = fli_camera_parport_flush_rows(dev,
					     cam->flushcountafterlastrow, 1)))
			return r;

      cam->flushcountafterlastrow = 0;
      cam->grabrowbatchsize = 1;
    }
  }

  cam->readto = 1000;
  cam->writeto = 1000;

  return 0;
}

long fli_camera_parport_expose_frame(flidev_t dev)
{
  flicamdata_t *cam;
  long rlen, wlen;
  unsigned short buf;

  cam = DEVICE->device_data;

  debug(FLIDEBUG_INFO, "Setting X Row Offset.");
  rlen = 2; wlen = 2;
  buf = htons((unsigned short) D_XROWOFF(cam->image_area.ul.x));
  IO(dev, &buf, &wlen, &rlen);


	if (cam->removebias)
	{
		debug(FLIDEBUG_INFO, "Setting X Row Width to %d.", cam->ccd.array_area.lr.x - cam->ccd.array_area.ul.x + 5 + 64);
		buf = htons((unsigned short) D_XROWWID(cam->ccd.array_area.lr.x - cam->ccd.array_area.ul.x + 5 + 64));
		IO(dev, &buf, &wlen, &rlen);
	}
	else
	{
  debug(FLIDEBUG_INFO, "Setting X Row Width to %d.", cam->ccd.array_area.lr.x - cam->ccd.array_area.ul.x);
  buf = htons((unsigned short) D_XROWWID(cam->ccd.array_area.lr.x - cam->ccd.array_area.ul.x));
  IO(dev, &buf, &wlen, &rlen);
	}

  debug(FLIDEBUG_INFO, "Setting X Flush Bin.");
  buf = htons((unsigned short) D_XFLBIN(cam->hflushbin));
  IO(dev, &buf, &wlen, &rlen);

  debug(FLIDEBUG_INFO, "Setting Y Flush Bin.");
  buf = htons((unsigned short) D_YFLBIN(cam->vflushbin));
  IO(dev, &buf, &wlen, &rlen);

  debug(FLIDEBUG_INFO, "Setting X Bin.");
  buf = htons((unsigned short) D_XBIN(cam->hbin));
  IO(dev, &buf, &wlen, &rlen);

  debug(FLIDEBUG_INFO, "Setting Y Bin.");
  buf = htons((unsigned short) D_YBIN(cam->vbin));
  IO(dev, &buf, &wlen, &rlen);

  debug(FLIDEBUG_INFO, "Setting Exposure Duration.");
  buf = htons((unsigned short) D_EXPDUR(cam->expdur));
  IO(dev, &buf, &wlen, &rlen);

  if (cam->bitdepth == FLI_MODE_8BIT)
  {
    debug(FLIDEBUG_INFO, "Eight Bit.");
    buf = htons((unsigned short)((cam->exttrigger > 0) ?
				 C_RESTCFG(0,0,1,7) : C_RESTCFG(0,0,0,7)));
  }
  else
  {
    debug(FLIDEBUG_INFO, "Sixteen Bit.");
    buf = htons((unsigned short)((cam->exttrigger > 0) ?
				 C_RESTCFG(0,0,1,15) :
				 C_RESTCFG(0,0,0,15)));
  }
  IO(dev, &buf, &wlen, &rlen);

  if (cam->flushes > 0)
  {
    int r;

    debug(FLIDEBUG_INFO, "Flushing array.");
    if ((r = fli_camera_parport_flush_rows(dev,
					   cam->ccd.array_area.lr.y - cam->ccd.array_area.ul.y,
					   cam->flushes)))
      return r;
  }

  debug(FLIDEBUG_INFO, "Exposing.");
  buf = htons((unsigned short) C_SHUTTER((cam->frametype & FLI_FRAME_TYPE_DARK)?0:1,
			cam->expmul));
  IO(dev, &buf, &wlen, &rlen);

  cam->grabrowwidth = cam->image_area.lr.x - cam->image_area.ul.x;
  cam->flushcountbeforefirstrow = cam->image_area.ul.y;
  cam->flushcountafterlastrow =
    (cam->ccd.array_area.lr.y - cam->ccd.array_area.ul.y) -
    ((cam->image_area.lr.y - cam->image_area.ul.y) * cam->vbin) -
    cam->image_area.ul.y;

  if (cam->flushcountafterlastrow < 0)
    cam->flushcountafterlastrow = 0;

	cam->pix_sum = 0.0;
	cam->pix_cnt = 0.0;

  cam->grabrowcount = cam->image_area.lr.y - cam->image_area.ul.y;

  return 0;
}

long fli_camera_parport_flush_rows(flidev_t dev, long rows, long repeat)
{
  flicamdata_t *cam;
  double dTm;
  long rlen, wlen;
  unsigned short buf;

  if (rows < 0)
    return -EINVAL;

  if (rows == 0)
    return 0;

  cam = DEVICE->device_data;

  dTm = ((25e-6) / (cam->hflushbin / 2)) * cam->ccd.array_area.lr.x + 1e-3;
  dTm = dTm * rows;
  dTm = dTm / 1e-6;
  cam->readto = (long)dTm;
  cam->writeto = (long)dTm;

  while (repeat>0)
  {
    long retval;

    rlen = 2; wlen = 2;
    buf = htons((unsigned short) C_FLUSH(rows));
    retval = DEVICE->fli_io(dev, &buf, &wlen, &rlen);
    if (retval != 0)
    {
      cam->readto = 1000;
      cam->writeto = 1000;
      return retval;
    }
    repeat--;
  }

  return 0;
}

long fli_camera_parport_set_bit_depth(flidev_t dev, flibitdepth_t bitdepth)
{
  flicamdata_t *cam;

  cam = DEVICE->device_data;

  if (DEVICE->devinfo.type != 0x01) /* IMG cameras only support this */
    return -EINVAL;

  if ((bitdepth != FLI_MODE_8BIT) && (bitdepth != FLI_MODE_16BIT))
  {
    debug(FLIDEBUG_FAIL, "Invalid bit depth setting.");
    return -EINVAL;
  }

  cam->bitdepth = bitdepth;

  return 0;
}

static void correctioportdatawrite(flidev_t dev, unsigned short *Data)
{
  unsigned short data;

  data = 0;

  switch(DEVICE->devinfo.hwrev)
  {
  case 0x01:
    data |= (*Data & FLICCD_IO_P0)?0x01:0;
    data |= (*Data & FLICCD_IO_P1)?0x02:0;
    data |= (*Data & FLICCD_IO_P2)?0x04:0;
    data |= (*Data & FLICCD_IO_P3)?0x80:0;
    break;

  case 0x02:
    data |= (*Data & FLICCD_IO_P0)?0x08:0;
    data |= (*Data & FLICCD_IO_P1)?0x10:0;
    data |= (*Data & FLICCD_IO_P2)?0x20:0;
    data |= (*Data & FLICCD_IO_P3)?0x40:0;
    break;

  default:
    break;
  }

  *Data = data;

  return;
}

static void correctioportdataread(flidev_t dev, unsigned short *Data)
{
  unsigned short data;

  data = 0;

  switch (DEVICE->devinfo.hwrev)
  {
  case 0x01:
    data |= (*Data & 0x01)?FLICCD_IO_P0:0;
    data |= (*Data & 0x02)?FLICCD_IO_P1:0;
    data |= (*Data & 0x04)?FLICCD_IO_P2:0;
    data |= (*Data & 0x80)?FLICCD_IO_P3:0;
    break;

  case 0x02:
    data |= (*Data & 0x08)?FLICCD_IO_P0:0;
    data |= (*Data & 0x10)?FLICCD_IO_P1:0;
    data |= (*Data & 0x20)?FLICCD_IO_P2:0;
    data |= (*Data & 0x40)?FLICCD_IO_P3:0;
    break;

  default:
    break;
  }

  *Data = data;

  return;
}

long fli_camera_parport_read_ioport(flidev_t dev, long *ioportset)
{
  long rlen, wlen;
  unsigned short buf;

  rlen = 2; wlen = 2;
  buf = htons(0x7900);
  IO(dev, &buf, &wlen, &rlen);

  *ioportset = ntohs(buf) & 0x00ff;
  correctioportdataread(dev, (unsigned short *) ioportset);

  return 0;
}

long fli_camera_parport_write_ioport(flidev_t dev, long ioportset)
{
  long rlen, wlen;
  unsigned short buf;

  correctioportdatawrite(dev, (unsigned short *) &ioportset);
  buf = htons((unsigned short) (0x7100 | (ioportset & 0x00ff)));

  rlen = 2; wlen = 2;
  IO(dev, &buf, &wlen, &rlen);

  return 0;
}

long fli_camera_parport_configure_ioport(flidev_t dev, long ioportset)
{
  long rlen, wlen;
  unsigned short buf;

  correctioportdatawrite(dev, (unsigned short *) &ioportset);
  buf = htons((unsigned short) (0x7000 | (ioportset & 0x00ff)));

  rlen = 2; wlen = 2;
  IO(dev, &buf, &wlen, &rlen);

  return 0;
}

long fli_camera_parport_control_shutter(flidev_t dev, long shutter)
{
  long rlen, wlen;
  unsigned short buf;

  rlen = 2; wlen = 2;
  buf = htons(D_EXPDUR(0));
  IO(dev, &buf, &wlen, &rlen);

  switch (shutter)
  {
  case FLI_SHUTTER_CLOSE:
    debug(FLIDEBUG_INFO, "Closing shutter.");
    buf = htons(C_SHUTTER(0, 0));
    IO(dev, &buf, &wlen, &rlen);
    break;

  case FLI_SHUTTER_OPEN:
    buf = htons(C_SHUTTER(1, 1));
    IO(dev, &buf, &wlen, &rlen);
    break;

  default:
    return -EINVAL;
  }

  return 0;
}
