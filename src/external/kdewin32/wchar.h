/* This file is part of the KDE libraries
   Copyright (C) 2007 Shane King <kde@dontletsstart.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDEWIN_WCHAR_H
#define KDEWIN_WCHAR_H

/* regular header from msvc includes */
#include <../include/wchar.h>

#ifndef wcscasecmp
#define wcscasecmp _wcsicmp
#endif

#endif /* KDEWIN_WCHAR_H */
