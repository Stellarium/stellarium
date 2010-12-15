/* This file is part of the KDE project
   Copyright (C) 2007 Ralf Habacker <ralf.habacker@freenet.de>

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
#ifndef KDE_BYTESWAP_H
#define KDE_BYTESWAP_H

#define bswap_16(x) (((x) << 8) & 0xff00) | (((x) >> 8 ) & 0xff)
#define bswap_32(x) (((x) << 24) & 0xff000000)  \
                    | (((x) << 8) & 0xff0000)   \
                    | (((x) >> 8) & 0xff00)     \
                    | (((x) >> 24) & 0xff )
#define bswap_64(x) ((((x) & 0xff00000000000000ull) >> 56) \
                    | (((x) & 0x00ff000000000000ull) >> 40) \
                    | (((x) & 0x0000ff0000000000ull) >> 24) \
                    | (((x) & 0x000000ff00000000ull) >> 8) \
                    | (((x) & 0x00000000ff000000ull) << 8) \
                    | (((x) & 0x0000000000ff0000ull) << 24) \
                    | (((x) & 0x000000000000ff00ull) << 40) \
                    | (((x) & 0x00000000000000ffull) << 56))

#endif 
