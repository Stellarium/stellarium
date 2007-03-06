#ifndef STELKEY_HPP_
#define STELKEY_HPP_

#include "stellarium.h"

/*************************************************************************
 *  Map stellarium keys to SDL keys if we use SDL
*************************************************************************/
#ifdef USE_SDL 

#include "SDL.h"

typedef enum {
StelKey_UNKNOWN = SDLK_UNKNOWN,
StelKey_FIRST = SDLK_FIRST,
StelKey_BACKSPACE = SDLK_BACKSPACE,
StelKey_TAB = SDLK_TAB,
StelKey_CLEAR = SDLK_CLEAR,
StelKey_RETURN = SDLK_RETURN,
StelKey_PAUSE = SDLK_PAUSE,
StelKey_ESCAPE = SDLK_ESCAPE,
StelKey_SPACE = SDLK_SPACE,
StelKey_EXCLAIM = SDLK_EXCLAIM,
StelKey_QUOTEDBL = SDLK_QUOTEDBL,
StelKey_HASH = SDLK_HASH,
StelKey_DOLLAR = SDLK_DOLLAR,
StelKey_AMPERSAND = SDLK_AMPERSAND,
StelKey_QUOTE = SDLK_QUOTE,
StelKey_LEFTPAREN = SDLK_LEFTPAREN,
StelKey_RIGHTPAREN = SDLK_RIGHTPAREN,
StelKey_ASTERISK = SDLK_ASTERISK,
StelKey_PLUS = SDLK_PLUS,
StelKey_COMMA = SDLK_COMMA,
StelKey_MINUS = SDLK_MINUS,
StelKey_PERIOD = SDLK_PERIOD,
StelKey_SLASH = SDLK_SLASH,
StelKey_0 = SDLK_0,
StelKey_1 = SDLK_1,
StelKey_2 = SDLK_2,
StelKey_3 = SDLK_3,
StelKey_4 = SDLK_4,
StelKey_5 = SDLK_5,
StelKey_6 = SDLK_6,
StelKey_7 = SDLK_7,
StelKey_8 = SDLK_8,
StelKey_9 = SDLK_9,
StelKey_COLON = SDLK_COLON,
StelKey_SEMICOLON = SDLK_SEMICOLON,
StelKey_LESS = SDLK_LESS,
StelKey_EQUALS = SDLK_EQUALS,
StelKey_GREATER = SDLK_GREATER,
StelKey_QUESTION = SDLK_QUESTION,
StelKey_AT = SDLK_AT,

StelKey_LEFTBRACKET = SDLK_LEFTBRACKET,
StelKey_BACKSLASH = SDLK_BACKSLASH,
StelKey_RIGHTBRACKET = SDLK_RIGHTBRACKET,
StelKey_CARET = SDLK_CARET,
StelKey_UNDERSCORE = SDLK_UNDERSCORE,
StelKey_BACKQUOTE = SDLK_BACKQUOTE,
StelKey_a = SDLK_a,
StelKey_b = SDLK_b,
StelKey_c = SDLK_c,
StelKey_d = SDLK_d,
StelKey_e = SDLK_e,
StelKey_f = SDLK_f,
StelKey_g = SDLK_g,
StelKey_h = SDLK_h,
StelKey_i = SDLK_i,
StelKey_j = SDLK_j,
StelKey_k = SDLK_k,
StelKey_l = SDLK_l,
StelKey_m = SDLK_m,
StelKey_n = SDLK_n,
StelKey_o = SDLK_o,
StelKey_p = SDLK_p,
StelKey_q = SDLK_q,
StelKey_r = SDLK_r,
StelKey_s = SDLK_s,
StelKey_t = SDLK_t,
StelKey_u = SDLK_u,
StelKey_v = SDLK_v,
StelKey_w = SDLK_w,
StelKey_x = SDLK_x,
StelKey_y = SDLK_y,
StelKey_z = SDLK_z,
StelKey_DELETE = SDLK_DELETE,

StelKey_KP0 = SDLK_KP0,
StelKey_KP1 = SDLK_KP1,
StelKey_KP2 = SDLK_KP2,
StelKey_KP3 = SDLK_KP3,
StelKey_KP4 = SDLK_KP4,
StelKey_KP5 = SDLK_KP5,
StelKey_KP6 = SDLK_KP6,
StelKey_KP7 = SDLK_KP7,
StelKey_KP8 = SDLK_KP8,
StelKey_KP9 = SDLK_KP9,
StelKey_KP_PERIOD = SDLK_KP_PERIOD,
StelKey_KP_DIVIDE = SDLK_KP_DIVIDE,
StelKey_KP_MULTIPLY = SDLK_KP_MULTIPLY,
StelKey_KP_MINUS = SDLK_KP_MINUS,
StelKey_KP_PLUS = SDLK_KP_PLUS,
StelKey_KP_ENTER = SDLK_KP_ENTER,
StelKey_KP_EQUALS = SDLK_KP_EQUALS,

StelKey_UP = SDLK_UP,
StelKey_DOWN = SDLK_DOWN,
StelKey_RIGHT = SDLK_RIGHT,
StelKey_LEFT = SDLK_LEFT,
StelKey_INSERT = SDLK_INSERT,
StelKey_HOME = SDLK_HOME,
StelKey_END = SDLK_END,
StelKey_PAGEUP = SDLK_PAGEUP,
StelKey_PAGEDOWN = SDLK_PAGEDOWN,

StelKey_F1 = SDLK_F1,
StelKey_F2 = SDLK_F2,
StelKey_F3 = SDLK_F3,
StelKey_F4 = SDLK_F4,
StelKey_F5 = SDLK_F5,
StelKey_F6 = SDLK_F6,
StelKey_F7 = SDLK_F7,
StelKey_F8 = SDLK_F8,
StelKey_F9 = SDLK_F9,
StelKey_F10 = SDLK_F10,
StelKey_F11 = SDLK_F11,
StelKey_F12 = SDLK_F12,
StelKey_F13 = SDLK_F13,
StelKey_F14 = SDLK_F14,
StelKey_F15 = SDLK_F15,

StelKey_NUMLOCK = SDLK_NUMLOCK,
StelKey_CAPSLOCK = SDLK_CAPSLOCK,
StelKey_SCROLLOCK = SDLK_SCROLLOCK,
StelKey_RSHIFT = SDLK_RSHIFT,
StelKey_LSHIFT = SDLK_LSHIFT,
StelKey_RCTRL = SDLK_RCTRL,
StelKey_LCTRL = SDLK_LCTRL,
StelKey_RALT = SDLK_RALT,
StelKey_LALT = SDLK_LALT,
StelKey_RMETA = SDLK_RMETA,
StelKey_LMETA = SDLK_LMETA,
StelKey_LSUPER = SDLK_LSUPER,
StelKey_RSUPER = SDLK_RSUPER,
StelKey_MODE = SDLK_MODE,
StelKey_COMPOSE = SDLK_COMPOSE,

StelKey_HELP = SDLK_HELP,
StelKey_PRINT = SDLK_PRINT,
StelKey_SYSREQ = SDLK_SYSREQ,
StelKey_BREAK = SDLK_BREAK,
StelKey_MENU = SDLK_MENU,
StelKey_POWER = SDLK_POWER,
StelKey_EURO = SDLK_EURO,
StelKey_UNDO = SDLK_UNDO,

StelKey_LAST = SDLK_LAST,

} StelKey;

