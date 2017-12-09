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

#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/ioctl.h>

#include "libfli-libfli.h"
#include "libfli-debug.h"
#include "libfli-mem.h"
#include "libfli-camera.h"
#include "libfli-filter-focuser.h"
#include "libfli-sys.h"
#include "libfli-parport.h"
#include "libfli-usb.h"
#include "libfli-serial.h"
#include "fliusb_ioctl.h"
#include "fli-usb.h"

static long unix_fli_list_parport(flidomain_t domain, char ***names);
static long unix_fli_list_usb(flidomain_t domain, char ***names);
static long unix_fli_list_serial(flidomain_t domain, char ***names);

long unix_fli_connect(flidev_t dev, char *name, long domain)
{
  fli_unixio_t *io;
  struct usb_device_descriptor usbdesc;

  CHKDEVICE(dev);

  if (name == NULL)
    return -EINVAL;

  /* Lock functions should be set before any other functions used */
  DEVICE->fli_lock = unix_fli_lock;
  DEVICE->fli_unlock = unix_fli_unlock;

  DEVICE->domain = domain & 0x00ff;
  DEVICE->devinfo.type = domain & 0xff00;

  debug(FLIDEBUG_INFO, "Domain: 0x%04x", DEVICE->domain);
  debug(FLIDEBUG_INFO, "  Type: 0x%04x", DEVICE->devinfo.type);

  if ((io = xcalloc(1, sizeof(fli_unixio_t))) == NULL)
    return -ENOMEM;

  if ((io->fd = open(name, O_RDWR)) == -1)
  {
    xfree(io);
    return -errno;
  }

  switch (DEVICE->domain)
  {
  case FLIDOMAIN_PARALLEL_PORT:
    DEVICE->fli_io = unix_parportio;
    break;

  case FLIDOMAIN_USB:
    {
      int r;

      if ((r = unix_usb_connect(dev, io, name)))
      {
	close(io->fd);
	xfree(io);
	return r;
      }

      if (ioctl(io->fd, FLIUSB_GET_DEVICE_DESCRIPTOR, &usbdesc) == -1)
      {
        debug(FLIDEBUG_FAIL, "%s: Could not read descriptor: %s",
              __PRETTY_FUNCTION__, strerror(errno));
        return -EIO;
      }

      // try to open only device with correct idProduct
      switch (DEVICE->devinfo.type)
      {
        case FLIDEVICE_CAMERA:
	  if (!(usbdesc.idProduct == 0x0002 || usbdesc.idProduct == 0x000a))
	    return -ENODEV;
          break;
        case FLIDEVICE_FOCUSER:
	  if (usbdesc.idProduct != 0x0006)
	    return -ENODEV;
	  break;
        case FLIDEVICE_FILTERWHEEL:
          if (usbdesc.idProduct != 0x0007)
            return -ENODEV;
          break;
      }

      DEVICE->fli_io = unix_usbio;
    }
    break;

  case FLIDOMAIN_SERIAL:
    DEVICE->fli_io = unix_serialio;
    break;

  default:
    close(io->fd);
    xfree(io);
    return -EINVAL;
  }

  switch (DEVICE->devinfo.type)
  {
  case FLIDEVICE_CAMERA:
    DEVICE->fli_open = fli_camera_open;
    DEVICE->fli_close = fli_camera_close;
    DEVICE->fli_command = fli_camera_command;
    break;

  case FLIDEVICE_FOCUSER:
    DEVICE->fli_open = fli_focuser_open;
    DEVICE->fli_close = fli_focuser_close;
    DEVICE->fli_command = fli_focuser_command;
    break;

  case FLIDEVICE_FILTERWHEEL:
    DEVICE->fli_open = fli_filter_open;
    DEVICE->fli_close = fli_filter_close;
    DEVICE->fli_command = fli_filter_command;
    break;

  default:
    close(io->fd);
    xfree(io);
    return -EINVAL;
  }

  DEVICE->io_data = io;
  DEVICE->name = xstrdup(name);
  DEVICE->io_timeout = 60 * 1000; /* 1 min. */

  return 0;
}

