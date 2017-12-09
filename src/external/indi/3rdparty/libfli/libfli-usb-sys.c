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

#include <linux/version.h>
#include <unistd.h>
#include <sys/types.h>

#include "fli-usb.h"

#include <linux/usbdevice_fs.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include <errno.h>

#include "libfli-libfli.h"
#include "libfli-sys.h"
#include "libfli-mem.h"
#include "libfli-usb.h"
#include "fliusb_ioctl.h"

long linux_usb_connect(flidev_t dev, fli_unixio_t *io, char *name)
{
  struct usb_device_descriptor usbdesc;
  fliusb_string_descriptor_t strdesc; 
  int confg, r;

  if (ioctl(io->fd, FLIUSB_GET_DEVICE_DESCRIPTOR, &usbdesc) == -1)
  {
    debug(FLIDEBUG_FAIL, "%s: Could not read descriptor: %s",
	  __PRETTY_FUNCTION__, strerror(errno));
    return -EIO;
  }

  if (usbdesc.idVendor != FLIUSB_VENDORID)
  {
    debug(FLIDEBUG_INFO, "%s: Not a FLI device!", __PRETTY_FUNCTION__);
    return -ENODEV;
  }

  switch (usbdesc.idProduct)
  {
    /* These are valid product IDs */
  case FLIUSB_CAM_ID:
  case FLIUSB_FOCUSER_ID:
  case FLIUSB_FILTER_ID:
  case FLIUSB_PROLINE_ID:
    break;

  default:
    /* Anything else is unknown */
    return -ENODEV;
  }

  DEVICE->devinfo.devid = usbdesc.idProduct;
  DEVICE->devinfo.fwrev = usbdesc.bcdDevice;

  strdesc.index = 3;
  if (ioctl(io->fd, FLIUSB_GET_STRING_DESCRIPTOR, &strdesc) != 0)
  {
    debug(FLIDEBUG_FAIL, "%s: Could not read descriptor: %s",
	  __PRETTY_FUNCTION__, strerror(errno));
  }
  else
  {
    DEVICE->devinfo.serial = xstrndup(strdesc.buf, sizeof(strdesc.buf));
  }
  
  confg = 0;
  r = ioctl (io->fd, USBDEVFS_SETCONFIGURATION, &confg);
  debug(FLIDEBUG_INFO, "USBDEVFS_SETCONFIGURATION return %i", r);
  confg = 1;
  r = ioctl (io->fd, USBDEVFS_SETCONFIGURATION, &confg);
  debug(FLIDEBUG_INFO, "USBDEVFS_SETCONFIGURATION return %i", r);
  return 0;
}

long linux_bulktransfer(flidev_t dev, int ep, void *buf, long *len)
{
  fli_unixio_t *io;
  fliusb_bulktransfer_t bulkxfer;
  size_t remaining;
  int err = 0;

#define _DEBUG

#ifdef _DEBUG
  debug(FLIDEBUG_INFO, "%s: attempting %ld bytes %s",
	__PRETTY_FUNCTION__, *len, (ep & USB_DIR_IN) ? "in" : "out");
#endif

  io = DEVICE->io_data;


#ifdef _DEBUG
  if ((ep & 0xf0) == 0) {
    char buffer[1024];
    int i;

    sprintf(buffer, "OUT %6ld: ", *len);
    for (i = 0; i < ((*len > 16)?16:*len); i++) {
      sprintf(buffer + strlen(buffer), "%02x ", ((unsigned char *) buf)[i]);
    }

    debug(FLIDEBUG_INFO, buffer);
  }

#endif /* _DEBUG */

  remaining = *len;
  while (remaining)  /* read up to USB_READ_SIZ_MAX bytes at a time */
  {
    int bytes;

    bulkxfer.ep = ep;
    bulkxfer.count = MIN(remaining, USB_READ_SIZ_MAX);
    bulkxfer.timeout = DEVICE->io_timeout;
    bulkxfer.buf = buf + *len - remaining;

    /* This ioctl returns the number of bytes transfered */
    bytes = ioctl(io->fd,
		  (ep & USB_DIR_IN) ? FLIUSB_BULKREAD : FLIUSB_BULKWRITE,
		  &bulkxfer);

    if (bytes < 0)
      break;
    remaining -= bytes;
    if (bytes < bulkxfer.count)
      break;
  }

  /* Set *len to the number of bytes actually transfered */
  if (remaining)
    err = -errno;
  *len -= remaining;

#ifdef _DEBUG

  if ((ep & 0xf0) != 0) {
    char buffer[1024];
    int i;

    sprintf(buffer, " IN %6ld: ", *len);
    for (i = 0; i < ((*len > 16)?16:*len); i++) {
      sprintf(buffer + strlen(buffer), "%02x ", ((unsigned char *) buf)[i]);
    }

    debug(FLIDEBUG_INFO, buffer);
  }

#endif /* _DEBUG */

  return err;
}

long linux_bulkwrite(flidev_t dev, void *buf, long *wlen)
{
  int ep;

  switch (DEVICE->devinfo.devid)
  {
  case FLIUSB_CAM_ID:
  case FLIUSB_FOCUSER_ID:
  case FLIUSB_FILTER_ID:
    ep = 0x02;
    break;

  case FLIUSB_PROLINE_ID:
    ep = 0x01;
    break;

  default:
    debug(FLIDEBUG_FAIL, "Unknown device type.");
    return -EINVAL;
  }

  return linux_bulktransfer(dev, ep | USB_DIR_OUT, buf, wlen);
}

long linux_bulkread(flidev_t dev, void *buf, long *rlen)
{
  int ep;

  switch (DEVICE->devinfo.devid)
  {
  case FLIUSB_CAM_ID:
  case FLIUSB_FOCUSER_ID:
  case FLIUSB_FILTER_ID:
    ep = 0x02;
    break;

  case FLIUSB_PROLINE_ID:
    ep = 0x01;
    break;

  default:
    debug(FLIDEBUG_FAIL, "Unknown device type.");
    return -EINVAL;
  }

  return linux_bulktransfer(dev, ep | USB_DIR_IN, buf, rlen);
}

long linux_usb_disconnect(flidev_t dev)
{
  return 0;
}
