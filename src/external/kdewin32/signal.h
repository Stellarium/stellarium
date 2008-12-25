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

#ifndef KDE_SIGNAL_H
#define KDE_SIGNAL_H

#include "kdewin32/sys/types.h"

#ifdef  __cplusplus
extern "C" {
#endif

//additional defs (some are sommented out because winows defines these):
#define	SIGHUP	1	/* hangup */
/* #define	SIGINT	2*/	/* interrupt */
#define	SIGQUIT	3	/* quit */
/* #define	SIGILL	4*/	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
/* #define	SIGABRT 6*/	/* used by abort */
#define	SIGEMT	7	/* EMT instruction */
/* #define	SIGFPE	8*/	/* floating point exception */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
/* #define	SIGBUS	10	*/ /* bus error, only used in  kconfigbackend.cpp but this signal.h is not complete */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
/* #define	SIGTERM	15*/	/* software termination signal from kill */
#define	SIGURG	16	/* urgent condition on IO channel */
#define	SIGSTOP	17	/* sendable stop signal not from tty */
#define	SIGTSTP	18	/* stop signal from tty */
#define	SIGCONT	19	/* continue a stopped process */
#define	SIGCHLD	20	/* to parent on child stop or exit */
#define	SIGCLD	20	/* System V name for SIGCHLD */
#define	SIGTTIN	21	/* to readers pgrp upon background tty read */
#define	SIGTTOU	22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGIO	23	/* input/output possible signal */
#define	SIGPOLL	SIGIO	/* System V name for SIGIO */
#define	SIGXCPU	24	/* exceeded CPU time limit */
#define	SIGXFSZ	25	/* exceeded file size limit */
#define	SIGVTALRM 26	/* virtual time alarm */
#define	SIGPROF	27	/* profiling time alarm */
#define	SIGWINCH 28	/* window changed */
#define	SIGLOST 29	/* resource lost (eg, record-lock lost) */
#define	SIGUSR1 30	/* user defined signal 1 */
#define	SIGUSR2 31	/* user defined signal 2 */

/**
 Sends signal to a process. 
 */
int kill(pid_t pid, int sig);
 
/** A typedef for signal handler
 */
typedef void (*sighandler_t)(int);

/**
 Sets interrupt signal handling. 
 This is a wrapper of signal() function from the Windows CRT module
 provided for portability. Should be used as KDE_signal() in KDE code.

 instead of raising error (asseriting), for UNIX compatibility,
 it does nothing (but stil returns SIG_ERR) for the types other than
 SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM.

 Most notable examples of unsupported signal types are SIGKILL, SIGHUP and SIGBUS.

 @see http://msdn2.microsoft.com/en-us/library/xdkz3x12(VS.80).aspx for list 
 of supported signals.
 
 @return the previous handler associated with the given signal. 
 Returns SIG_ERR on error, in which case errno is set to EINVAL. 
*/
sighandler_t kdewin32_signal(int signum, sighandler_t handler);

#ifdef  __cplusplus
}
#endif

#include <../include/signal.h>

#endif // KDE_SIGNAL_H