long unix_fli_disconnect(flidev_t dev)
{
  int err = 0;
  fli_unixio_t *io;

  CHKDEVICE(dev);

  if ((io = DEVICE->io_data) == NULL)
    return -EINVAL;

  switch (DEVICE->domain)
  {
  case FLIDOMAIN_USB:
    err = unix_usb_disconnect(dev);
    break;

  default:
    break;
  }

  if (close(io->fd))
    if (!err)
      err = -errno;

  xfree(DEVICE->io_data);

  DEVICE->io_data = NULL;
  DEVICE->fli_lock = NULL;
  DEVICE->fli_unlock = NULL;
  DEVICE->fli_io = NULL;
  DEVICE->fli_open = NULL;
  DEVICE->fli_close = NULL;
  DEVICE->fli_command = NULL;

  return err;
}

#if defined(_USE_FLOCK_)

long unix_fli_lock(flidev_t dev)
{
  fli_unixio_t *io = DEVICE->io_data;

  if (io == NULL)
    return -ENODEV;

  if (flock(io->fd, LOCK_EX) == -1)
    return -errno;
  else
    return 0;
}

long unix_fli_unlock(flidev_t dev)
{
  fli_unixio_t *io = DEVICE->io_data;

  if (io == NULL)
    return -ENODEV;

  if (flock(io->fd, LOCK_UN) == -1)
    return -errno;
  else
    return 0;
}

#else /* !defined(_USE_FLOCK_) */

#define PUBLIC_DIR "/var/spool/lock"

long unix_fli_lock(flidev_t dev)
{
  int fd, err = 0, locked = 0, i;
  char tmpf[] = PUBLIC_DIR "/temp.XXXXXX", lockf[PATH_MAX], name[PATH_MAX];
  FILE *f;
  unsigned int backoff = 10000;
  pid_t pid;

  if ((fd = mkstemp(tmpf)) == -1)
    return -errno;

  if ((f = fdopen(fd, "w")) == NULL)
  {
    err = -errno;
    goto done;
  }

  fprintf(f, "%d\n", getpid());
  fclose(f);

  if (chmod(tmpf, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
	    S_IROTH | S_IWOTH) == -1)
  {
    err = -errno;
    goto done;
  }

  for (i = 0; DEVICE->name[i] != '\0' && i < PATH_MAX; i++)
    name[i] = (DEVICE->name[i] == '/') ? '-' : DEVICE->name[i];

  name[MIN(i, PATH_MAX - 1)] = '\0';

  if (snprintf(lockf, PATH_MAX, PUBLIC_DIR "/libfli%s.lock",
	       name) >= PATH_MAX)
  {
    err = -EOVERFLOW;
    goto done;
  }

  do {
    if (link(tmpf, lockf) == -1)
    {
      int r;

      if (errno != EEXIST)
      {
	err = -errno;
	goto done;
      }

      if ((f = fopen(lockf, "r")) == NULL)
	continue;

      r = fscanf(f, "%d\n", &pid);
      fclose(f);
      if (r != 1)
	continue;

      if (kill(pid, 0))
      {
	if (errno == ESRCH)
	{
	  debug(FLIDEBUG_WARN, "Removing stale lock file");
	  unlink(lockf);
	}

	continue;
      }
      else
      {
	usleep(backoff);
	if ((backoff <<= 2) == 0)
	{
	  err = -ETIMEDOUT;
	  goto done;
	}
      }
    }
    else
      locked = 1;
  } while (!locked);

 done:

  unlink(tmpf);

  return err;
}

