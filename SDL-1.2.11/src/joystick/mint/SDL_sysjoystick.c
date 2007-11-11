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

#ifdef SDL_JOYSTICK_MINT

/*
 *	Atari Joystick/Joypad drivers
 *
 *	Patrice Mandin
 */

#include <mint/cookie.h>
#include <mint/osbind.h>

#include "SDL_events.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include "../../video/ataricommon/SDL_ikbdinterrupt_s.h"
#include "../../video/ataricommon/SDL_xbiosevents_c.h"
#include "../../video/ataricommon/SDL_xbiosinterrupt_s.h"

/*--- Const ---*/

/* We can have:
	1 joystick on IKBD port 1, read via hardware I/O
      or same joystick on IKBD port 1, read via xbios
	2 joypads on ports A,B
	  or 4 joysticks on joypads ports A,B
	  or 1 lightpen on joypad port A
	  or 2 analog paddles on joypads ports A,B
	2 joysticks on parallel port
*/

enum {
	IKBD_JOY1=0,
	XBIOS_JOY1,
	PORTA_PAD,
	PORTB_PAD,
	PORTA_JOY0,
	PORTA_JOY1,
	PORTB_JOY0,
	PORTB_JOY1,
	PORTA_LP,
	PORTA_ANPAD,
	PORTB_ANPAD,
#if 0
	PARA_JOY0,
	PARA_JOY1,
#endif
	MAX_JOYSTICKS
};

enum {
	MCH_ST=0,
	MCH_STE,
	MCH_TT,
	MCH_F30,
	MCH_CLONE,
	MCH_ARANYM
};

/*	Joypad buttons
 *		Procontroller note:
 *			L,R are connected to 4,6
 *			X,Y,Z are connected to 7,8,9
 */

enum {
	JP_UP=0,	JP_DOWN,	JP_LEFT,	JP_RIGHT,
	JP_KPMULT,	JP_KP7,		JP_KP4,		JP_KP1,
	JP_KP0,		JP_KP8,		JP_KP5,		JP_KP2,
	JP_KPNUM,	JP_KP9,		JP_KP6,		JP_KP3,
	JP_PAUSE,	JP_FIRE0,	JP_UNDEF0,	JP_FIRE1,
	JP_UNDEF1,	JP_FIRE2,	JP_UNDEF2,	JP_OPTION
};

#define JP_NUM_BUTTONS 17

#define PORT_JS_RIGHT	(1<<0)
#define PORT_JS_LEFT	(1<<1)
#define PORT_JS_DOWN	(1<<2)
#define PORT_JS_UP		(1<<3)
#define PORT_JS_FIRE	(1<<4)

/*--- Types ---*/

typedef struct {
	SDL_bool enabled;
	unsigned char *name;
	Uint32 prevstate;
} atarijoy_t;

/*--- Variables ---*/

static atarijoy_t atarijoysticks[MAX_JOYSTICKS]={
	{SDL_FALSE,"IKBD joystick port 1",0},
	{SDL_FALSE,"Xbios joystick port 1",0},
	{SDL_FALSE,"Joypad port A",0},
	{SDL_FALSE,"Joypad port B",0},
	{SDL_FALSE,"Joystick 0 port A",0},
	{SDL_FALSE,"Joystick 1 port A",0},
	{SDL_FALSE,"Joystick 0 port B",0},
	{SDL_FALSE,"Joystick 1 port B",0},
	{SDL_FALSE,"Lightpen port A",0},
	{SDL_FALSE,"Analog paddle port A",0},
	{SDL_FALSE,"Analog paddle port B",0}
#if 0
	,{SDL_FALSE,"Joystick 0 parallel port",0},
	{SDL_FALSE,"Joystick 1 parallel port",0}
#endif
};

static const int jp_buttons[JP_NUM_BUTTONS]={
	JP_FIRE0,	JP_FIRE1,	JP_FIRE2,	JP_PAUSE,
	JP_OPTION,	JP_KPMULT,	JP_KPNUM,	JP_KP0,
	JP_KP1,		JP_KP2,		JP_KP3,		JP_KP4,
	JP_KP5,		JP_KP6,		JP_KP7,		JP_KP8,
	JP_KP9
};

static SDL_bool joypad_ports_enabled=SDL_FALSE;

/* Updated joypad ports */
static Uint16 jp_paddles[4];
static Uint16 jp_lightpens[2];
static Uint16 jp_directions;
static Uint16 jp_fires;
static Uint32 jp_joypads[2];

/*--- Functions prototypes ---*/

static int GetEnabledAtariJoystick(int index);
static void UpdateJoypads(void);