/* Enumeration of valid key mods (possibly OR'd together) */
typedef enum {
	StelMod_NONE  = KMOD_NONE,
	StelMod_LSHIFT= KMOD_LSHIFT,
	StelMod_RSHIFT= KMOD_RSHIFT,
	StelMod_LCTRL = KMOD_LCTRL,
	StelMod_RCTRL = KMOD_RCTRL,
	StelMod_LALT  = KMOD_LALT,
	StelMod_RALT  = KMOD_RALT,
	StelMod_LMETA = KMOD_LMETA,
	StelMod_RMETA = KMOD_RMETA,
	StelMod_NUM   = KMOD_NUM,
	StelMod_CAPS  = KMOD_CAPS,
	StelMod_MODE  = KMOD_MODE,
	StelMod_RESERVED = KMOD_RESERVED
} StelMod;

#define StelMod_CTRL KMOD_CTRL
#define StelMod_SHIFT KMOD_SHIFT
#define StelMod_ALT	KMOD_ALT
#define StelMod_META KMOD_META

#define SDLKeyToStelKey(key) ((StelKey)key)
#define SDLKmodToStelMod(mod) ((StelMod)mod)

#define Stel_BUTTON_LEFT SDL_BUTTON_LEFT
#define Stel_BUTTON_MIDDLE SDL_BUTTON_MIDDLE
#define Stel_BUTTON_RIGHT SDL_BUTTON_RIGHT
#define Stel_BUTTON_WHEELUP SDL_BUTTON_WHEELUP
#define Stel_BUTTON_WHEELDOWN SDL_BUTTON_WHEELDOWN

enum {
       Stel_KEYDOWN = SDL_KEYDOWN,			/* Keys pressed */
       Stel_KEYUP = SDL_KEYUP,			/* Keys released */
       Stel_MOUSEMOTION = SDL_MOUSEMOTION,			/* Mouse moved */
       Stel_MOUSEBUTTONDOWN = SDL_MOUSEBUTTONDOWN,		/* Mouse button pressed */
       Stel_MOUSEBUTTONUP = SDL_MOUSEBUTTONUP		/* Mouse button released */
};

#endif // USE_SDL 

// mac seems to use KMOD_META instead of KMOD_CTRL
#ifdef MACOSX
#define COMPATIBLE_StelMod_CTRL StelMod_META
#else
#define COMPATIBLE_StelMod_CTRL StelMod_CTRL
#endif

#endif /*STELKEY_HPP_*/
