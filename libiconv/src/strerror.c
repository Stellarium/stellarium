/* strerror.c --- ANSI C compatible system error routine

   Copyright (C) 1986, 1988-1989, 1991, 2002-2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

# include <config.h>

#if !HAVE_STRERROR

#if 0 /* Avoid colliding declaration of sys_errlist.  */
# include <stdio.h>
#endif

extern int sys_nerr;
extern char *sys_errlist[];

char *
strerror (int n)
{
  static char mesg[30];

  if (n < 0 || n >= sys_nerr)
    {
      sprintf (mesg, "Unknown error (%d)", n);
      return mesg;
    }
  else
    return sys_errlist[n];
}

#else

/* This declaration is solely to ensure that after preprocessing
   this file is never empty.  */
typedef int dummy;

#endif
