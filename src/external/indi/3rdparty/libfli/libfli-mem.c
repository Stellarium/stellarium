/*

  Copyright (c) 2000, 2002 Finger Lakes Instrumentation (FLI), L.L.C.
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

#ifdef __linux__
#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#endif /* __linux__ */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "libfli-libfli.h"
#include "libfli-mem.h"

#define DEFAULT_NUM_POINTERS (1024)

static struct _mem_ptrs {
  void **pointers;
  int total;
  int used;
} allocated = {NULL, 0, 0};

static void *saveptr(void *ptr)
{
  int i, err = 0;

  if (allocated.used + 1 > allocated.total)
  {
    void **tmp;
    int newtotal;

    if (allocated.total == 0)
      newtotal = DEFAULT_NUM_POINTERS;
    else
      newtotal = 2 * allocated.total;

    if ((tmp = realloc(allocated.pointers,
		       newtotal * sizeof(void **))) == NULL)
    {
      err = 1;
      goto done;
    }

    allocated.pointers = tmp;
    memset(allocated.pointers + allocated.total, 0,
	   (newtotal - allocated.total) * sizeof(void **));
    allocated.total = newtotal;
  }

  for (i = 0; i < allocated.total; i++)
    if (allocated.pointers[i] == NULL)
      break;

  if (i == allocated.total)
  {
    /* This shouldn't happen */
    debug(FLIDEBUG_WARN, "Internal memory allocation error");
    err = 1;
    goto done;
  }

  allocated.pointers[i] = ptr;
  allocated.used++;

 done:

  if (err)
  {
    free(ptr);
    return NULL;
  }

  return ptr;
}

static void **findptr(void *ptr)
{
  int i;

  for (i = 0; i < allocated.total; i++)
    if (allocated.pointers[i] == ptr)
      return &allocated.pointers[i];

  debug(FLIDEBUG_WARN, "Invalid pointer not found: %p", ptr);

  return NULL;
}

static int deleteptr(void *ptr)
{
  void **allocatedptr;

  if ((allocatedptr = findptr(ptr)) == NULL)
    return -1;

  *allocatedptr = NULL;
  allocated.used--;

  return 0;
}

void *xmalloc(size_t size)
{
  void *ptr;

  if ((ptr = malloc(size)) == NULL)
    return NULL;

  return saveptr(ptr);
}

void *xcalloc(size_t nmemb, size_t size)
{
  void *ptr;

  if ((ptr = calloc(nmemb, size)) == NULL)
    return NULL;

  return saveptr(ptr);
}

#ifdef __linux__

void *xmemalign(size_t alignment, size_t size)
{
  int err;
  void *ptr;

  if ((err = posix_memalign(&ptr, alignment, size)))
  {
    debug(FLIDEBUG_WARN, "posix_memalign() failed: %d", err);
    return NULL;
  }

  return saveptr(ptr);
}

#endif /* __linux__ */

void xfree(void *ptr)
{
  if (deleteptr(ptr))
    return;

  free(ptr);

  return;
}

void *xrealloc(void *ptr, size_t size)
{
  void **allocatedptr, *tmp;

  if ((allocatedptr = findptr(ptr)) == NULL)
    return NULL;

  if ((tmp = realloc(ptr, size)) == NULL)
    return NULL;

  *allocatedptr = tmp;

  return tmp;
}

int xfree_all(void)
{
  int i;
  int freed = 0;

  for (i = 0; i < allocated.total; i++)
  {
    if (allocated.pointers[i] != NULL)
    {
      free(allocated.pointers[i]);
      allocated.pointers[i] = NULL;
      allocated.used--;
      freed++;
    }
  }

  if (allocated.used != 0)
    debug(FLIDEBUG_WARN, "Internal memory handling error");

  if (allocated.pointers != NULL)
    free(allocated.pointers);

  allocated.pointers = NULL;
  allocated.used = 0;
  allocated.total = 0;

  return freed;
}

char *xstrdup(const char *s)
{
  char *tmp;

#ifdef _WIN32
#define strdup _strdup
#endif

  if ((tmp = strdup(s)) == NULL)
    return NULL;

  return saveptr(tmp);
}

char *xstrndup(const char *s, size_t siz)
{
  char *tmp;
	size_t len;

  if (strlen(s) > siz)
		len = siz;
	else
		len = strlen(s);

	tmp = (char *) xmalloc(len + 1);

	if (tmp == NULL)
		return NULL;

	strncpy(tmp, s, len);
	tmp[len] = '\0';

	return tmp;
}

#ifdef __linux__

int xasprintf(char **strp, const char *fmt, ...)
{
  va_list ap;
  char *tmp;
  int err;

  va_start(ap, fmt);

  if ((err = vasprintf(&tmp, fmt, ap)) < 0)
    goto done;

  if (saveptr(tmp) == NULL)
    err = -1;

	done:

  va_end(ap);

  return err;
}

#else

int xasprintf(char **strp, const char *fmt, ...)
{
  va_list ap;
  char *p;
  int n, size = 100;

  /* Guess we need no more than 100 bytes. */
  if ((p = (char *) xmalloc (size)) == NULL)
    return (-1);
  while (1)
  {
    /* Try to print in the allocated space. */
    va_start(ap, fmt);
#ifdef _WIN32
    n = _vsnprintf (p, size, fmt, ap);
#else
    n = vsnprintf (p, size, fmt, ap);
#endif
    va_end(ap);
    /* If that worked, send the string. */
    if (n > -1 && n < size)
    {
      *strp = p;
      return n;
    }

    /* Else try again with more space. */
    if (n > -1)    /* glibc 2.1 */
      size = n + 1; /* precisely what is needed */
    else           /* glibc 2.0 */
      size *= 2;  /* twice the old size */
    if ((p = (char *) xrealloc (p, size)) == NULL)
      return (-1);
  }
}

#endif
