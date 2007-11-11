/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/*
    SDL_sysmutex.cpp

    Epoc version by Markus Mertama (w@iki.fi)
*/


/* Mutex functions using the Win32 API */

#include<e32std.h>

#include "SDL_error.h"
#include "SDL_mutex.h"



struct SDL_mutex
    {
    TInt handle;
    };

extern TInt CreateUnique(TInt (*aFunc)(const TDesC& aName, TAny*, TAny*), TAny*, TAny*);

TInt NewMutex(const TDesC& aName, TAny* aPtr1, TAny*)
    {
    return ((RMutex*)aPtr1)->CreateGlobal(aName);
    }

/* Create a mutex */
SDL_mutex *SDL_CreateMutex(void)
{
    RMutex rmutex;

    TInt status = CreateUnique(NewMutex, &rmutex, NULL);
	if(status != KErrNone)
	    {
			SDL_SetError("Couldn't create mutex");
		}
    SDL_mutex* mutex = new /*(ELeave)*/ SDL_mutex;
    mutex->handle = rmutex.Handle();
	return(mutex);
}

/* Free the mutex */
void SDL_DestroyMutex(SDL_mutex *mutex)
{
	if ( mutex ) 
	{
    RMutex rmutex;
    rmutex.SetHandle(mutex->handle);
	rmutex.Signal();
	rmutex.Close();
	delete(mutex);
    mutex = NULL;
	}
}

/* Lock the mutex */
int SDL_mutexP(SDL_mutex *mutex)
{
	if ( mutex == NULL ) {
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}
    RMutex rmutex;
    rmutex.SetHandle(mutex->handle);
	rmutex.Wait(); 
	return(0);
}

/* Unlock the mutex */
int SDL_mutexV(SDL_mutex *mutex)
{
	if ( mutex == NULL ) {
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}
	RMutex rmutex;
    rmutex.SetHandle(mutex->handle);
	rmutex.Signal();
	return(0);
}
