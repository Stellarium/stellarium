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

#ifndef _STEL_UI_H
#define _STEL_UI_H

#include "stellarium.h"

class stel_ui
{
public:
	stel_ui();			// Create and initialize a stellarium ui.
    virtual ~stel_ui();	// Delete the ui

	void renderUi();	// Display the ui

	// TODO : for the following functions, get rid of the SDL macro def and types
	// need the creation of an interface between s_gui and SDL

	// Handle mouse clics
	void GuiHandleClic(Uint16 x, Uint16 y, Uint8 state, Uint8 button);

	// Handle mouse move
	void GuiHandleMove(int x, int y);

	// Handle key press and release
	bool GuiHandleKeys(SDLKey key, int state);

	// Handle mouse movements
	void GuiHandleMove(Uint16 x, Uint16 y);
private:

};

#endif  //_STEL_UI_H
