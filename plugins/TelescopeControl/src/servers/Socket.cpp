/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains smaple server classes (dummy, Meade LX200).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "Socket.hpp"

#ifdef Q_OS_WIN32
  #include <windows.h> // GetSystemTimeAsFileTime
#else
  #include <sys/time.h>
#endif

long long int GetNow(void)
{
#ifdef Q_OS_WIN32
	union
	{
		FILETIME file_time;
		__int64 t;
	} tmp;
	GetSystemTimeAsFileTime(&tmp.file_time);
	return (tmp.t/10) - 86400000000LL*(369*365+89);
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 1000000LL + tv.tv_usec;
#endif
}

void Socket::hangup(void)
{
	if (!IS_INVALID_SOCKET(fd))
	{
		close(fd);
		fd = INVALID_SOCKET;
	}
}

