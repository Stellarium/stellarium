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

#include <sys/types.h>
#include <sys/time.h>

#include <termios.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "libfli-libfli.h"
#include "libfli-debug.h"
#include "libfli-sys.h"
#include "libfli-serial.h"

long unix_serialio(flidev_t dev, void *buf, long *wlen, long *rlen)
{
  int err = 0, locked = 0, gotattr = 0;
  long org_wlen = *wlen, org_rlen = *rlen;
  struct termios old_termios, new_termios;
  fli_unixio_t *io;

  io = DEVICE->io_data;

  if ((err = unix_fli_lock(dev)))
  {
    debug(FLIDEBUG_WARN, "Lock failed");
    goto done;
  }
  locked = 1;

  if (tcgetattr(io->fd, &old_termios))
  {
    err = -errno;
    debug(FLIDEBUG_WARN, "tcgetattr() failed: %s", strerror(errno));
    goto done;
  }
  gotattr = 1;

  bzero(&new_termios, sizeof(struct termios));
  new_termios.c_cflag = CS8 | CREAD | CLOCAL;
  new_termios.c_cc[VMIN] = 1;
  new_termios.c_cc[VTIME] = 0;

  /* Set the input baud rate */
  if (cfsetispeed(&new_termios, BAUDRATE))
  {
    err = -errno;
    debug(FLIDEBUG_WARN, "cfsetispeed() failed: %s", strerror(errno));
    goto done;
  }
  /* Set the output baud rate */
  if (cfsetospeed(&new_termios, BAUDRATE))
  {
    err = -errno;
    debug(FLIDEBUG_WARN, "cfsetospeed() failed: %s", strerror(errno));
    goto done;
  }

  if (tcsetattr(io->fd, TCSANOW, &new_termios))
  {
    err = -errno;
    // FIX: Should this be FLIDEBUG_FAIL
    debug(FLIDEBUG_WARN, "tcsetattr() failed: %s", strerror(errno));
    goto done;
  }

  if ((*wlen = write(io->fd, buf, org_wlen)) != org_wlen)
  {
    err = -errno;
    debug(FLIDEBUG_WARN, "write() failed, only %d of %d bytes written",
	  *wlen, org_wlen);
    goto done;
  }

  if (tcdrain(io->fd))
  {
    err = -errno;
    debug(FLIDEBUG_WARN, "tcdrain() failed: %s", strerror(errno));
    goto done;
  }

  for (*rlen = 0; *rlen < org_rlen; )
  {
    ssize_t r;
    fd_set readfds;
    struct timeval timeout;

    timeout.tv_sec = DEVICE->io_timeout / 1000;
    timeout.tv_usec = (DEVICE->io_timeout % 1000) * 1000;

    FD_ZERO(&readfds);
    FD_SET(io->fd, &readfds);

    switch (select(io->fd + 1, &readfds, NULL, NULL, &timeout))
    {
    case -1:			/* An error occurred */
      err = -errno;
      debug(FLIDEBUG_WARN, "select() failed: %s", strerror(errno));
      break;

    case 0:			/* A timeout occurred */
      err = -ETIMEDOUT;
      debug(FLIDEBUG_WARN, "A serial communication timeout occurred");
      break;

    default:			/* There's some data to read */
      if ((r = read(io->fd, buf + *rlen, org_rlen - *rlen)) <= 0)
      {
	err = -errno;
	debug(FLIDEBUG_WARN, "read() failed, only %d of %d bytes read",
	      r, org_rlen - *rlen);
      }
      else
	*rlen += r;
      break;
    }

    if (err)
      break;
  }

 done:

  if (gotattr)
  {
    if (tcsetattr(io->fd, TCSANOW, &old_termios))
    {
      if (err == 0)
	err = -errno;
      debug(FLIDEBUG_WARN,
	    "tcsetattr() failed, could not restore terminal settings: %s",
	    strerror(errno));
    }
  }

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