/*--- Functions ---*/

int SDL_SYS_JoystickInit(void)
{
	int i;
	unsigned long cookie_mch;
	const char *envr=SDL_getenv("SDL_JOYSTICK_ATARI");
	
#define TEST_JOY_ENABLED(env,idstring,num) \
	if (SDL_strstr(env,idstring"-off")) { \
		atarijoysticks[num].enabled=SDL_FALSE; \
	} \
	if (SDL_strstr(env,idstring"-on")) { \
		atarijoysticks[num].enabled=SDL_TRUE; \
	}

	/* Cookie _MCH present ? if not, assume ST machine */
	if (Getcookie(C__MCH, &cookie_mch) != C_FOUND) {
		cookie_mch = MCH_ST << 16;
	}

	/* Enable some default joysticks */
	if ((cookie_mch == MCH_ST<<16) || ((cookie_mch>>16) == MCH_STE) ||
		(cookie_mch == MCH_TT<<16) || (cookie_mch == MCH_F30<<16) ||
		(cookie_mch == MCH_ARANYM<<16)) {
		atarijoysticks[IKBD_JOY1].enabled=(SDL_AtariIkbd_enabled!=0);
	}
	if ((cookie_mch == MCH_STE<<16) || (cookie_mch == MCH_F30<<16)) {
		atarijoysticks[PORTA_PAD].enabled=SDL_TRUE;
		atarijoysticks[PORTB_PAD].enabled=SDL_TRUE;
	}
	if (!atarijoysticks[IKBD_JOY1].enabled) {
		atarijoysticks[XBIOS_JOY1].enabled=(SDL_AtariXbios_enabled!=0);
	}

	/* Read environment for joysticks to enable */
	if (envr) {
		/* IKBD on any Atari, maybe clones */
		if ((cookie_mch == MCH_ST<<16) || ((cookie_mch>>16) == MCH_STE) ||
			(cookie_mch == MCH_TT<<16) || (cookie_mch == MCH_F30<<16) ||
			(cookie_mch == MCH_ARANYM<<16)) {
			if (SDL_AtariIkbd_enabled!=0) {
				TEST_JOY_ENABLED(envr, "ikbd-joy1", IKBD_JOY1);
			}
		}
		/* Joypads ports only on STE and Falcon */
		if ((cookie_mch == MCH_STE<<16) || (cookie_mch == MCH_F30<<16)) {
			TEST_JOY_ENABLED(envr, "porta-pad", PORTA_PAD);
			if (!atarijoysticks[PORTA_PAD].enabled) {
				TEST_JOY_ENABLED(envr, "porta-joy0", PORTA_JOY0);
				TEST_JOY_ENABLED(envr, "porta-joy1", PORTA_JOY1);
				if (!(atarijoysticks[PORTA_JOY0].enabled) && !(atarijoysticks[PORTA_JOY1].enabled)) {
					TEST_JOY_ENABLED(envr, "porta-lp", PORTA_LP);
					if (!atarijoysticks[PORTA_LP].enabled) {
						TEST_JOY_ENABLED(envr, "porta-anpad", PORTA_ANPAD);
					}
				}
			}

			TEST_JOY_ENABLED(envr, "portb-pad", PORTB_PAD);
			if (!atarijoysticks[PORTB_PAD].enabled) {
				TEST_JOY_ENABLED(envr, "portb-joy0", PORTB_JOY0);
				TEST_JOY_ENABLED(envr, "portb-joy1", PORTB_JOY1);
				if (!(atarijoysticks[PORTB_JOY0].enabled) && !(atarijoysticks[PORTB_JOY1].enabled)) {
					TEST_JOY_ENABLED(envr, "portb-anpad", PORTB_ANPAD);
				}
			}
		}

		if (!atarijoysticks[IKBD_JOY1].enabled) {
			if (SDL_AtariXbios_enabled!=0) {
				TEST_JOY_ENABLED(envr, "xbios-joy1", XBIOS_JOY1);
			}
		}
#if 0
		/* Parallel port on any Atari, maybe clones */
		if ((cookie_mch == MCH_ST<<16) || ((cookie_mch>>16) == MCH_STE) ||
			(cookie_mch == MCH_TT<<16) || (cookie_mch == MCH_F30<<16)) {
			TEST_JOY_ENABLED(envr, "para-joy0", PARA_JOY0);
			TEST_JOY_ENABLED(envr, "para-joy1", PARA_JOY1);
		}
#endif
	}

	/* Need to update joypad ports ? */
	joypad_ports_enabled=SDL_FALSE;
	for (i=PORTA_PAD;i<=PORTB_ANPAD;i++) {
		if (atarijoysticks[i].enabled) {
			joypad_ports_enabled=SDL_TRUE;
			break;
		}
	}

	SDL_numjoysticks = 0;
	for (i=0;i<MAX_JOYSTICKS;i++) {
		if (atarijoysticks[i].enabled) {
			++SDL_numjoysticks;
		}
	}

	return(SDL_numjoysticks);
}

