#ifndef _MSC_BUILD
/* This file is part of the KDE project
   Copyright (C) 2006 Peter Kuemmel <syntheticpp@gmx.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDEWIN_DIRENT_H
#define KDEWIN_DIRENT_H

// include everywhere
#include "kdewin32/sys/types.h"

#include <../include/dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

// Implement readdir_r(). For now do not provide dummy function to avoid setting HAVE_READDIR_R.
// Code like DirectoryListThread::run() in kio/kio/kurlcompletion.cpp uses readdir() when !HAVE_READDIR_R.

// struct dirent* readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);

#ifdef __cplusplus
}
#endif

#endif  // KDEWIN_DIRENT_H

#else

/* lt__dirent.h -- internal directory entry scanning interface

   Copyright (C) 2001, 2004, 2006 Free Software Foundation, Inc.
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
copy can be downloaded from http://www.gnu.org/licenses/lgpl.html,
or obtained by writing to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#if !defined(__DIRENT_H)
#define __DIRENT_H 1

#ifdef __cplusplus
extern "C" {
#endif

#  include <windows.h>

#  define D_NAMLEN(dirent)	(strlen((dirent)->d_name))
//#  define dirent		lt__dirent
//#  define DIR			lt__DIR
//#  define opendir		lt__opendir
//#  define readdir		lt__readdir
//#  define closedir		lt__closedir

//LT_BEGIN_C_DECLS

struct dirent
{
  char d_name[260];
  int  d_namlen;
};

typedef struct _DIR
{
  HANDLE hSearch;
  WIN32_FIND_DATAA Win32FindDataA;
  BOOL firsttime;
  struct dirent file_info;
} DIR;

DIR*				opendir		(const char *path);
struct dirent*		readdir		(DIR *entry);
void				closedir	(DIR *entry);

#ifdef __cplusplus
}
#endif

#endif /*!defined(LT__DIRENT_H)*/

#endif