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

/* An implementation of semaphores using mutexes and condition variables */

#include "SDL_thread.h"
#include "SDL_systhread_c.h"


struct SDL_semaphore
{
	struct SignalSemaphore Sem;
};

#undef D

#define D(x)

SDL_sem *SDL_CreateSemaphore(Uint32 initial_value)
{
	SDL_sem *sem;

	sem = (SDL_sem *)SDL_malloc(sizeof(*sem));

	if ( ! sem ) {
		SDL_OutOfMemory();
		return(0);
	}

	D(bug("Creating semaphore %lx...\n",sem));

	SDL_memset(sem,0,sizeof(*sem));

	InitSemaphore(&sem->Sem);

	return(sem);
}

void SDL_DestroySemaphore(SDL_sem *sem)
{
	D(bug("Destroying semaphore %lx...\n",sem));

	if ( sem ) {
// Condizioni per liberare i task in attesa?
		SDL_free(sem);
	}
}

int SDL_SemTryWait(SDL_sem *sem)
{
	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

	D(bug("TryWait semaphore...%lx\n",sem));

	ObtainSemaphore(&sem->Sem);
//	ReleaseSemaphore(&sem->Sem);

	return 1;
}

int SDL_SemWaitTimeout(SDL_sem *sem, Uint32 timeout)
{
	int retval;


	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

	D(bug("WaitTimeout (%ld) semaphore...%lx\n",timeout,sem));

	/* A timeout of 0 is an easy case */
	if ( timeout == 0 ) {
		ObtainSemaphore(&sem->Sem);
		return 1;
	}
	if(!(retval=AttemptSemaphore(&sem->Sem)))
	{
		SDL_Delay(timeout);
		retval=AttemptSemaphore(&sem->Sem);
	}

	if(retval==TRUE)
	{
//		ReleaseSemaphore(&sem->Sem);
		retval=1;
	}

	return retval;
}

int SDL_SemWait(SDL_sem *sem)
{
	ObtainSemaphore(&sem->Sem);
	return 0;
}

Uint32 SDL_SemValue(SDL_sem *sem)
{
	Uint32 value;

	value = 0;
	if ( sem ) {
		#ifdef STORMC4_WOS
		value = sem->Sem.ssppc_SS.ss_NestCount;
		#else
		value = sem->Sem.ss_NestCount;
		#endif
	}
	return value;
}

int SDL_SemPost(SDL_sem *sem)
{
	if ( ! sem ) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}
	D(bug("SemPost semaphore...%lx\n",sem));

	ReleaseSemaphore(&sem->Sem);
	return 0;
}

