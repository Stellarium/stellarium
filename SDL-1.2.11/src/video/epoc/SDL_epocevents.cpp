/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/*
    SDL_epocevents.cpp
    Handle the event stream, converting Epoc events into SDL events

    Epoc version by Hannu Viitala (hannu.j.viitala@mbnet.fi)
*/


#include <stdio.h>
#undef NULL

extern "C" {
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_keysym.h"
#include "SDL_keyboard.h"
#include "SDL_timer.h"
#include "../../events/SDL_events_c.h"
}; /* extern "C" */

#include "SDL_epocvideo.h"
#include "SDL_epocevents_c.h"

#include <hal.h>

extern "C" {
/* The translation tables from a console scancode to a SDL keysym */
static SDLKey keymap[MAX_SCANCODE];
static SDL_keysym *TranslateKey(int scancode, SDL_keysym *keysym);
}; /* extern "C" */

TBool isCursorVisible = ETrue;

int EPOC_HandleWsEvent(_THIS, const TWsEvent& aWsEvent)
{
    int posted = 0;
    SDL_keysym keysym;


    switch (aWsEvent.Type()) {
    
    case EEventPointer: /* Mouse pointer events */
    {
        const TPointerEvent* pointerEvent = aWsEvent.Pointer();
        TPoint mousePos = pointerEvent->iPosition;

        //SDL_TRACE1("SDL: EPOC_HandleWsEvent, pointerEvent->iType=%d", pointerEvent->iType); //!!

        if (Private->EPOC_ShrinkedHeight) {
            mousePos.iY <<= 1; /* Scale y coordinate to shrinked screen height */
        }
		posted += SDL_PrivateMouseMotion(0, 0, mousePos.iX, mousePos.iY); /* Absolute position on screen */
        if (pointerEvent->iType==TPointerEvent::EButton1Down) {
            posted += SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_LEFT, 0, 0);
        }
        else if (pointerEvent->iType==TPointerEvent::EButton1Up) {
			posted += SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_LEFT, 0, 0);
        }
        else if (pointerEvent->iType==TPointerEvent::EButton2Down) {
            posted += SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_RIGHT, 0, 0);
        }
        else if (pointerEvent->iType==TPointerEvent::EButton2Up) {
			posted += SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_RIGHT, 0, 0);
        }
	    //!!posted += SDL_PrivateKeyboard(SDL_PRESSED, TranslateKey(aWsEvent.Key()->iScanCode, &keysym));
        break;
    }
    
    case EEventKeyDown: /* Key events */
    {
       (void*)TranslateKey(aWsEvent.Key()->iScanCode, &keysym);
        
        /* Special handling */
        switch((int)keysym.sym) {
        case SDLK_CAPSLOCK:
            if (!isCursorVisible) {
                /* Enable virtual cursor */
	            HAL::Set(HAL::EMouseState, HAL::EMouseState_Visible);
            }
            else {
                /* Disable virtual cursor */
                HAL::Set(HAL::EMouseState, HAL::EMouseState_Invisible);
            }
            isCursorVisible = !isCursorVisible;
            break;
        }
        
	    posted += SDL_PrivateKeyboard(SDL_PRESSED, &keysym);
        break;
	} 

    case EEventKeyUp: /* Key events */
    {
	    posted += SDL_PrivateKeyboard(SDL_RELEASED, TranslateKey(aWsEvent.Key()->iScanCode, &keysym));
        break;
	} 
    
    case EEventFocusGained: /* SDL window got focus */
    {
        //Private->EPOC_IsWindowFocused = ETrue;
        /* Draw window background and screen buffer */
        RedrawWindowL(_this);  
        break;
    }

    case EEventFocusLost: /* SDL window lost focus */
    {
        //Private->EPOC_IsWindowFocused = EFalse;

        // Wait and eat events until focus is gained again
        /*
	    while (ETrue) {
            Private->EPOC_WsSession.EventReady(&Private->EPOC_WsEventStatus);
            User::WaitForRequest(Private->EPOC_WsEventStatus);
		    Private->EPOC_WsSession.GetEvent(Private->EPOC_WsEvent);
            TInt eventType = Private->EPOC_WsEvent.Type();
		    Private->EPOC_WsEventStatus = KRequestPending;
		    //Private->EPOC_WsSession.EventReady(&Private->EPOC_WsEventStatus);
            if (eventType == EEventFocusGained) {
                RedrawWindowL(_this);
                break;
            }
	    }
        */
        break;
    }

    case EEventModifiersChanged: 
    {
	    TModifiersChangedEvent* modEvent = aWsEvent.ModifiersChanged();
        TUint modstate = KMOD_NONE;
        if (modEvent->iModifiers == EModifierLeftShift)
            modstate |= KMOD_LSHIFT;
        if (modEvent->iModifiers == EModifierRightShift)
            modstate |= KMOD_RSHIFT;
        if (modEvent->iModifiers == EModifierLeftCtrl)
            modstate |= KMOD_LCTRL;
        if (modEvent->iModifiers == EModifierRightCtrl)
            modstate |= KMOD_RCTRL;
        if (modEvent->iModifiers == EModifierLeftAlt)
            modstate |= KMOD_LALT;
        if (modEvent->iModifiers == EModifierRightAlt)
            modstate |= KMOD_RALT;
        if (modEvent->iModifiers == EModifierLeftFunc)
            modstate |= KMOD_LMETA;
        if (modEvent->iModifiers == EModifierRightFunc)
            modstate |= KMOD_RMETA;
        if (modEvent->iModifiers == EModifierCapsLock)
            modstate |= KMOD_CAPS;
        SDL_SetModState(STATIC_CAST(SDLMod,(modstate | KMOD_LSHIFT)));
        break;
    }
    default:            
        break;
	} 
	
    return posted;
}

