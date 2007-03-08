#ifndef STELKEY_HPP_
#define STELKEY_HPP_

#include "stellarium.h"

// Directly and shamlessly copied from SDL_keysym.h

typedef enum {
	/* The keyboard syms have been cleverly chosen to map to ASCII */
	StelKey_UNKNOWN		= 0,
	StelKey_FIRST		= 0,
	StelKey_BACKSPACE		= 8,
	StelKey_TAB		= 9,
	StelKey_CLEAR		= 12,
	StelKey_RETURN		= 13,
	StelKey_PAUSE		= 19,
	StelKey_ESCAPE		= 27,
	StelKey_SPACE		= 32,
	StelKey_EXCLAIM		= 33,
	StelKey_QUOTEDBL		= 34,
	StelKey_HASH		= 35,
	StelKey_DOLLAR		= 36,
	StelKey_AMPERSAND		= 38,
	StelKey_QUOTE		= 39,
	StelKey_LEFTPAREN		= 40,
	StelKey_RIGHTPAREN		= 41,
	StelKey_ASTERISK		= 42,
	StelKey_PLUS		= 43,
	StelKey_COMMA		= 44,
	StelKey_MINUS		= 45,
	StelKey_PERIOD		= 46,
	StelKey_SLASH		= 47,
	StelKey_0			= 48,
	StelKey_1			= 49,
	StelKey_2			= 50,
	StelKey_3			= 51,
	StelKey_4			= 52,
	StelKey_5			= 53,
	StelKey_6			= 54,
	StelKey_7			= 55,
	StelKey_8			= 56,
	StelKey_9			= 57,
	StelKey_COLON		= 58,
	StelKey_SEMICOLON		= 59,
	StelKey_LESS		= 60,
	StelKey_EQUALS		= 61,
	StelKey_GREATER		= 62,
	StelKey_QUESTION		= 63,
	StelKey_AT			= 64,
	/* 
	   Skip uppercase letters
	 */
	StelKey_LEFTBRACKET	= 91,
	StelKey_BACKSLASH		= 92,
	StelKey_RIGHTBRACKET	= 93,
	StelKey_CARET		= 94,
	StelKey_UNDERSCORE		= 95,
	StelKey_BACKQUOTE		= 96,
	StelKey_a			= 97,
	StelKey_b			= 98,
	StelKey_c			= 99,
	StelKey_d			= 100,
	StelKey_e			= 101,
	StelKey_f			= 102,
	StelKey_g			= 103,
	StelKey_h			= 104,
	StelKey_i			= 105,
	StelKey_j			= 106,
	StelKey_k			= 107,
	StelKey_l			= 108,
	StelKey_m			= 109,
	StelKey_n			= 110,
	StelKey_o			= 111,
	StelKey_p			= 112,
	StelKey_q			= 113,
	StelKey_r			= 114,
	StelKey_s			= 115,
	StelKey_t			= 116,
	StelKey_u			= 117,
	StelKey_v			= 118,
	StelKey_w			= 119,
	StelKey_x			= 120,
	StelKey_y			= 121,
	StelKey_z			= 122,
	StelKey_DELETE		= 127,
	/* End of ASCII mapped keysyms */

	/* International keyboard syms */
	StelKey_WORLD_0		= 160,		/* 0xA0 */
	StelKey_WORLD_1		= 161,
	StelKey_WORLD_2		= 162,
	StelKey_WORLD_3		= 163,
	StelKey_WORLD_4		= 164,
	StelKey_WORLD_5		= 165,
	StelKey_WORLD_6		= 166,
	StelKey_WORLD_7		= 167,
	StelKey_WORLD_8		= 168,
	StelKey_WORLD_9		= 169,
	StelKey_WORLD_10		= 170,
	StelKey_WORLD_11		= 171,
	StelKey_WORLD_12		= 172,
	StelKey_WORLD_13		= 173,
	StelKey_WORLD_14		= 174,
	StelKey_WORLD_15		= 175,
	StelKey_WORLD_16		= 176,
	StelKey_WORLD_17		= 177,
	StelKey_WORLD_18		= 178,
	StelKey_WORLD_19		= 179,
	StelKey_WORLD_20		= 180,
	StelKey_WORLD_21		= 181,
	StelKey_WORLD_22		= 182,
	StelKey_WORLD_23		= 183,
	StelKey_WORLD_24		= 184,
	StelKey_WORLD_25		= 185,
	StelKey_WORLD_26		= 186,
	StelKey_WORLD_27		= 187,
	StelKey_WORLD_28		= 188,
	StelKey_WORLD_29		= 189,
	StelKey_WORLD_30		= 190,
	StelKey_WORLD_31		= 191,
	StelKey_WORLD_32		= 192,
	StelKey_WORLD_33		= 193,
	StelKey_WORLD_34		= 194,
	StelKey_WORLD_35		= 195,
	StelKey_WORLD_36		= 196,
	StelKey_WORLD_37		= 197,
	StelKey_WORLD_38		= 198,
	StelKey_WORLD_39		= 199,
	StelKey_WORLD_40		= 200,
	StelKey_WORLD_41		= 201,
	StelKey_WORLD_42		= 202,
	StelKey_WORLD_43		= 203,
	StelKey_WORLD_44		= 204,
	StelKey_WORLD_45		= 205,
	StelKey_WORLD_46		= 206,
	StelKey_WORLD_47		= 207,
	StelKey_WORLD_48		= 208,
	StelKey_WORLD_49		= 209,
	StelKey_WORLD_50		= 210,
	StelKey_WORLD_51		= 211,
	StelKey_WORLD_52		= 212,
	StelKey_WORLD_53		= 213,
	StelKey_WORLD_54		= 214,
	StelKey_WORLD_55		= 215,
	StelKey_WORLD_56		= 216,
	StelKey_WORLD_57		= 217,
	StelKey_WORLD_58		= 218,
	StelKey_WORLD_59		= 219,
	StelKey_WORLD_60		= 220,
	StelKey_WORLD_61		= 221,
	StelKey_WORLD_62		= 222,
	StelKey_WORLD_63		= 223,
	StelKey_WORLD_64		= 224,
	StelKey_WORLD_65		= 225,
	StelKey_WORLD_66		= 226,
	StelKey_WORLD_67		= 227,
	StelKey_WORLD_68		= 228,
	StelKey_WORLD_69		= 229,
	StelKey_WORLD_70		= 230,
	StelKey_WORLD_71		= 231,
	StelKey_WORLD_72		= 232,
	StelKey_WORLD_73		= 233,
	StelKey_WORLD_74		= 234,
	StelKey_WORLD_75		= 235,
	StelKey_WORLD_76		= 236,
	StelKey_WORLD_77		= 237,
	StelKey_WORLD_78		= 238,
	StelKey_WORLD_79		= 239,
	StelKey_WORLD_80		= 240,
	StelKey_WORLD_81		= 241,
	StelKey_WORLD_82		= 242,
	StelKey_WORLD_83		= 243,
	StelKey_WORLD_84		= 244,
	StelKey_WORLD_85		= 245,
	StelKey_WORLD_86		= 246,
	StelKey_WORLD_87		= 247,
	StelKey_WORLD_88		= 248,
	StelKey_WORLD_89		= 249,
	StelKey_WORLD_90		= 250,
	StelKey_WORLD_91		= 251,
	StelKey_WORLD_92		= 252,
	StelKey_WORLD_93		= 253,
	StelKey_WORLD_94		= 254,
	StelKey_WORLD_95		= 255,		/* 0xFF */

	/* Numeric keypad */
	StelKey_KP0		= 256,
	StelKey_KP1		= 257,
	StelKey_KP2		= 258,
	StelKey_KP3		= 259,
	StelKey_KP4		= 260,
	StelKey_KP5		= 261,
	StelKey_KP6		= 262,
	StelKey_KP7		= 263,
	StelKey_KP8		= 264,
	StelKey_KP9		= 265,
	StelKey_KP_PERIOD		= 266,
	StelKey_KP_DIVIDE		= 267,
	StelKey_KP_MULTIPLY	= 268,
	StelKey_KP_MINUS		= 269,
	StelKey_KP_PLUS		= 270,
	StelKey_KP_ENTER		= 271,
	StelKey_KP_EQUALS		= 272,

	/* Arrows + Home/End pad */
	StelKey_UP			= 273,
	StelKey_DOWN		= 274,
	StelKey_RIGHT		= 275,
	StelKey_LEFT		= 276,
	StelKey_INSERT		= 277,
	StelKey_HOME		= 278,
	StelKey_END		= 279,
	StelKey_PAGEUP		= 280,
	StelKey_PAGEDOWN		= 281,

	/* Function keys */
	StelKey_F1			= 282,
	StelKey_F2			= 283,
	StelKey_F3			= 284,
	StelKey_F4			= 285,
	StelKey_F5			= 286,
	StelKey_F6			= 287,
	StelKey_F7			= 288,
	StelKey_F8			= 289,
	StelKey_F9			= 290,
	StelKey_F10		= 291,
	StelKey_F11		= 292,
	StelKey_F12		= 293,
	StelKey_F13		= 294,
	StelKey_F14		= 295,
	StelKey_F15		= 296,

	/* Key state modifier keys */
	StelKey_NUMLOCK		= 300,
	StelKey_CAPSLOCK		= 301,
	StelKey_SCROLLOCK		= 302,
	StelKey_RSHIFT		= 303,
	StelKey_LSHIFT		= 304,
	StelKey_RCTRL		= 305,
	StelKey_LCTRL		= 306,
	StelKey_RALT		= 307,
	StelKey_LALT		= 308,
	StelKey_RMETA		= 309,
	StelKey_LMETA		= 310,
	StelKey_LSUPER		= 311,		/* Left "Windows" key */
	StelKey_RSUPER		= 312,		/* Right "Windows" key */
	StelKey_MODE		= 313,		/* "Alt Gr" key */
	StelKey_COMPOSE		= 314,		/* Multi-key compose key */

	/* Miscellaneous function keys */
	StelKey_HELP		= 315,
	StelKey_PRINT		= 316,
	StelKey_SYSREQ		= 317,
	StelKey_BREAK		= 318,
	StelKey_MENU		= 319,
	StelKey_POWER		= 320,		/* Power Macintosh power key */
	StelKey_EURO		= 321,		/* Some european keyboards */
	StelKey_UNDO		= 322,		/* Atari keyboard has Undo */

	/* Add any other keys here */

	StelKey_LAST
} StelKey;

