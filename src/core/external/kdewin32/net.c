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

#include "kdewin32/sys/types.h"

unsigned long int htonl (unsigned long int x)
{
  return ((((x & 0x000000ffU) << 24) |
	((x & 0x0000ff00U) << 8) |
	((x & 0x00ff0000U) >> 8) |
	((x & 0xff000000U) >> 24)));
}

unsigned long int ntohl (unsigned long int x)
{
	return htonl (x);
}

unsigned short htons (unsigned short x)
{
	return ((((x & 0x000000ffU) << 8) |
		((x & 0x0000ff00U) >> 8)));
}

unsigned short ntohs (unsigned short x)
{
	return htons (x);
}

