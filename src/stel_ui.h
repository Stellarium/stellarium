/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// Class which handles a stellarium User Interface
// TODO : get rid of the SDL macro def and types
// need the creation of an interface between s_gui and SDL

#ifndef _STEL_UI_H
#define _STEL_UI_H

#include "stellarium.h"
#include "stel_core.h"
#include "s_gui.h"

// Predeclaration of the stel_core class
class stel_core;

using namespace std;
using namespace s_gui;

class stel_ui
{
public:
	stel_ui(stel_core *);	// Create a stellarium ui. Need to call init() before use
    virtual ~stel_ui();		// Delete the ui
	void init(void);		// Initialize the ui.
	void draw(void);		// Display the ui

	// Handle mouse clics
	int handle_clic(Uint16 x, Uint16 y, Uint8 state, Uint8 button);
	// Handle mouse move
	int handle_move(int x, int y);
	// Handle key press and release
	int handle_keys(SDLKey key, S_GUI_VALUE state);

private:
	stel_core * core;

	s_font * spaceFont;		// The standard font
	s_texture * baseTex;	// The standard fill texture

	Container * desktop;	// The container which contains everything (=the desktop)

	FramedContainer * containerTest;
	FramedContainer * containerTest2;
	s_texture* testTex;
	Label* testLabel;
	FilledButton * btTest;
	void btTestOnPress(void);
};

#endif  //_STEL_UI_H