/* Enumeration of valid key mods (possibly OR'd together) */
typedef enum {
	StelMod_NONE  = 0x0000,
	StelMod_LSHIFT= 0x0001,
	StelMod_RSHIFT= 0x0002,
	StelMod_LCTRL = 0x0040,
	StelMod_RCTRL = 0x0080,
	StelMod_LALT  = 0x0100,
	StelMod_RALT  = 0x0200,
	StelMod_LMETA = 0x0400,
	StelMod_RMETA = 0x0800,
	StelMod_NUM   = 0x1000,
	StelMod_CAPS  = 0x2000,
	StelMod_MODE  = 0x4000,
	StelMod_RESERVED = 0x8000
} StelMod;

#define StelMod_CTRL	((StelMod)(StelMod_LCTRL|StelMod_RCTRL))
#define StelMod_SHIFT	((StelMod)(StelMod_LSHIFT|StelMod_RSHIFT))
#define StelMod_ALT	((StelMod)(StelMod_LALT|StelMod_RALT))
#define StelMod_META	((StelMod)(StelMod_LMETA|StelMod_RMETA))

#define Stel_BUTTON_LEFT		1
#define Stel_BUTTON_MIDDLE	2
#define Stel_BUTTON_RIGHT	3
#define Stel_BUTTON_WHEELUP	4
#define Stel_BUTTON_WHEELDOWN	5

typedef enum {
Stel_KEYDOWN,
Stel_KEYUP,
Stel_MOUSEBUTTONDOWN,		/* Mouse button pressed */
Stel_MOUSEBUTTONUP		/* Mouse button released */
} StelEvent;

// mac seems to use KMOD_META instead of KMOD_CTRL
#ifdef MACOSX
#define COMPATIBLE_StelMod_CTRL StelMod_META
#else
#define COMPATIBLE_StelMod_CTRL StelMod_CTRL
#endif

#define SDLKeyToStelKey(key) ((StelKey)key)
#define SDLmodToStelMod(mod) ((StelMod)mod)
#define SDLButtonToStelButton(button) (button)

#endif /*STELKEY_HPP_*/
