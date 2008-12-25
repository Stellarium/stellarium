/*
   This file is part of the KDE libraries
   Copyright (c) 2006 Christian Ehrlicher <ch.ehrlicher@gmx.de>

   These sources are based on ftp://g.oswego.edu/pub/misc/malloc.c
   file by Doug Lea, released to the public domain.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <windows.h>

#include <assert.h>
#include "kdewin32/stdlib.h"
#include "kdewin32/errno.h"
#include <io.h>
#include "kdewin32/sys/mman.h"
#include "kdewin32/sys/socket.h"

#ifndef SECTION_MAP_EXECUTE_EXPLICIT
//not defined in the February 2003 version of the Platform  SDK 
#define SECTION_MAP_EXECUTE_EXPLICIT 0x0020
#endif

#ifndef FILE_MAP_EXECUTE
//not defined in the February 2003 version of the Platform  SDK 
#define FILE_MAP_EXECUTE SECTION_MAP_EXECUTE_EXPLICIT
#endif

#define MUNMAP_FAILURE  (-1)

#define USE_MALLOC_LOCK 1

struct mmapInfos {
    HANDLE hFile;   // the duplicated fd
    HANDLE hMap;    // handle returned by CreateFileMapping
    void* start;    // ptr returned by MapViewOfFile
};

CRITICAL_SECTION cs;

static int g_curMMapInfos = 0;
static int g_maxMMapInfos = -1;
static struct mmapInfos *g_mmapInfos = NULL;
#define NEW_MMAP_STRUCT_CNT 10

static int mapProtFlags(int flags, DWORD *dwAccess)
{
    if ( ( flags & PROT_READ ) == PROT_READ ) {
        if ( ( flags & PROT_WRITE ) == PROT_WRITE ) {
            *dwAccess = FILE_MAP_WRITE;
            if ( ( flags & PROT_EXEC ) == PROT_EXEC ) {
                return PAGE_EXECUTE_READWRITE;
            }
            return PAGE_READWRITE;
        }
        if ( ( flags & PROT_EXEC ) == PROT_EXEC ) {
            *dwAccess = FILE_MAP_EXECUTE;
            return PAGE_EXECUTE_READ;
        }
        *dwAccess = FILE_MAP_READ;
        return PAGE_READONLY;
    }
    if ( ( flags & PROT_WRITE ) == PROT_WRITE ) {
        *dwAccess = FILE_MAP_COPY;
        return PAGE_WRITECOPY;
    }   
    if ( ( flags & PROT_EXEC ) == PROT_EXEC ) {
        *dwAccess = FILE_MAP_EXECUTE;
        return PAGE_EXECUTE_READ;
    }
    *dwAccess = 0;
    return 0;   
}

void *mmap(void *start, size_t length, int prot , int flags, int fd, off_t offset)
{
    struct mmapInfos mmi;
    DWORD dwAccess;
    DWORD flProtect;
    DWORD dwFlags;
    HANDLE hfd;

    if ( g_maxMMapInfos == -1 ) {
        g_maxMMapInfos = 0;
        InitializeCriticalSection( &cs );
    }

    flProtect = mapProtFlags( prot, &dwAccess );
    if ( flProtect == 0 ) {
        _set_errno( EINVAL );
        return MAP_FAILED;
    }
    // we don't support this atm
    if ( flags == MAP_FIXED ) {
        _set_errno( ENOTSUP );
        return MAP_FAILED;
    }

    if ( fd == -1 ) {
        _set_errno( EBADF );
        return MAP_FAILED;
    }

    // fd can be a crt or a win32 handle -> convert to win32 handle
    if(!GetHandleInformation( (HANDLE)fd, &dwFlags )) {
        if(GetLastError() == ERROR_INVALID_HANDLE) {
            hfd = (HANDLE)_get_osfhandle( fd );
            if ( hfd == INVALID_HANDLE_VALUE )
                return MAP_FAILED;
        } else {
            return MAP_FAILED;
        }
    } else {
        hfd = (HANDLE)fd;
    }

    if ( !DuplicateHandle( GetCurrentProcess(), hfd, GetCurrentProcess(),
                           &mmi.hFile, 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
#ifdef _DEBUG
        DWORD dwLastErr = GetLastError();
#endif
        return MAP_FAILED;
    }
    mmi.hMap = CreateFileMapping( mmi.hFile, NULL, flProtect,
                                  0, length, NULL );
    if ( mmi.hMap == 0 ) {
        CloseHandle( mmi.hFile );
        _set_errno( EACCES );
        return MAP_FAILED;
    }

    mmi.start = MapViewOfFile( mmi.hMap, dwAccess, 0, offset, 0 );
    if ( mmi.start == 0 ) {
        DWORD dwLastErr = GetLastError();
        CloseHandle( mmi.hMap );
        CloseHandle( mmi.hFile );
        if ( dwLastErr == ERROR_MAPPED_ALIGNMENT )
            _set_errno( EINVAL );
        else
            _set_errno( EACCES );
        return MAP_FAILED;
    }
    EnterCriticalSection( &cs );
    if ( g_mmapInfos == NULL ) {
        g_maxMMapInfos = NEW_MMAP_STRUCT_CNT;
        g_mmapInfos = ( struct mmapInfos* )calloc( g_maxMMapInfos,
                      sizeof( struct mmapInfos ) );
    }
    if( g_curMMapInfos == g_maxMMapInfos) {
        g_maxMMapInfos += NEW_MMAP_STRUCT_CNT;
        g_mmapInfos = ( struct mmapInfos* )realloc( g_mmapInfos,
                      g_maxMMapInfos * sizeof( struct mmapInfos ) );
    }
    memcpy( &g_mmapInfos[g_curMMapInfos], &mmi, sizeof( struct mmapInfos) );
    g_curMMapInfos++;
    
    LeaveCriticalSection( &cs );
    
    return mmi.start;
}

int munmap(void *start, size_t length)
{
    int i, j;

    for( i = 0; i < g_curMMapInfos; i++ ) {
        if( g_mmapInfos[i].start == start )
            break;
    }
    if( i == g_curMMapInfos ) {
        _set_errno( EINVAL );
        return -1;
    }

    UnmapViewOfFile( g_mmapInfos[ i ].start );
    CloseHandle( g_mmapInfos[ i ].hMap );
    CloseHandle( g_mmapInfos[ i ].hFile );

    EnterCriticalSection( &cs );
    for( j = i + 1; j < g_curMMapInfos; j++ ) {
        memcpy( &g_mmapInfos[ j - 1 ], &g_mmapInfos[ j ],
                sizeof( struct mmapInfos ) );
    }
    g_curMMapInfos--;
    
    if( g_curMMapInfos == 0 ) {
        free( g_mmapInfos );
        g_mmapInfos = NULL;
        g_maxMMapInfos = 0;
    }
    LeaveCriticalSection( &cs );
    
    return 0;
}
