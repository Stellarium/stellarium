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

#include <errno.h>

#include "libfli-libfli.h"
#include "libfli-debug.h"
#include "libfli-sys.h"
#include "libfli-usb.h"

long unix_usbio(flidev_t dev, void *buf, long *wlen, long *rlen)
{
  int err = 0, locked = 0;
  long org_wlen = *wlen, org_rlen = *rlen;

  if ((err = unix_fli_lock(dev)))
  {
    debug(FLIDEBUG_WARN, "Lock failed");
    goto done;
  }

  locked = 1;

  if (*wlen > 0)
  {
    if ((err = unix_bulkwrite(dev, buf, wlen)))
    {
      debug(FLIDEBUG_WARN, "Bulkwrite failed, wrote %d of %d bytes",
	    *wlen, org_wlen);
      goto done;
    }
  }

  if (*rlen > 0)
  {
    if ((err = unix_bulkread(dev, buf, rlen)))
    {
      debug(FLIDEBUG_WARN, "Bulkread failed, read %d of %d bytes",
	    *rlen, org_rlen);
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
