/* Error handler for noninteractive utilities
   Copyright (C) 1990-1998, 2000-2003 Free Software Foundation, Inc.
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#include <config.h>
#include <stdio.h>

#if !HAVE_ERROR_AT_LINE

#include "gettext.h"

#if HAVE_VPRINTF || HAVE_DOPRNT || _LIBC
# if __STDC__
#  include <stdarg.h>
#  define VA_START(args, lastarg) va_start(args, lastarg)
# else
#  include <varargs.h>
#  define VA_START(args, lastarg) va_start(args)
# endif
#else
# define va_alist a1, a2, a3, a4, a5, a6, a7, a8
# define va_dcl char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
#endif

#if STDC_HEADERS || _LIBC
# include <stdlib.h>
# include <string.h>
#else
void exit ();
#endif

#include "error.h"

#ifndef _
# define _(String) String
#endif

/* If NULL, error will flush stdout, then print on stderr the program
   name, a colon and a space.  Otherwise, error will call this
   function without parameters instead.  */
void (*error_print_progname) (
#if __STDC__ - 0
			      void
#endif
			      );

/* This variable is incremented each time `error' is called.  */
unsigned int error_message_count;



# if !HAVE_DECL_STRERROR_R && STRERROR_R_CHAR_P
#  ifndef HAVE_DECL_STRERROR_R
"this configure-time declaration test was not run"
#  endif
char *strerror_r ();
# endif

/* The calling program should define program_name and set it to the
   name of the executing program.  */
extern char *program_name;

# if HAVE_STRERROR_R || defined strerror_r
#  define __strerror_r strerror_r
# else
#  if HAVE_STRERROR
#   ifndef HAVE_DECL_STRERROR
"this configure-time declaration test was not run"
#   endif
#   if !HAVE_DECL_STRERROR
char *strerror ();
#   endif
#  else
static char *
private_strerror (int errnum)
{
  extern char *sys_errlist[];
  extern int sys_nerr;

  if (errnum > 0 && errnum <= sys_nerr)
    return _(sys_errlist[errnum]);
  return _("Unknown system error");
}
#   define strerror private_strerror
#  endif /* HAVE_STRERROR */
# endif	/* HAVE_STRERROR_R || defined strerror_r */

static void
print_errno_message (int errnum)
{
  char const *s;

#if defined HAVE_STRERROR_R || _LIBC
  char errbuf[1024];
# if STRERROR_R_CHAR_P || _LIBC
  s = __strerror_r (errnum, errbuf, sizeof errbuf);
# else
  if (__strerror_r (errnum, errbuf, sizeof errbuf) == 0)
    s = errbuf;
  else
    s = 0;
# endif
#else
  s = strerror (errnum);
#endif

#if !_LIBC
  if (! s)
    s = _("Unknown system error");
#endif

#if _LIBC && USE_IN_LIBIO
  if (_IO_fwide (stderr, 0) > 0)
    {
      __fwprintf (stderr, L": %s", s);
      return;
    }
#endif

  fprintf (stderr, ": %s", s);
}

#ifdef VA_START
static void
error_tail (int status, int errnum, const char *message, va_list args)
{
# if HAVE_VPRINTF || _LIBC
#  if _LIBC && USE_IN_LIBIO
  if (_IO_fwide (stderr, 0) > 0)
    {
#   define ALLOCA_LIMIT	2000
      size_t len = strlen (message) + 1;
      wchar_t *wmessage = NULL;
      mbstate_t st;
      size_t res;
      const char *tmp;

      do
	{
	  if (len < ALLOCA_LIMIT)
	    wmessage = (wchar_t *) alloca (len * sizeof (wchar_t));
	  else
	    {
	      if (wmessage != NULL && len / 2 < ALLOCA_LIMIT)
		wmessage = NULL;

	      wmessage = (wchar_t *) realloc (wmessage,
					      len * sizeof (wchar_t));

	      if (wmessage == NULL)
		{
		  fputws_unlocked (L"out of memory\n", stderr);
		  return;
		}
	    }

	  memset (&st, '\0', sizeof (st));
	  tmp =message;
	}
      while ((res = mbsrtowcs (wmessage, &tmp, len, &st)) == len);

      if (res == (size_t) -1)
	/* The string cannot be converted.  */
	wmessage = (wchar_t *) L"???";

      __vfwprintf (stderr, wmessage, args);
    }
  else
#  endif
    vfprintf (stderr, message, args);
# else
  _doprnt (message, args, stderr);
# endif
  va_end (args);

  ++error_message_count;
  if (errnum)
    print_errno_message (errnum);
# if _LIBC && USE_IN_LIBIO
  if (_IO_fwide (stderr, 0) > 0)
    putwc (L'\n', stderr);
  else
# endif
    putc ('\n', stderr);
  fflush (stderr);
  if (status)
    exit (status);
}
#endif


