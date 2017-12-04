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
#include <sys/stat.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <limits.h>
#include <errno.h>

#include "libfli.h"

#define TRYFUNC(f, ...)				\
  do {						\
    if ((r = f(__VA_ARGS__)))			\
      warnc(-r, #f "() failed");		\
  } while (0)

extern const char *__progname;

#define info(format, args...)				\
  printf("%s: " format "\n", __progname, ## args)

#define warnc(c, format, args...)		\
  warnx(format ": %s", ## args, strerror(c))

#define LIBVERSIZ 1024

typedef struct {
  flidomain_t domain;
  char *dname;
  char *name;
} filt_t;

int numfilts = 0;

void findfilts(flidomain_t domain, filt_t **filters);

void usage(char *fmt, ...)
{
  extern const char *__progname;
  va_list ap;

  va_start(ap, fmt);

  printf("\n");
  if (fmt != NULL)
  {
    vprintf(fmt, ap);
    printf("\n\n");
  }
  printf("  Usage: %s <filter position>"
	 "\n\n", __progname);

  va_end(ap);

  exit(0);
}

int main(int argc, char *argv[])
{
  int i = 0, filter = 0;
  long r;
  char libver[LIBVERSIZ];
  filt_t *filt = NULL;
	flidev_t dev;
	long tmp1;

#define BUFF_SIZ (1024)
	char buff[BUFF_SIZ];

  if (argc != 2)
    usage("No filter position given");

	filter = atol(argv[1]);

  TRYFUNC(FLISetDebugLevel, NULL /* "NO HOST" */, FLIDEBUG_ALL);

  TRYFUNC(FLIGetLibVersion, libver, LIBVERSIZ);
  info("Library version '%s'", libver);

  // XXX
  /* Parallel port */
  //findfilts(FLIDOMAIN_PARALLEL_PORT, &filt);
  /* USB */
  findfilts(FLIDOMAIN_USB, &filt);
  /* Serial */
  //findfilts(FLIDOMAIN_SERIAL, &filt);
  /* Inet */
  //findfilts(FLIDOMAIN_INET, &filt);

  if (numfilts == 0)
    info("No filter wheels found.");
  else
  {

  info("Trying filter wheel '%s' from %s domain", filt[i].name, filt[i].dname);

	TRYFUNC(FLIOpen, &dev, filt[i].name, FLIDEVICE_FILTERWHEEL | filt[i].domain);

	TRYFUNC(FLIGetModel, dev, buff, BUFF_SIZ);
	info("Model:        %s", buff);

	TRYFUNC(FLIGetHWRevision, dev, &tmp1);
	info("Hardware Rev: %ld", tmp1);

	TRYFUNC(FLIGetFWRevision, dev, &tmp1);
	info("Firmware Rev: %ld", tmp1);

	TRYFUNC(FLISetFilterPos, dev, filter);

  TRYFUNC(FLIClose, dev);
  }

  for (i = 0; i < numfilts; i++)
    free(filt[i].name);

  free(filt);

  exit(0);
}

void findfilts(flidomain_t domain, filt_t **filt)
{
  long r;
  char **tmplist;

  TRYFUNC(FLIList, domain | FLIDEVICE_FILTERWHEEL, &tmplist);

  if (tmplist != NULL && tmplist[0] != NULL)
  {
    int i, filts = 0;

    for (i = 0; tmplist[i] != NULL; i++)
      filts++;

    if ((*filt = realloc(*filt, (numfilts + filts) * sizeof(filt_t))) == NULL)
      err(1, "realloc() failed");

    for (i = 0; tmplist[i] != NULL; i++)
    {
      int j;
      filt_t *tmpfilt = *filt + i;

      for (j = 0; tmplist[i][j] != '\0'; j++)
	if (tmplist[i][j] == ';')
	{
	  tmplist[i][j] = '\0';
	  break;
	}

      tmpfilt->domain = domain;
      switch (domain)
      {
      case FLIDOMAIN_PARALLEL_PORT:
	tmpfilt->dname = "parallel port";
	break;

      case FLIDOMAIN_USB:
	tmpfilt->dname = "USB";
	break;

      case FLIDOMAIN_SERIAL:
	tmpfilt->dname = "serial";
	break;

      case FLIDOMAIN_INET:
	tmpfilt->dname = "inet";
	break;

      default:
	tmpfilt->dname = "Unknown domain";
	break;
      }
      tmpfilt->name = strdup(tmplist[i]);
    }

    numfilts += filts;
  }

  TRYFUNC(FLIFreeList, tmplist);

  return;
}
