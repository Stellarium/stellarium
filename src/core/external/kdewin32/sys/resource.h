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

#ifndef KDEWIN_SYS_RESOURCE_H
#define KDEWIN_SYS_RESOURCE_H

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RLIMIT_CPU	0		/* CPU time in seconds */
#define RLIMIT_FSIZE	1		/* Maximum filesize */
#define RLIMIT_DATA	2		/* max data size */
#define RLIMIT_STACK	3		/* max stack size */
#define RLIMIT_CORE	4		/* max core file size */
#define RLIMIT_NOFILE	5		/* max number of open files */
#define RLIMIT_OFILE	RLIMIT_NOFILE	/* BSD name */
#define RLIMIT_AS	6		/* address space (virt. memory) limit */

#define RLIMIT_NLIMITS  7		/* upper bound of RLIMIT_* defines */
#define RLIM_NLIMITS    RLIMIT_NLIMITS

#define RLIM_INFINITY	(0xffffffffUL)
#define RLIM_SAVED_MAX	RLIM_INFINITY
#define RLIM_SAVED_CUR	RLIM_INFINITY

typedef unsigned long rlim_t;

struct rlimit {
	rlim_t	rlim_cur;
	rlim_t	rlim_max;
};

#define	RUSAGE_SELF	0		/* calling process */
#define	RUSAGE_CHILDREN	-1		/* terminated child processes */

/*
struct timeval {
  long tv_sec;
  long tv_usec;
}; 
*/
struct rusage {
	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
	long ru_maxrss;
	long ru_ixrss;               /* XXX: 0 */
	long ru_idrss;               /* XXX: sum of rm_asrss */
	long ru_isrss;               /* XXX: 0 */
	long ru_minflt;              /* any page faults not requiring I/O */
	long ru_majflt;              /* any page faults requiring I/O */
	long ru_nswap;               /* swaps */
	long ru_inblock;             /* block input operations */
	long ru_oublock;             /* block output operations */
	long ru_msgsnd;              /* messages sent */
	long ru_msgrcv;              /* messages received */
	long ru_nsignals;            /* signals received */
	long ru_nvcsw;               /* voluntary context switches */
	long ru_nivcsw;              /* involuntary " */
#define ru_last         ru_nivcsw
};

int getrlimit (int __resource, struct rlimit *__rlp);
int setrlimit (int __resource, const struct rlimit *__rlp);
int getrusage (int __who, struct rusage *__rusage);

#ifdef __cplusplus
}
#endif

#endif  // KDEWIN_SYS_RESOURCE_H

