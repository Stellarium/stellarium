#ifndef _MSC_BUILD
/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

#include <windows.h>
#include <malloc.h>

#include "kdewin32/string.h"
#include "kdewin32/errno.h"

#include "kdewin32/dirent.h"

/**********************************************************************
 * Implement dirent-style opendir/readdir/closedir on Window 95/NT
 *
 * Functions defined are opendir(), readdir() and closedir() with the
 * same prototypes as the normal dirent.h implementation.
 *
 * Does not implement telldir(), seekdir(), rewinddir() or scandir(). 
 * The dirent struct is compatible with Unix, except that d_ino is 
 * always 1 and d_off is made up as we go along.
 *
 * The DIR typedef is not compatible with Unix.
 **********************************************************************/

#ifndef __MINGW32__

DIR * opendir(const char *dir)
{
    DIR *dp;
    char *filespec;
    long handle;
    int index;

    filespec = malloc(strlen(dir) + 2 + 1);
    strcpy(filespec, dir);
    index = strlen(filespec) - 1;
    if (index >= 0 && (filespec[index] == '/' || filespec[index] == '\\'))
        filespec[index] = '\0';
    strcat(filespec, "\\*");

    dp = (DIR *)malloc(sizeof(DIR));
    dp->offset = 0;
    dp->finished = 0;
    dp->dir = strdup(dir);

    if ((handle = _findfirst(filespec, &(dp->fileinfo))) < 0) {
        if (errno == ENOENT)
            dp->finished = 1;
        else
        return NULL;
    }

    dp->handle = handle;
    free(filespec);

    return dp;
}

struct dirent * readdir(DIR *dp)
{
    if (!dp || dp->finished) return NULL;

    if (dp->offset != 0) {
        if (_findnext(dp->handle, &(dp->fileinfo)) < 0) {
            dp->finished = 1;
            /* posix does not set errno in this case */
            errno = 0;
            return NULL;
        }
    }
    dp->offset++;

    strncpy(dp->dent.d_name, dp->fileinfo.name, _MAX_FNAME);
#ifdef KDEWIN32_HAVE_DIRENT_D_TYPE
    dp->dent.d_type = DT_UNKNOWN;
#endif    
    dp->dent.d_ino = 1;
    dp->dent.d_reclen = strlen(dp->dent.d_name);
    dp->dent.d_off = dp->offset;

    return &(dp->dent);
}

int closedir(DIR *dp)
{
    if (!dp) return 0;
    if ((HANDLE)dp->handle != INVALID_HANDLE_VALUE) _findclose(dp->handle);
    if (dp->dir) free(dp->dir);
    if (dp) free(dp);

    return 0;
}

#endif // #ifndef __MINGW32__

#else

/* lt__dirent.c -- internal directory entry scanning interface

   Copyright (C) 2001, 2004 Free Software Foundation, Inc.
   Written by Bob Friesenhahn, 2001

   NOTE: The canonical source of this file is maintained with the
   GNU Libtool package.  Report bugs to bug-libtool@gnu.org.

GNU Libltdl is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

As a special exception to the GNU Lesser General Public License,
if you distribute this file as part of a program or library that
is built using GNU Libtool, you may include this file under the
same distribution terms that you use for the rest of that program.

GNU Libltdl is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with GNU Libltdl; see the file COPYING.LIB.  If not, a
copy can be downloaded from  http://www.gnu.org/licenses/lgpl.html,
or obtained by writing to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

//#include "lt__private.h"

#include <assert.h>
#include <stddef.h>
#include "kdewin32/dirent.h"

#if defined(WIN32)

void  closedir (DIR *entry)
{
  assert (entry != (DIR *) NULL);
  FindClose (entry->hSearch);
  free ((void *) entry);
}

DIR*  opendir (const char *path)
{
  char file_spec[260];
  DIR *entry;

  assert (path != (char *) 0);
  if (strncpy (file_spec, path, sizeof file_spec) >= sizeof file_spec
      || strncat (file_spec, "\\", sizeof file_spec) >= sizeof file_spec)
    return (DIR *) 0;
  entry = (DIR *) malloc (sizeof(DIR));
  if (entry != (DIR *) 0)
    {
      entry->firsttime = TRUE;
      entry->hSearch = FindFirstFileA (file_spec, &entry->Win32FindDataA);

      if (entry->hSearch == INVALID_HANDLE_VALUE)
	{
	  if (strncat (file_spec, "\\*.*", sizeof file_spec) < sizeof file_spec)
	    {
	      entry->hSearch = FindFirstFileA (file_spec, &entry->Win32FindDataA);
	    }

	  if (entry->hSearch == INVALID_HANDLE_VALUE)
	    {
	      entry = (free (entry), (DIR *) 0);
	    }
	}
    }

  return entry;
}

struct dirent*  readdir (DIR *entry)
{
  int status;

  if (entry == (DIR *) 0)
    return (struct dirent *) 0;

  if (!entry->firsttime)
    {
      status = FindNextFileA (entry->hSearch, &entry->Win32FindDataA);
      if (status == 0)
        return (struct dirent *) 0;
    }

  entry->firsttime = FALSE;
  if (strncpy (entry->file_info.d_name, entry->Win32FindDataA.cFileName,
	sizeof entry->file_info.d_name) >= sizeof entry->file_info.d_name)
    return (struct dirent *) 0;
  entry->file_info.d_namlen = strlen (entry->file_info.d_name);

  return &entry->file_info;
}

#endif /*defined(__WINDOWS__)*/

#endif