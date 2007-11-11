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

#ifdef SDL_JOYSTICK_AMIGA

/* This is the system specific header for the SDL joystick API */

#include <libraries/lowlevel.h>
#if defined(__SASC) || defined(STORMC4_WOS)
#include <proto/exec.h>
#include <proto/lowlevel.h>
#include <proto/graphics.h>
#else
#include <inline/exec.h>
#include <inline/lowlevel.h>
#include <inline/graphics.h>
#endif
#include "mydebug.h"

extern struct ExecBase *SysBase;
extern struct GfxBase *GfxBase;

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */


/* Amiga specific datas */
struct Library *LowLevelBase=NULL;

ULONG joybut[]=
{
	JPF_BUTTON_RED,
	JPF_BUTTON_BLUE,
	JPF_BUTTON_PLAY,
	JPF_BUTTON_YELLOW,
	JPF_BUTTON_GREEN,
	JPF_BUTTON_FORWARD,
	JPF_BUTTON_REVERSE,
};

struct joystick_hwdata
{
	ULONG joystate;
};

int SDL_SYS_JoystickInit(void)
{
	if(!LowLevelBase)
	{
		if(LowLevelBase=OpenLibrary("lowlevel.library",37))
			return 2;
	}
	else
		return 2;

	D(bug("%ld joysticks available.\n",SDL_numjoysticks));

	return 0;
}

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
	if(index<2&&LowLevelBase)
	{
		switch(index)
		{
			case 0:
				return "Port 1 Amiga Joystick/Joypad";
			case 1:
				return "Port 2 Amiga Joystick/Joypad";
		}
	}

	SDL_SetError("No joystick available with that index");
	return(NULL);
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */

int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	ULONG temp,i;
	D(bug("Opening joystick %ld\n",joystick->index));

	if(!(joystick->hwdata=SDL_malloc(sizeof(struct joystick_hwdata))))
		return -1;

/* This loop is to check if the controller is a joypad */

	for(i=0;i<20;i++)
	{
		temp=ReadJoyPort(joystick->index^1); // fix to invert amiga joyports
		WaitTOF();
	}

	if((temp&JP_TYPE_MASK)==JP_TYPE_GAMECTLR)
		joystick->nbuttons=7;
	else
		joystick->nbuttons=3;

	joystick->nhats=0;
	joystick->nballs=0;
	joystick->naxes=2;
	joystick->hwdata->joystate=0L;

	return 0;
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	ULONG data;
	int i;

	if(joystick->index<2)
	{
		data=ReadJoyPort(joystick->index);

		if(data&JP_DIRECTION_MASK)
		{
			if(data&JPF_JOY_DOWN)
			{
				if(!(joystick->hwdata->joystate&JPF_JOY_DOWN))
					SDL_PrivateJoystickAxis(joystick,0,127);
			}
			else if(data&JPF_JOY_UP)
			{
				if(!(joystick->hwdata->joystate&JPF_JOY_UP))
					SDL_PrivateJoystickAxis(joystick,0,-127);
			}
			else if(joystick->hwdata->joystate&(JPF_JOY_UP|JPF_JOY_DOWN))
				SDL_PrivateJoystickAxis(joystick,0,0);

			if(data&JPF_JOY_LEFT)
			{
				if(!(joystick->hwdata->joystate&JPF_JOY_LEFT))
					SDL_PrivateJoystickAxis(joystick,1,-127);
			}
			else if(data&JPF_JOY_RIGHT)
			{
				if(!(joystick->hwdata->joystate&JPF_JOY_RIGHT))
					SDL_PrivateJoystickAxis(joystick,1,127);
			}
			else if(joystick->hwdata->joystate&(JPF_JOY_LEFT|JPF_JOY_RIGHT))
				SDL_PrivateJoystickAxis(joystick,1,0);
		}
		else if(joystick->hwdata->joystate&(JPF_JOY_LEFT|JPF_JOY_RIGHT))
		{
				SDL_PrivateJoystickAxis(joystick,1,0);
		}
		else if(joystick->hwdata->joystate&(JPF_JOY_UP|JPF_JOY_DOWN))
		{
				SDL_PrivateJoystickAxis(joystick,0,0);
		}

		for(i=0;i<joystick->nbuttons;i++)
		{
			if( (data&joybut[i]) )
			{
				if(i==1)
					data&=(~(joybut[2]));

				if(!(joystick->hwdata->joystate&joybut[i]))
					SDL_PrivateJoystickButton(joystick,i,SDL_PRESSED);
			}
			else if(joystick->hwdata->joystate&joybut[i])
				SDL_PrivateJoystickButton(joystick,i,SDL_RELEASED);
		}

		joystick->hwdata->joystate=data;
	}

	return;
}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	if(joystick->hwdata)
		SDL_free(joystick->hwdata);
	return;
}

/* Function to perform any system-specific joystick related cleanup */

void SDL_SYS_JoystickQuit(void)
{
	if(LowLevelBase)
	{
		CloseLibrary(LowLevelBase);
		LowLevelBase=NULL;
		SDL_numjoysticks=0;
	}
	return;
}

#endif /* SDL_JOYSTICK_AMIGA */
