/* This file is part of the KDE project
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

#include <windows.h>

#include "kdewin32/errno.h"
#include "kdewin32/stdlib.h"
#include "kdewin32/stdio.h"
#include "kdewin32/string.h"

#if defined(_MSC_VER) && _MSC_VER >= 1400
// use secure functions declared in msvc >= 2005
#define KDEWIN32_USE_ENV_S
#endif

static void putenvMsvcrt(const char *name, const char *value)
{
    typedef int (*msvc6putenv)(const char *envstring);
    static msvc6putenv s_msvcrtputenv = 0;
    static int alreadyResolved = 0;
    int i;
    char * a;

    if( !alreadyResolved ) {
#ifdef _MSC_VER
        HANDLE hModule = LoadLibraryA("msvcrt");
#else
        // it doesn't work when msvcr80 isn't loaded - we end up in an error
        // message due to crappy manifest things :(
        // maybe someone has an idea how to fix this.
        //HANDLE hModule = LoadLibraryA("msvcr80");
        HANDLE hModule = NULL;
#endif
        if( hModule )
            s_msvcrtputenv = (msvc6putenv)GetProcAddress(hModule, "_putenv");
        alreadyResolved = 1;
    }
    if( !s_msvcrtputenv )
        return;

    i = strlen(name) + (value ? strlen(value) : 0) + 2;
    a = (char*)malloc(i);
    if (!a) return;

    strcpy(a, name);
    strcat(a, "=");
    if (value)
      strcat(a, value);

    s_msvcrtputenv(a);
    free(a);
}

// from kdecore/fakes.c
int setenv(const char *name, const char *value, int overwrite)
{
#ifndef KDEWIN32_USE_ENV_S
    int i, iRet;
    char * a;
#endif

    if (!overwrite && getenv(name)) return 0;

    // make sure to set the env var in all our possible runtime environments
    putenvMsvcrt(name, value);
    //SetEnvironmentVariableA(name, value);     // unsure if we need it...

#ifdef KDEWIN32_USE_ENV_S
    return _putenv_s(name, value ? value : "");
#else 
    if (!name) return -1;

    i = strlen(name) + (value ? strlen(value) : 0) + 2;
    a = (char*)malloc(i);
    if (!a) return 1;

    strcpy(a, name);
    strcat(a, "=");
    if (value)
      strcat(a, value);

    iRet = putenv(a);
    free(a);
    return iRet;
#endif
}


// from kdecore/fakes.c
void unsetenv (const char *name)
{
  if (name == NULL || *name == '\0' || strchr (name, '=') != NULL)
    {
      errno = EINVAL;
      return;
    }

  setenv(name, "", 1);
}

long int random()
{
	return rand();
}

void srandom(unsigned int seed)
{
	srand(seed);
}


double drand48(void)
{
	return 1.0*rand()/(RAND_MAX+1.0);
}

int rand_r(unsigned *seed)
{
    srand(*seed);
    return rand();
}