static int GetEnabledAtariJoystick(int index)
{
	int i,j;

	/* Return the nth'index' enabled atari joystick */
	j=0;
	for (i=0;i<MAX_JOYSTICKS;i++) {
		if (!atarijoysticks[i].enabled) {
			continue;
		}

		if (j==index) {
			break;
		}

		++j;
	}
	if (i==MAX_JOYSTICKS)
		return -1;

	return i;
}

const char *SDL_SYS_JoystickName(int index)
{
	int numjoystick;

	numjoystick=GetEnabledAtariJoystick(index);
	if (numjoystick==-1)
		return NULL;

	return(atarijoysticks[numjoystick].name);
}

int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	int numjoystick;
	
	numjoystick=GetEnabledAtariJoystick(joystick->index);
	if (numjoystick==-1)
		return -1;
	
	joystick->naxes=0;
	joystick->nhats=0;
	joystick->nballs=0;

	switch(numjoystick) {
		case PORTA_PAD:
		case PORTB_PAD:
			joystick->nhats=1;
			joystick->nbuttons=JP_NUM_BUTTONS;
			break;
		case PORTA_LP:
		case PORTA_ANPAD:
		case PORTB_ANPAD:
			joystick->naxes=2;
			joystick->nbuttons=2;
			break;
		default:
			joystick->nhats=1;
			joystick->nbuttons=1;
			break;
	}

	return(0);
}

