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

#ifndef KDEWIN_SYS_UN_H
#define KDEWIN_SYS_UN_H

/* POSIX requires only at least 100 bytes */
#define UNIX_PATH_LEN   108

struct sockaddr_un {
  unsigned short sun_family;              /* address family AF_LOCAL/AF_UNIX */
  char	         sun_path[UNIX_PATH_LEN]; /* 108 bytes of socket address     */
};

/* Evaluates the actual length of `sockaddr_un' structure. */
#define SUN_LEN(p) ((size_t)(((struct sockaddr_un *) NULL)->sun_path) \
		   + strlen ((p)->sun_path))

#endif  // KDEWIN_SYS_UN_H
