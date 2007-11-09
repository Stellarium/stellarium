/* Binary mode I/O.
   Copyright (C) 2001, 2003 Free Software Foundation, Inc.

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

#ifndef _BINARY_H
#define _BINARY_H

#include <fcntl.h>
/* For systems that distinguish between text and binary I/O.
   O_BINARY is usually declared in <fcntl.h>. */
#if !defined O_BINARY && defined _O_BINARY
  /* For MSC-compatible compilers.  */
# define O_BINARY _O_BINARY
# define O_TEXT _O_TEXT
#endif
#ifdef __BEOS__
  /* BeOS 5 has O_BINARY and O_TEXT, but they have no effect.  */
# undef O_BINARY
# undef O_TEXT
#endif
#if O_BINARY
# if !(defined __EMX__ || defined __DJGPP__)
#  define setmode _setmode
#  define fileno _fileno
# endif
# ifdef __DJGPP__
#  include <io.h> /* declares setmode() */
#  include <unistd.h> /* declares isatty() */
#  /* Avoid putting stdin/stdout in binary mode if it is connected to the
#     console, because that would make it impossible for the user to
#     interrupt the program through Ctrl-C or Ctrl-Break.  */
#  define SET_BINARY(fd) (!isatty (fd) ? (setmode (fd, O_BINARY), 0) : 0)
# else
#  define SET_BINARY(fd) setmode (fd, O_BINARY)
# endif
#else
  /* On reasonable systems, binary I/O is the default.  */
# define O_BINARY 0
# define SET_BINARY(fd) /* nothing */
#endif

#endif /* _BINARY_H */