extern "C" {

void EPOC_PumpEvents(_THIS)
{
    int posted = 0; // !! Do we need this?
    //Private->EPOC_WsSession.EventReady(&Private->EPOC_WsEventStatus);
	while (Private->EPOC_WsEventStatus != KRequestPending) {

		Private->EPOC_WsSession.GetEvent(Private->EPOC_WsEvent);
		posted = EPOC_HandleWsEvent(_this, Private->EPOC_WsEvent);
		Private->EPOC_WsEventStatus = KRequestPending;
		Private->EPOC_WsSession.EventReady(&Private->EPOC_WsEventStatus);
	}
}


void EPOC_InitOSKeymap(_THIS)
{
	int i;

	/* Initialize the key translation table */
	for ( i=0; i<SDL_TABLESIZE(keymap); ++i )
		keymap[i] = SDLK_UNKNOWN;


	/* Numbers */
	for ( i = 0; i<32; ++i ){
		keymap[' ' + i] = (SDLKey)(SDLK_SPACE+i);
	}
	/* e.g. Alphabet keys */
	for ( i = 0; i<32; ++i ){
		keymap['A' + i] = (SDLKey)(SDLK_a+i);
	}

	keymap[EStdKeyBackspace]    = SDLK_BACKSPACE;
	keymap[EStdKeyTab]          = SDLK_TAB;
	keymap[EStdKeyEnter]        = SDLK_RETURN;
	keymap[EStdKeyEscape]       = SDLK_ESCAPE;
   	keymap[EStdKeySpace]        = SDLK_SPACE;
   	keymap[EStdKeyPause]        = SDLK_PAUSE;
   	keymap[EStdKeyHome]         = SDLK_HOME;
   	keymap[EStdKeyEnd]          = SDLK_END;
   	keymap[EStdKeyPageUp]       = SDLK_PAGEUP;
   	keymap[EStdKeyPageDown]     = SDLK_PAGEDOWN;
	keymap[EStdKeyDelete]       = SDLK_DELETE;
	keymap[EStdKeyUpArrow]      = SDLK_UP;
	keymap[EStdKeyDownArrow]    = SDLK_DOWN;
	keymap[EStdKeyLeftArrow]    = SDLK_LEFT;
	keymap[EStdKeyRightArrow]   = SDLK_RIGHT;
	keymap[EStdKeyCapsLock]     = SDLK_CAPSLOCK;
	keymap[EStdKeyLeftShift]    = SDLK_LSHIFT;
	keymap[EStdKeyRightShift]   = SDLK_RSHIFT;
	keymap[EStdKeyLeftAlt]      = SDLK_LALT;
	keymap[EStdKeyRightAlt]     = SDLK_RALT;
	keymap[EStdKeyLeftCtrl]     = SDLK_LCTRL;
	keymap[EStdKeyRightCtrl]    = SDLK_RCTRL;
	keymap[EStdKeyLeftFunc]     = SDLK_LMETA;
	keymap[EStdKeyRightFunc]    = SDLK_RMETA;
	keymap[EStdKeyInsert]       = SDLK_INSERT;
	keymap[EStdKeyComma]        = SDLK_COMMA;
	keymap[EStdKeyFullStop]     = SDLK_PERIOD;
	keymap[EStdKeyForwardSlash] = SDLK_SLASH;
	keymap[EStdKeyBackSlash]    = SDLK_BACKSLASH;
	keymap[EStdKeySemiColon]    = SDLK_SEMICOLON;
	keymap[EStdKeySingleQuote]  = SDLK_QUOTE;
	keymap[EStdKeyHash]         = SDLK_HASH;
	keymap[EStdKeySquareBracketLeft]    = SDLK_LEFTBRACKET;
	keymap[EStdKeySquareBracketRight]   = SDLK_RIGHTBRACKET;
	keymap[EStdKeyMinus]        = SDLK_MINUS;
	keymap[EStdKeyEquals]       = SDLK_EQUALS;

   	keymap[EStdKeyF1]          = SDLK_F1;  /* chr + q */
   	keymap[EStdKeyF2]          = SDLK_F2;  /* chr + w */
   	keymap[EStdKeyF3]          = SDLK_F3;  /* chr + e */
   	keymap[EStdKeyF4]          = SDLK_F4;  /* chr + r */
   	keymap[EStdKeyF5]          = SDLK_F5;  /* chr + t */
   	keymap[EStdKeyF6]          = SDLK_F6;  /* chr + y */
   	keymap[EStdKeyF7]          = SDLK_F7;  /* chr + i */
   	keymap[EStdKeyF8]          = SDLK_F8;  /* chr + o */

   	keymap[EStdKeyF9]          = SDLK_F9;  /* chr + a */
   	keymap[EStdKeyF10]         = SDLK_F10; /* chr + s */
   	keymap[EStdKeyF11]         = SDLK_F11; /* chr + d */
   	keymap[EStdKeyF12]         = SDLK_F12; /* chr + f */

    /* !!TODO
	EStdKeyNumLock=0x1b,
	EStdKeyScrollLock=0x1c,

	EStdKeyNkpForwardSlash=0x84,
	EStdKeyNkpAsterisk=0x85,
	EStdKeyNkpMinus=0x86,
	EStdKeyNkpPlus=0x87,
	EStdKeyNkpEnter=0x88,
	EStdKeyNkp1=0x89,
	EStdKeyNkp2=0x8a,
	EStdKeyNkp3=0x8b,
	EStdKeyNkp4=0x8c,
	EStdKeyNkp5=0x8d,
	EStdKeyNkp6=0x8e,
	EStdKeyNkp7=0x8f,
	EStdKeyNkp8=0x90,
	EStdKeyNkp9=0x91,
	EStdKeyNkp0=0x92,
	EStdKeyNkpFullStop=0x93,
    EStdKeyMenu=0x94,
    EStdKeyBacklightOn=0x95,
    EStdKeyBacklightOff=0x96,
    EStdKeyBacklightToggle=0x97,
    EStdKeyIncContrast=0x98,
    EStdKeyDecContrast=0x99,
    EStdKeySliderDown=0x9a,
    EStdKeySliderUp=0x9b,
    EStdKeyDictaphonePlay=0x9c,
    EStdKeyDictaphoneStop=0x9d,
    EStdKeyDictaphoneRecord=0x9e,
    EStdKeyHelp=0x9f,
    EStdKeyOff=0xa0,
    EStdKeyDial=0xa1,
    EStdKeyIncVolume=0xa2,
    EStdKeyDecVolume=0xa3,
    EStdKeyDevice0=0xa4,
    EStdKeyDevice1=0xa5,
    EStdKeyDevice2=0xa6,
    EStdKeyDevice3=0xa7,
    EStdKeyDevice4=0xa8,
    EStdKeyDevice5=0xa9,
    EStdKeyDevice6=0xaa,
    EStdKeyDevice7=0xab,
    EStdKeyDevice8=0xac,
    EStdKeyDevice9=0xad,
    EStdKeyDeviceA=0xae,
    EStdKeyDeviceB=0xaf,
    EStdKeyDeviceC=0xb0,
    EStdKeyDeviceD=0xb1,
    EStdKeyDeviceE=0xb2,
    EStdKeyDeviceF=0xb3,
    EStdKeyApplication0=0xb4,
    EStdKeyApplication1=0xb5,
    EStdKeyApplication2=0xb6,
    EStdKeyApplication3=0xb7,
    EStdKeyApplication4=0xb8,
    EStdKeyApplication5=0xb9,
    EStdKeyApplication6=0xba,
    EStdKeyApplication7=0xbb,
    EStdKeyApplication8=0xbc,
    EStdKeyApplication9=0xbd,
    EStdKeyApplicationA=0xbe,
    EStdKeyApplicationB=0xbf,
    EStdKeyApplicationC=0xc0,
    EStdKeyApplicationD=0xc1,
    EStdKeyApplicationE=0xc2,
    EStdKeyApplicationF=0xc3,
    EStdKeyYes=0xc4,
    EStdKeyNo=0xc5,
    EStdKeyIncBrightness=0xc6,
    EStdKeyDecBrightness=0xc7, 
    EStdKeyCaseOpen=0xc8,
    EStdKeyCaseClose=0xc9
    */

}



static SDL_keysym *TranslateKey(int scancode, SDL_keysym *keysym)
{
    char debug[256];
    //SDL_TRACE1("SDL: TranslateKey, scancode=%d", scancode); //!!

	/* Set the keysym information */ 

	keysym->scancode = scancode;

    if ((scancode >= MAX_SCANCODE) && 
        ((scancode - ENonCharacterKeyBase + 0x0081) >= MAX_SCANCODE)) {
        SDL_SetError("Too big scancode");
        keysym->scancode = SDLK_UNKNOWN;
	    keysym->mod = KMOD_NONE; 
        return keysym;
    }

	keysym->mod = SDL_GetModState(); //!!Is this right??

    /* Handle function keys: F1, F2, F3 ... */
    if (keysym->mod & KMOD_META) {
        if (scancode >= 'A' && scancode < ('A' + 24)) { /* first 32 alphapet keys */
            switch(scancode) {
                case 'Q': scancode = EStdKeyF1; break;
                case 'W': scancode = EStdKeyF2; break;
                case 'E': scancode = EStdKeyF3; break;
                case 'R': scancode = EStdKeyF4; break;
                case 'T': scancode = EStdKeyF5; break;
                case 'Y': scancode = EStdKeyF6; break;
                case 'U': scancode = EStdKeyF7; break;
                case 'I': scancode = EStdKeyF8; break;
                case 'A': scancode = EStdKeyF9; break;
                case 'S': scancode = EStdKeyF10; break;
                case 'D': scancode = EStdKeyF11; break;
                case 'F': scancode = EStdKeyF12; break;
            }
            keysym->sym = keymap[scancode];
        }
    }

    if (scancode >= ENonCharacterKeyBase) {
        // Non character keys
	    keysym->sym = keymap[scancode - 
            ENonCharacterKeyBase + 0x0081]; // !!hard coded
    } else {
	    keysym->sym = keymap[scancode];
    }


	/* If UNICODE is on, get the UNICODE value for the key */
	keysym->unicode = 0;

#if 0 // !!TODO:unicode

	if ( SDL_TranslateUNICODE ) 
    {
		/* Populate the unicode field with the ASCII value */
		keysym->unicode = scancode;
	}
#endif

    //!!
    //sprintf(debug, "SDL: TranslateKey: keysym->scancode=%d, keysym->sym=%d, keysym->mod=%d",
    //    keysym->scancode, keysym->sym, keysym->mod);
    //SDL_TRACE(debug); //!!

	return(keysym);
}

}; /* extern "C" */


