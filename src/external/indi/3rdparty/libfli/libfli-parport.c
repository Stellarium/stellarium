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

#include <sys/ioctl.h>
#include <asm/param.h>

#include <errno.h>
#include <unistd.h>

#include "libfli-libfli.h"
#include "libfli-debug.h"
#include "libfli-camera.h"

#include "fli_ioctl.h"

long unix_parportio_linux(flidev_t dev, void *buf, long *wlen, long *rlen)
{
  fli_unixio_t *io;
  flicamdata_t *cam;
  int err = 0, locked = 0;
  long org_wlen = *wlen, org_rlen = *rlen;
  int wto, rto, dto;

  io = DEVICE->io_data;
  cam = DEVICE->device_data;

  if ((err = unix_fli_lock(dev)))
  {
    debug(FLIDEBUG_WARN, "Lock failed");
    goto done;
  }

  locked = 1;

  /* Convert timeout to jiffies */
  wto = cam->writeto / 1000 * HZ;
  rto = cam->readto / 1000 * HZ;
  dto = cam->dirto / 1000 * HZ;

  if (ioctl(io->fd, FLI_SET_WTO, &wto))
  {
    err = -errno;
    goto done;
  }

  if (ioctl(io->fd, FLI_SET_DTO, &dto))
  {
    err = -errno;
    goto done;
  }

  if (ioctl(io->fd, FLI_SET_RTO, &rto))
  {
    err = -errno;
    goto done;
  }

  if (*wlen > 0)
  {
    if ((*wlen = write(io->fd, buf, *wlen)) != org_wlen)
    {
      debug(FLIDEBUG_WARN, "write failed, only %d of %d bytes written",
	    *wlen, org_wlen);
      err = -errno;
      goto done;
    }
  }

  if (*rlen > 0)
  {
    if ((*rlen = read(io->fd, buf, *rlen)) != org_rlen)
    {
      debug(FLIDEBUG_WARN, "read failed, only %d of %d bytes read",
	    *rlen, org_rlen);
      err = -errno;
      goto done;
    }
  }

 done:

  if (locked)
  {
    int r;

    if ((r = unix_fli_unlock(dev)))
      debug(FLIDEBUG_WARN, "Unlock failed");
    if (err == 0)
      err = r;
  }

  return err;
}
