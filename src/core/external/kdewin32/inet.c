/* This file is part of the KDE project
   Copyright (C) 2006 Peter Kuemmel <syntheticpp@gmx.net>
   Copyright (C) 2006 Christian Ehrlicher <ch.ehrlicher@gmx.de>

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


#ifdef _MSC_VER
#include "kdewin32/winsock2.h"
#endif
#include <ws2tcpip.h>
#include "kdewin32/arpa/inet.h"

int inet_aton(const char *src, struct in_addr *addr)
{
	unsigned long ret = inet_addr( src );
	if ( ret == INADDR_NONE ) {
		if( strcmp( "255.255.255.255", src ) )
		    return 0;
		addr->s_addr = ret;
		return 1;
	}	
	addr->s_addr = ret;
	return 1;
}

#if __MINGW32__ || !defined(NTDDI_VERSION) || (NTDDI_VERSION < NTDDI_LONGHORN)
// backward compatibility functions to prevent symbol not found runtime errors with older kde releases
#undef kde_inet_pton
#undef inet_pton
int inet_pton(int af, const char * src, void * dst)
{
    return kde_inet_pton(af,src,dst);
}

#undef kde_inet_ntop
#undef inet_ntop
const char *inet_ntop(int af, const void *src, char *dst, size_t cnt)
{
    return kde_inet_ntop(af, src, dst, cnt);
}
#endif
