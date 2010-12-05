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

#include <windows.h>

#include "kdewin32/stdlib.h"

char * kde_getenv(const char *name)
{
  char dummy[1];
  int len;
  char *p;
  if (!name)
    return 0;
  len = GetEnvironmentVariableA(name, dummy, 0);
  if (len == 0)
    return 0;

  len++;
	// NOTE: This creates a memory whole, because it will never be free'd 
	//       better use a static char vector 
  p = malloc(len);
  if (GetEnvironmentVariableA(name, p, len))
    return p;

  free(p);
  return 0;
}

