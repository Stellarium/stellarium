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

#ifndef KDEWIN_SYS_WAIT_H
#define KDEWIN_SYS_WAIT_H

#include "kdewin32/sys/types.h"
#include <sys/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WNOHANG 1
#define WUNTRACED 2

/* A status looks like:
      <2 bytes info> <2 bytes code>

      <code> == 0, child has exited, info is the exit value
      <code> == 1..7e, child has exited, info is the signal number.
      <code> == 7f, child has stopped, info was the signal number.
      <code> == 80, there was a core dump.
*/

#define WIFEXITED(w)	(((w) & 0xff) == 0)
#define WIFSIGNALED(w)	(((w) & 0x7f) > 0 && (((w) & 0x7f) < 0x7f))
#define WIFSTOPPED(w)	(((w) & 0xff) == 0x7f)
#define WEXITSTATUS(w)	(((w) >> 8) & 0xff)
#define WTERMSIG(w)	((w) & 0x7f)
#define WSTOPSIG	WEXITSTATUS

pid_t wait (int *);
pid_t waitpid (pid_t, int *, int);
pid_t wait3 (int *__status, int __options, struct rusage *__rusage);
pid_t wait4 (pid_t __pid, int *__status, int __options, struct rusage *__rusage);

union wait
  {
    int w_status;
    struct
      {
	unsigned int __w_termsig:7; /* Terminating signal.  */
	unsigned int __w_coredump:1; /* Set if dumped core.  */
	unsigned int __w_retcode:8; /* Return code if exited normally.  */
	unsigned int:16;
      } __wait_terminated;
    struct
      {
	unsigned int __w_stopval:8; /* W_STOPPED if stopped.  */
	unsigned int __w_stopsig:8; /* Stopping signal.  */
	unsigned int:16;
      } __wait_stopped;
  };

#define	w_termsig	__wait_terminated.__w_termsig
#define	w_coredump	__wait_terminated.__w_coredump
#define	w_retcode	__wait_terminated.__w_retcode
#define	w_stopsig	__wait_stopped.__w_stopsig
#define	w_stopval	__wait_stopped.__w_stopval

#ifdef __cplusplus
};
#endif

#endif  // KDEWIN_SYS_WAIT_H