void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	int numjoystick;
	Uint8 hatstate;
	Uint32 curstate,prevstate;
	
	numjoystick=GetEnabledAtariJoystick(joystick->index);
	if (numjoystick==-1)
		return;

	prevstate = atarijoysticks[numjoystick].prevstate;

	if (joypad_ports_enabled) {
		Supexec(UpdateJoypads);
	}

	switch (numjoystick) {
		case IKBD_JOY1:
		case XBIOS_JOY1:
			{
				curstate = 0;

				if (numjoystick==IKBD_JOY1) {
					curstate = SDL_AtariIkbd_joystick & 0xff;
				}
				if (numjoystick==XBIOS_JOY1) {
					curstate = SDL_AtariXbios_joystick & 0xff;
				}

				if (curstate != prevstate) {
					hatstate = SDL_HAT_CENTERED;
					if (curstate & IKBD_JOY_LEFT) {
						hatstate |= SDL_HAT_LEFT;
					}
					if (curstate & IKBD_JOY_RIGHT) {
						hatstate |= SDL_HAT_RIGHT;
					}
					if (curstate & IKBD_JOY_UP) {
						hatstate |= SDL_HAT_UP;
					}
					if (curstate & IKBD_JOY_DOWN) {
						hatstate |= SDL_HAT_DOWN;
					}
					SDL_PrivateJoystickHat(joystick, 0, hatstate);

					/* Button */
					if ((curstate & IKBD_JOY_FIRE) && !(prevstate & IKBD_JOY_FIRE)) {
						SDL_PrivateJoystickButton(joystick,0,SDL_PRESSED);
					}
					if (!(curstate & IKBD_JOY_FIRE) && (prevstate & IKBD_JOY_FIRE)) {
						SDL_PrivateJoystickButton(joystick,0,SDL_RELEASED);
					}
				}
				atarijoysticks[numjoystick].prevstate = curstate;
			}
			break;
		case PORTA_PAD:
		case PORTB_PAD:
			{
				int numjoypad,i;
				
				numjoypad=0;
				if (numjoystick==PORTB_PAD) numjoypad=1;
				
				curstate=jp_joypads[numjoypad];
				if (curstate!=prevstate) {
					hatstate = SDL_HAT_CENTERED;
					if (curstate & (1<<JP_LEFT)) {
						hatstate |= SDL_HAT_LEFT;
					}
					if (curstate & (1<<JP_RIGHT)) {
						hatstate |= SDL_HAT_RIGHT;
					}
					if (curstate & (1<<JP_UP)) {
						hatstate |= SDL_HAT_UP;
					}
					if (curstate & (1<<JP_DOWN)) {
						hatstate |= SDL_HAT_DOWN;
					}
					SDL_PrivateJoystickHat(joystick, 0, hatstate);

					/* Buttons */
					for (i=0;i<JP_NUM_BUTTONS;i++) {
						int button;
						
						button=1<<jp_buttons[i];

						if ((curstate & button) && !(prevstate & button)) {
							SDL_PrivateJoystickButton(joystick,i,SDL_PRESSED);
						}
						if (!(curstate & button) && (prevstate & button)) {
							SDL_PrivateJoystickButton(joystick,i,SDL_RELEASED);
						}
					}
				}
				atarijoysticks[numjoystick].prevstate = curstate;
			}
			break;
		case PORTA_JOY0:
		case PORTA_JOY1:
		case PORTB_JOY0:
		case PORTB_JOY1:
			{
				int fire_shift=0,dir_shift=0;
				
				if (numjoystick==PORTA_JOY0) {	fire_shift=0; dir_shift=0; }
				if (numjoystick==PORTA_JOY1) {	fire_shift=1; dir_shift=4; }
				if (numjoystick==PORTB_JOY0) {	fire_shift=2; dir_shift=8; }
				if (numjoystick==PORTB_JOY1) {	fire_shift=3; dir_shift=12; }

				curstate = (jp_directions>>dir_shift) & 15;
				curstate |= ((jp_fires>>fire_shift) & 1)<<4;

				if (curstate != prevstate) {
					hatstate = SDL_HAT_CENTERED;
					if (curstate & PORT_JS_LEFT) {
						hatstate |= SDL_HAT_LEFT;
					}
					if (curstate & PORT_JS_RIGHT) {
						hatstate |= SDL_HAT_RIGHT;
					}
					if (curstate & PORT_JS_UP) {
						hatstate |= SDL_HAT_UP;
					}
					if (curstate & PORT_JS_DOWN) {
						hatstate |= SDL_HAT_DOWN;
					}
					SDL_PrivateJoystickHat(joystick, 0, hatstate);

					/* Button */
					if ((curstate & PORT_JS_FIRE) && !(prevstate & PORT_JS_FIRE)) {
						SDL_PrivateJoystickButton(joystick,0,SDL_PRESSED);
					}
					if (!(curstate & PORT_JS_FIRE) && (prevstate & PORT_JS_FIRE)) {
						SDL_PrivateJoystickButton(joystick,0,SDL_RELEASED);
					}
				}
				atarijoysticks[numjoystick].prevstate = curstate;
			}
			break;
		case PORTA_LP:
			{
				int i;

				curstate = jp_lightpens[0]>>1;
				curstate |= (jp_lightpens[1]>>1)<<15;
				curstate |= (jp_fires & 3)<<30;

				if (curstate != prevstate) {
					/* X axis */
					SDL_PrivateJoystickAxis(joystick,0,jp_lightpens[0] ^ 0x8000);
					/* Y axis */
					SDL_PrivateJoystickAxis(joystick,1,jp_lightpens[1] ^ 0x8000);
					/* Buttons */
					for (i=0;i<2;i++) {
						int button;
						
						button=1<<(30+i);

						if ((curstate & button) && !(prevstate & button)) {
							SDL_PrivateJoystickButton(joystick,i,SDL_PRESSED);
						}
						if (!(curstate & button) && (prevstate & button)) {
							SDL_PrivateJoystickButton(joystick,i,SDL_RELEASED);
						}
					}
				}
				atarijoysticks[numjoystick].prevstate = curstate;
			}
			break;
		case PORTA_ANPAD:
		case PORTB_ANPAD:
			{
				int numpaddle, i;
				
				numpaddle=0<<1;
				if (numjoystick==PORTB_ANPAD) numpaddle=1<<1;

				curstate = jp_paddles[numpaddle]>>1;
				curstate |= (jp_paddles[numpaddle+1]>>1)<<15;
				curstate |= ((jp_fires>>numpaddle) & 3)<<30;

				if (curstate != prevstate) {
					/* X axis */
					SDL_PrivateJoystickAxis(joystick,0,jp_paddles[numpaddle] ^ 0x8000);
					/* Y axis */
					SDL_PrivateJoystickAxis(joystick,1,jp_paddles[numpaddle+1] ^ 0x8000);
					/* Buttons */
					for (i=0;i<2;i++) {
						int button;
						
						button=1<<(30+i);

						if ((curstate & button) && !(prevstate & button)) {
							SDL_PrivateJoystickButton(joystick,i,SDL_PRESSED);
						}
						if (!(curstate & button) && (prevstate & button)) {
							SDL_PrivateJoystickButton(joystick,i,SDL_RELEASED);
						}
					}
				}
				atarijoysticks[numjoystick].prevstate = curstate;
			}
			break;
#if 0
		case PARA_JOY0:
		case PARA_JOY1:
			break;
#endif
	};

	return;
}