/* Print the program name and error message MESSAGE, which is a printf-style
   format string with optional args.
   If ERRNUM is nonzero, print its corresponding system error message.
   Exit with status STATUS if it is nonzero.  */
/* VARARGS */
void
#if defined VA_START && __STDC__
error (int status, int errnum, const char *message, ...)
#else
error (status, errnum, message, va_alist)
     int status;
     int errnum;
     char *message;
     va_dcl
#endif
{
#ifdef VA_START
  va_list args;
#endif

  fflush (stdout);
#ifdef _LIBC
# ifdef USE_IN_LIBIO
  _IO_flockfile (stderr);
# else
  __flockfile (stderr);
# endif
#endif
  if (error_print_progname)
    (*error_print_progname) ();
  else
    {
#if _LIBC && USE_IN_LIBIO
      if (_IO_fwide (stderr, 0) > 0)
	__fwprintf (stderr, L"%s: ", program_name);
      else
#endif
	fprintf (stderr, "%s: ", program_name);
    }

#ifdef VA_START
  VA_START (args, message);
  error_tail (status, errnum, message, args);
#else
  fprintf (stderr, message, a1, a2, a3, a4, a5, a6, a7, a8);

  ++error_message_count;
  if (errnum)
    print_errno_message (errnum);
  putc ('\n', stderr);
  fflush (stderr);
  if (status)
    exit (status);
#endif

#ifdef _LIBC
# ifdef USE_IN_LIBIO
  _IO_funlockfile (stderr);
# else
  __funlockfile (stderr);
# endif
#endif
}

/* Sometimes we want to have at most one error per line.  This
   variable controls whether this mode is selected or not.  */
int error_one_per_line;

void
#if defined VA_START && __STDC__
error_at_line (int status, int errnum, const char *file_name,
	       unsigned int line_number, const char *message, ...)
#else
error_at_line (status, errnum, file_name, line_number, message, va_alist)
     int status;
     int errnum;
     const char *file_name;
     unsigned int line_number;
     char *message;
     va_dcl
#endif
{
#ifdef VA_START
  va_list args;
#endif

  if (error_one_per_line)
    {
      static const char *old_file_name;
      static unsigned int old_line_number;

      if (old_line_number == line_number
	  && (file_name == old_file_name
	      || strcmp (old_file_name, file_name) == 0))
	/* Simply return and print nothing.  */
	return;

      old_file_name = file_name;
      old_line_number = line_number;
    }

  fflush (stdout);
#ifdef _LIBC
# ifdef USE_IN_LIBIO
  _IO_flockfile (stderr);
# else
  __flockfile (stderr);
# endif
#endif
  if (error_print_progname)
    (*error_print_progname) ();
  else
    {
#if _LIBC && USE_IN_LIBIO
      if (_IO_fwide (stderr, 0) > 0)
	__fwprintf (stderr, L"%s: ", program_name);
      else
#endif
	fprintf (stderr, "%s:", program_name);
    }

  if (file_name != NULL)
    {
#if _LIBC && USE_IN_LIBIO
      if (_IO_fwide (stderr, 0) > 0)
	__fwprintf (stderr, L"%s:%d: ", file_name, line_number);
      else
#endif
	fprintf (stderr, "%s:%d: ", file_name, line_number);
    }

#ifdef VA_START
  VA_START (args, message);
  error_tail (status, errnum, message, args);
#else
  fprintf (stderr, message, a1, a2, a3, a4, a5, a6, a7, a8);

  ++error_message_count;
  if (errnum)
    print_errno_message (errnum);
  putc ('\n', stderr);
  fflush (stderr);
  if (status)
    exit (status);
#endif

#ifdef _LIBC
# ifdef USE_IN_LIBIO
  _IO_funlockfile (stderr);
# else
  __funlockfile (stderr);
# endif
#endif
}

#ifdef _LIBC
/* Make the weak alias.  */
# undef error
# undef error_at_line
weak_alias (__error, error)
weak_alias (__error_at_line, error_at_line)
#endif

#endif
