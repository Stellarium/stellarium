/* This file is part of the KDE project
   Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef _FCNTL_H
#define _FCNTL_H

#include "kdewin32/sys/types.h"
#include <../include/fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif


#define O_NDELAY	_FNDELAY

#define	_FNONBLOCK	0x4000	/* non blocking I/O (POSIX style) */ 
#define	O_NONBLOCK	_FNONBLOCK 

#define	_FOPEN		(-1)	/* from sys/file.h, kernel use only */
#define	_FREAD		0x0001	/* read enabled */
#define	_FWRITE		0x0002	/* write enabled */
#define	_FAPPEND	0x0008	/* append (writes guaranteed at the end) */
#define	_FMARK		0x0010	/* internal; mark during gc() */
#define	_FDEFER		0x0020	/* internal; defer for next gc pass */
#define	_FASYNC		0x0040	/* signal pgrp when data ready */
#define	_FSHLOCK	0x0080	/* BSD flock() shared lock present */
#define	_FEXLOCK	0x0100	/* BSD flock() exclusive lock present */
#define	_FCREAT		0x0200	/* open with file create */
#define	_FTRUNC		0x0400	/* open with truncation */
#define	_FEXCL		0x0800	/* error on open if file exists */
#define	_FNBIO		0x1000	/* non blocking I/O (sys5 style) */
#define	_FSYNC		0x2000	/* do all writes synchronously */
#define	_FNONBLOCK	0x4000	/* non blocking I/O (POSIX style) */
#define	_FNDELAY	_FNONBLOCK	/* non blocking I/O (4.2 style) */
#define	_FNOCTTY	0x8000	/* don't assign a ctty on this open */

/* XXX close on exec request; must match UF_EXCLOSE in user.h */
#define	FD_CLOEXEC	1	/* posix */

/* fcntl(2) requests */
#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags (close on exec) */
#define	F_SETFD		2	/* Set fildes flags (close on exec) */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */

#ifndef	_POSIX_SOURCE
# define	F_GETOWN 	5	/* Get owner - for ASYNC */
# define	F_SETOWN 	6	/* Set owner - for ASYNC */
#endif	/* !_POSIX_SOURCE */

#define	F_GETLK  	7	/* Get record-locking information */
#define	F_SETLK  	8	/* Set or Clear a record-lock (Non-Blocking) */
#define	F_SETLKW 	9	/* Set or Clear a record-lock (Blocking) */

#ifndef	_POSIX_SOURCE
# define	F_RGETLK 	10	/* Test a remote lock to see if it is blocked */
# define	F_RSETLK 	11	/* Set or unlock a remote lock */
# define	F_CNVT 		12	/* Convert a fhandle to an open fd */
# define	F_RSETLKW 	13	/* Set or Clear remote record-lock(Blocking) */
#endif	/* !_POSIX_SOURCE */

/* fcntl(2) flags (l_type field of flock structure) */
#define	F_RDLCK		1	/* read lock */
#define	F_WRLCK		2	/* write lock */
#define	F_UNLCK		3	/* remove lock(s) */

#ifndef	_POSIX_SOURCE
# define	F_UNLKSYS	4	/* remove remote locks for a given system */
#endif	/* !_POSIX_SOURCE */
	
/* not implemented: int __cdecl fcntl (int fd, int cmd,...); */


#ifdef __cplusplus
}
#endif
#include <io.h>

#endif /* _FCNTL_H */

