/* This file is part of the KDE project
   Copyright (C) 2003-2005 Jaroslaw Staniek <js@iidea.pl>
   Copyright (C) 2005 Christian Ehrlicher <Ch.Ehrlicher@gmx.de>

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

#include "kdewin32/time.h"
#include "kdewin32/sys/time.h"
#include "kdewin32/errno.h"

#define KDE_SECONDS_SINCE_1601	11644473600LL
#define KDE_USEC_IN_SEC			1000000LL

//after Microsoft KB167296  
static void UnixTimevalToFileTime(struct timeval t, LPFILETIME pft)
{
	LONGLONG ll;

	ll = Int32x32To64(t.tv_sec, KDE_USEC_IN_SEC*10) + t.tv_usec*10 + KDE_SECONDS_SINCE_1601*KDE_USEC_IN_SEC*10;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

//
// sys/time.h fnctions
// Added a '2' to avoids a name collision...
int gettimeofday2(struct timeval *__p, void *__t)
{
	union {
		unsigned long long ns100; /*time since 1 Jan 1601 in 100ns units */
		FILETIME ft;
	} now;
	
	GetSystemTimeAsFileTime (&now.ft);
	__p->tv_usec = (long) ((now.ns100 / 10LL) % KDE_USEC_IN_SEC);
	__p->tv_sec  = (long)(((now.ns100 / 10LL ) / KDE_USEC_IN_SEC) - KDE_SECONDS_SINCE_1601);
	
	return (0); 
}

//errno==EACCES on read-only devices
int utimes(const char *filename, const struct timeval times[2])
{
	FILETIME LastAccessTime;
	FILETIME LastModificationTime;
	HANDLE hFile;

	if(times) {
		UnixTimevalToFileTime(times[0], &LastAccessTime);
		UnixTimevalToFileTime(times[1], &LastModificationTime);
	}
	else {
		GetSystemTimeAsFileTime(&LastAccessTime);
		GetSystemTimeAsFileTime(&LastModificationTime);
	}
 
	hFile=CreateFileA(filename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
	if(hFile==INVALID_HANDLE_VALUE) {
		switch(GetLastError()) {
			case ERROR_FILE_NOT_FOUND:
				errno=ENOENT;
				break;
			case ERROR_PATH_NOT_FOUND:
			case ERROR_INVALID_DRIVE:
				errno=ENOTDIR;
				break;
/*			case ERROR_WRITE_PROTECT:	//CreateFile sets ERROR_ACCESS_DENIED on read-only devices
				errno=EROFS;
				break;*/
			case ERROR_ACCESS_DENIED:
				errno=EACCES;
				break;
			default:
				errno=ENOENT;	//what other errors can occur?
		}
		return -1;
	}

	if(!SetFileTime(hFile, NULL, &LastAccessTime, &LastModificationTime)) {
		//can this happen?
		errno=ENOENT;
		return -1;
	}
	CloseHandle(hFile);
	return 0;
}

// this is no posix function
#if 0
int settimeofday(const struct timeval *__p, const struct timezone *__t)
{
	union {
		unsigned long long ns100; /*time since 1 Jan 1601 in 100ns units */
		FILETIME ft;
	} now;
	SYSTEMTIME st;
	
	now.ns100 = ((((__p->tv_sec + KDE_SECONDS_SINCE_1601) * KDE_USEC_IN_SEC) + __p->tv_usec) * 10LL);
	
	FileTimeToSystemTime( &now.ft, &st );
	SetSystemTime( &st );
	return 1;
}
#endif 

//
// time.h functions
//
struct tm* localtime_r(const time_t *t,struct tm *p)
{
	// CE: thread safe on windows - returns a ptr inside TLS afaik
	*p = *localtime( t );
	return p; 
}

struct tm* gmtime_r(const time_t *t, struct tm *p)
{
	// CE: thread safe on windows - returns a ptr inside TLS afaik
	*p = *gmtime( t );
	return p;
}