void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	return;
}

void SDL_SYS_JoystickQuit(void)
{
	SDL_numjoysticks=0;
	return;
}

/*--- Joypad I/O read/write interface ---*/

#define JOYPAD_IO_BASE (0xffff9200)
struct JOYPAD_IO_S {
	Uint16 fires;
	Uint16 directions;
	Uint16 dummy1[6];
	Uint16 paddles[4];
	Uint16 dummy2[4];
	Uint16 lightpens[2];
};
#define JOYPAD_IO ((*(volatile struct JOYPAD_IO_S *)JOYPAD_IO_BASE))

static void UpdateJoypads(void)
{
	Uint16 tmp;

	/*--- This function is called in supervisor mode ---*/

	/* Update joysticks */
	jp_fires = (~(JOYPAD_IO.fires)) & 15;
	jp_directions = (~(JOYPAD_IO.directions));
	
	/* Update lightpen */
	tmp = JOYPAD_IO.lightpens[0] & 1023;
	jp_lightpens[0] = (tmp<<6) | (tmp>>4);
	tmp = JOYPAD_IO.lightpens[1] & 1023;
	jp_lightpens[1] = (tmp<<6) | (tmp>>4);
	
	/* Update paddles */
	tmp = (JOYPAD_IO.paddles[0] & 255);
	jp_paddles[0] = (tmp<<8) | tmp;
	tmp = (JOYPAD_IO.paddles[1] & 255);
	jp_paddles[1] = (tmp<<8) | tmp;
	tmp = (JOYPAD_IO.paddles[2] & 255);
	jp_paddles[2] = (tmp<<8) | tmp;
	tmp = (JOYPAD_IO.paddles[3] & 255);
	jp_paddles[3] = (tmp<<8) | tmp;

	/* Update joypad 0 */	
	JOYPAD_IO.directions=0xfffe;
	jp_joypads[0]=((~(JOYPAD_IO.fires)) & 3)<<(16);
	JOYPAD_IO.directions=0xfffe;
	jp_joypads[0] |= ((~(JOYPAD_IO.directions))>>8) & 15;

	JOYPAD_IO.directions=0xfffd;
	jp_joypads[0] |= ((~(JOYPAD_IO.fires)) & 3)<<(16+2);
	JOYPAD_IO.directions=0xfffd;
	jp_joypads[0] |= (((~(JOYPAD_IO.directions))>>8) & 15)<<4;

	JOYPAD_IO.directions=0xfffb;
	jp_joypads[0] |= ((~(JOYPAD_IO.fires)) & 3)<<(16+4);
	JOYPAD_IO.directions=0xfffb;
	jp_joypads[0] |= (((~(JOYPAD_IO.directions))>>8) & 15)<<8;

	JOYPAD_IO.directions=0xfff7;
	jp_joypads[0] |= ((~(JOYPAD_IO.fires)) & 3)<<(16+6);
	JOYPAD_IO.directions=0xfff7;
	jp_joypads[0] |= (((~(JOYPAD_IO.directions))>>8) & 15)<<12;

	/* Update joypad 1 */	
	JOYPAD_IO.directions=0xffef;
	jp_joypads[1]=((~(JOYPAD_IO.fires)) & (3<<2))<<(16-2);
	JOYPAD_IO.directions=0xffef;
	jp_joypads[1] |= ((~(JOYPAD_IO.directions))>>12) & 15;

	JOYPAD_IO.directions=0xffdf;
	jp_joypads[1] |= ((~(JOYPAD_IO.fires)) & (3<<2))<<(16);
	JOYPAD_IO.directions=0xffdf;
	jp_joypads[1] |= (((~(JOYPAD_IO.directions))>>12) & 15)<<4;

	JOYPAD_IO.directions=0xffbf;
	jp_joypads[1] |= ((~(JOYPAD_IO.fires)) & (3<<2))<<(16+2);
	JOYPAD_IO.directions=0xffbf;
	jp_joypads[1] |= (((~(JOYPAD_IO.directions))>>12) & 15)<<8;

	JOYPAD_IO.directions=0xff7f;
	jp_joypads[1] |= ((~(JOYPAD_IO.fires)) & (3<<2))<<(16+4);
	JOYPAD_IO.directions=0xff7f;
	jp_joypads[1] |= (((~(JOYPAD_IO.directions))>>12) & 15)<<12;
}

#endif /* SDL_JOYSTICK_MINT */
