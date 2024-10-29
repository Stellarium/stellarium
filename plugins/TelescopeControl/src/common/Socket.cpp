/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains sample server classes (dummy, Meade LX200).

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

#ifdef Q_OS_WIN
  #include <Windows.h> // GetSystemTimeAsFileTime
#else
  #include <sys/time.h>
#endif

long long int GetNow(void)
{
	long long int t;
#ifdef Q_OS_WIN
	union
	{
		FILETIME file_time;
		qint64 t;
	} tmp;
	GetSystemTimeAsFileTime(&tmp.file_time);
	// convert from NT/NTFS time epoch (1601-01-01 0:00:00 UTC) in μs to UNIX epoch (1970-01-01 0:00:00 UTC) in μs. The difference is:
	// 24 hour/day × 3600 s/hour × 10^6 μs/s × ((369×365+89) day).
	// Here 369=1970-1601, and 89 is the number of leap years in the interval.
	// The division by 10 in the minuend is to convert from 100-nanosecond intervals to microseconds.
	t = (tmp.t/10) - 86400000000LL*(369*365+89);
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	t = tv.tv_sec * 1000000LL + tv.tv_usec;
#endif
	return t;
}

void Socket::hangup(void)
{
	if (!IS_INVALID_SOCKET(fd))
	{
		close(fd);
		fd = INVALID_SOCKET;
	}
}