long unix_fli_unlock(flidev_t dev)
{
  char lockf[PATH_MAX], name[PATH_MAX];
  FILE *f;
  pid_t pid = -1;
  int i;

  for (i = 0; DEVICE->name[i] != '\0' && i < PATH_MAX; i++)
    name[i] = (DEVICE->name[i] == '/') ? '-' : DEVICE->name[i];

  name[MIN(i, PATH_MAX - 1)] = '\0';

  if (snprintf(lockf, PATH_MAX, PUBLIC_DIR "/libfli%s.lock",
	       name) >= PATH_MAX)
    return -EOVERFLOW;

  if ((f = fopen(lockf, "r")) == NULL)
  {
    debug(FLIDEBUG_WARN, "Trying to unlock `%s' when not locked",
	  DEVICE->name);
    return -errno;
  }

  if (fscanf(f, "%d\n", &pid) != 1)
    debug(FLIDEBUG_WARN, "Invalid lock file for `%s'", DEVICE->name);

  fclose(f);

  if (pid != -1 && pid != getpid())
    debug(FLIDEBUG_WARN, "Forcing unlock of `%s' from process %d",
	  DEVICE->name, pid);

  unlink(lockf);

  return 0;
}

#undef PUBLIC_DIR

#endif /* defined(_USE_FLOCK_) */

long unix_fli_list(flidomain_t domain, char ***names)
{
  *names = NULL;

  switch (domain & 0x00ff)
  {
  case FLIDOMAIN_PARALLEL_PORT:
    return unix_fli_list_parport(domain, names);
    break;

  case FLIDOMAIN_USB:
    return unix_fli_list_usb(domain, names);
    break;

  case FLIDOMAIN_SERIAL:
    return unix_fli_list_serial(domain, names);
    break;

  default:
    return -EINVAL;
  }

  /* Not reached */
  return -EINVAL;
}

static long unix_fli_list_glob(char *pattern, flidomain_t domain,
			       char ***names)
{
  int retval, i, found = 0;
  char **list;
  glob_t g;

  if ((retval = glob(pattern, 0, NULL, &g)))
  {
#ifdef GLOB_NOMATCH
    if (retval != GLOB_NOMATCH)
    {
      globfree(&g);
      return -errno;
    }

    /* retval == GLOB_NOMATCH */
    g.gl_pathc = 0;
#else
    globfree(&g);
    return -errno;
#endif
  }

  if ((list = xmalloc((g.gl_pathc + 1) * sizeof(char *))) == NULL)
  {
    globfree(&g);
    return -ENOMEM;
  }

  for (i = 0; i < g.gl_pathc; i++)
  {
    flidev_t dev;

    if (FLIOpen(&dev, g.gl_pathv[i], domain))
      continue;

    if ((list[found] = xmalloc(strlen(g.gl_pathv[i]) +
			       (DEVICE->devinfo.model ? strlen(DEVICE->devinfo.model) : 6) +
			       2)) == NULL)
    {
      int j;

      FLIClose(dev);
      for (j = 0; j < found; j++)
	xfree(list[j]);
      xfree(list);
      globfree(&g);
      return -ENOMEM;
    }

    sprintf(list[found], "%s;%s", g.gl_pathv[i], DEVICE->devinfo.model);
    FLIClose(dev);
    found++;
  }

  globfree(&g);

  /* Terminate the list */
  list[found++] = NULL;

  list = xrealloc(list, found * sizeof(char *));
  *names = list;

  return 0;
}

#ifdef __linux__

static long unix_fli_list_parport(flidomain_t domain, char ***names)
{
  return unix_fli_list_glob(PARPORT_GLOB, domain, names);
}

#else

static long unix_fli_list_parport(flidomain_t domain, char ***names)
{
  return -EINVAL;
}

#endif

static long unix_fli_list_usb(flidomain_t domain, char ***names)
{
  return unix_fli_list_glob(USB_GLOB, domain, names);
}

static long unix_fli_list_serial(flidomain_t domain, char ***names)
{
  return unix_fli_list_glob(SERIAL_GLOB, domain, names);
}
