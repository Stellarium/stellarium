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
    SDL_syssem.cpp

    Epoc version by Markus Mertama  (w@iki.fi)
*/

/* Semaphore functions using the Win32 API */

#include <e32std.h>

#include "SDL_error.h"
#include "SDL_thread.h"


#define SDL_MUTEX_TIMEOUT -2

struct SDL_semaphore
 {
 TInt handle;
 TInt count;
 };

extern TInt CreateUnique(TInt (*aFunc)(const TDesC& aName, TAny*, TAny*), TAny*, TAny*);
extern TInt NewThread(const TDesC& aName, TAny* aPtr1, TAny* aPtr2);

TInt NewSema(const TDesC& aName, TAny* aPtr1, TAny* aPtr2) 
    {
    TInt value = *((TInt*) aPtr2);
    return ((RSemaphore*)aPtr1)->CreateGlobal(aName, value);
    }

/* Create a semaphore */
SDL_sem *SDL_CreateSemaphore(Uint32 initial_value)
{
   RSemaphore s;
   TInt status = CreateUnique(NewSema, &s, &initial_value);
   if(status != KErrNone)
	 {
			SDL_SetError("Couldn't create semaphore");
	}
    SDL_semaphore* sem = new /*(ELeave)*/ SDL_semaphore;  
    sem->handle = s.Handle();
	sem->count = initial_value;
	return(sem);
}

/* Free the semaphore */
void SDL_DestroySemaphore(SDL_sem *sem)
{
	if ( sem ) 
	{
    RSemaphore sema;
    sema.SetHandle(sem->handle);
	sema.Signal(sema.Count());
    sema.Close();
    delete sem;
	sem = NULL;
	}
}


  struct TInfo
    {
        TInfo(TInt aTime, TInt aHandle) : 
        iTime(aTime), iHandle(aHandle), iVal(0) {}
        TInt iTime;
        TInt iHandle;
        TInt iVal; 
    };

TBool ThreadRun(TAny* aInfo)
    {
        TInfo* info = STATIC_CAST(TInfo*, aInfo);
        User::After(info->iTime);
        RSemaphore sema;
        sema.SetHandle(info->iHandle);
        sema.Signal();
        info->iVal = SDL_MUTEX_TIMEOUT;
        return 0;
    }
    
  
    
    
void _WaitAll(SDL_sem *sem)
    {
       //since SemTryWait may changed the counter.
       //this may not be atomic, but hopes it works.
    RSemaphore sema;
    sema.SetHandle(sem->handle);
    sema.Wait();
    while(sem->count < 0)
        {
        sema.Wait();
        }    
    }

int SDL_SemWaitTimeout(SDL_sem *sem, Uint32 timeout)
{
	if ( ! sem ) {
		SDL_SetError("Passed a NULL sem");
		return -1;
	}

	if ( timeout == SDL_MUTEX_MAXWAIT )
	    {
	    _WaitAll(sem);
		return SDL_MUTEX_MAXWAIT;
	    } 
	
	
	RThread thread;
	
	TInfo* info = new (ELeave)TInfo(timeout, sem->handle);
	
    TInt status = CreateUnique(NewThread, &thread, info);
  
	if(status != KErrNone)
	    return status;
	
	thread.Resume();
	
	_WaitAll(sem);
	
	if(thread.ExitType() == EExitPending)
	    {
	        thread.Kill(SDL_MUTEX_TIMEOUT);
	    }
	
	thread.Close();
	
	return info->iVal;
}

int SDL_SemTryWait(SDL_sem *sem)
{
    if(sem->count > 0)
        {
        sem->count--;
        }
    return SDL_MUTEX_TIMEOUT;
}

int SDL_SemWait(SDL_sem *sem)
{
	return SDL_SemWaitTimeout(sem, SDL_MUTEX_MAXWAIT);
}

/* Returns the current count of the semaphore */
Uint32 SDL_SemValue(SDL_sem *sem)
{
	if ( ! sem ) {
		SDL_SetError("Passed a NULL sem");
		return 0;
	}
	return sem->count;
}

int SDL_SemPost(SDL_sem *sem)
{
	if ( ! sem ) {
		SDL_SetError("Passed a NULL sem");
		return -1;
	}
	sem->count++;
    RSemaphore sema;
    sema.SetHandle(sem->handle);
	sema.Signal();
	return 0;
}
