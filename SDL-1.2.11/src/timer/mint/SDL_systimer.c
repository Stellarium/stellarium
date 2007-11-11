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

#ifdef SDL_TIMER_MINT

/*
 *	TOS/MiNT timer driver
 *	based on vbl vector
 *
 *	Patrice Mandin
 */

#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <mint/cookie.h>
#include <mint/sysvars.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>

#include "SDL_timer.h"
#include "../SDL_timer_c.h"
#include "SDL_thread.h"

#include "SDL_vbltimer_s.h"

/* The first ticks value of the application */
static Uint32 start;
static SDL_bool supervisor;
static int mint_present; /* can we use Syield() ? */

void SDL_StartTicks(void)
{
	void *oldpile;
	unsigned long dummy;

	/* Set first ticks value */
	oldpile=(void *)Super(0);
	start=*((volatile long *)_hz_200);
	Super(oldpile);

	start *= 5;	/* One _hz_200 tic is 5ms */

	mint_present = (Getcookie(C_MiNT, &dummy) == C_FOUND);
}

Uint32 SDL_GetTicks (void)
{
	Uint32 now;
	void *oldpile=NULL;

	/* Check if we are in supervisor mode 
	   (this is the case when called from SDL_ThreadedTimerCheck,
	   which is called from RunTimer, running in the vbl vector)
	*/
	if (!supervisor) {
		oldpile=(void *)Super(0);
	}

	now=*((volatile long *)_hz_200);

	if (!supervisor) {
		Super(oldpile);
	}

	return((now*5)-start);
}

void SDL_Delay (Uint32 ms)
{
	Uint32 now;

	now = SDL_GetTicks();
	while ((SDL_GetTicks()-now)<ms){
		if (mint_present) {
			Syield();
		}
	}
}

/* Data to handle a single periodic alarm */
static SDL_bool timer_installed=SDL_FALSE;

static void RunTimer(void)
{
	supervisor=SDL_TRUE;
	SDL_ThreadedTimerCheck();
	supervisor=SDL_FALSE;
}

/* This is only called if the event thread is not running */
int SDL_SYS_TimerInit(void)
{
	void *oldpile;

	supervisor=SDL_FALSE;

	/* Install RunTimer in vbl vector */
	oldpile=(void *)Super(0);
	timer_installed = !SDL_AtariVblInstall(RunTimer);
	Super(oldpile);

	if (!timer_installed) {
		return(-1);
	}
	return(SDL_SetTimerThreaded(0));
}

void SDL_SYS_TimerQuit(void)
{
	void *oldpile;

	if (timer_installed) {
		/* Uninstall RunTimer vbl vector */
		oldpile=(void *)Super(0);
		SDL_AtariVblUninstall(RunTimer);
		Super(oldpile);
		timer_installed = SDL_FALSE;
	}
}

int SDL_SYS_StartTimer(void)
{
	SDL_SetError("Internal logic error: MiNT uses vbl timer");
	return(-1);
}

void SDL_SYS_StopTimer(void)
{
	return;
}

#endif /* SDL_TIMER_MINT */
