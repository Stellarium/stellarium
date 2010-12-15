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

#ifndef KDEWIN_SYS_SOCKET_H
#define KDEWIN_SYS_SOCKET_H

#include <sys/time.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef socklen_t
#define socklen_t int
#endif

#define EALREADY      WSAEALREADY    
#define ECONNABORTED  WSAECONNABORTED
#define ECONNREFUSED  WSAECONNREFUSED
#define ECONNRESET    WSAECONNRESET  
#define EHOSTDOWN     WSAEHOSTDOWN   
#define EHOSTUNREACH  WSAEHOSTUNREACH
#define EINPROGRESS   WSAEINPROGRESS 
#define EISCONN       WSAEISCONN     
#define ENETDOWN      WSAENETDOWN    
#define ENETRESET     WSAENETRESET   
#define ENETUNREACH   WSAENETUNREACH 
#define EWOULDBLOCK   WSAEWOULDBLOCK 
#define EADDRINUSE    WSAEADDRINUSE
#define ENOTSUP       ENOSYS
#define ETIMEDOUT     WSAETIMEDOUT
#define ENOTSOCK      WSAENOTSOCK
#define ENOBUFS       WSAENOBUFS
#define EMSGSIZE      WSAEMSGSIZE
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT

/* better do this at the app level
#include <sys/ioctl.h>
#undef ioctl
#define ioctl(a,b,c) ioctlsocket(a,b,(u_long*)c)
*/
/*
 does not work yet, replaces to many close functions()
#include <io.h>

// replacement for unix close function 
inline void kde_close(int fd, char *a, int b)
{
	u_long res; 
	if (ioctlsocket(fd,FIONREAD,&res) == 0) {
		fprintf(stderr,"%s socket close() called at %s %d",a,b);
		closesocket(fd);
	}
	else {
		fprintf(stderr,"%s close() called at %s %d",a,b);
		close(fd);
	}
}

#define close(a) kde_close(a,__FILE__,__LINE__)
*/ 


#endif  // KDEWIN_SYS_SOCKET_H
