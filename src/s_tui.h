/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
 * Inspired by the s_gui.h by Chris Laurel <claurel@shatters.net>
 * in his Open Source Software Celestia
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

#ifndef _TUI_H_
#define _TUI_H_

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "SDL.h" // Just for the key codes, i'm lasy to redefine them
		 // This is TODO to make the s_ library independent

#include <list>
#include <string>
#include <iostream>

#include "vecmath.h"
#include "callbacks.hpp"

using namespace std;
using namespace boost;


namespace s_tui
{
	// tui Return Values:
	enum S_TUI_VALUE
	{
		S_TUI_PRESSED,
		S_TUI_RELEASED
	};

	typedef Vec4f s_color;

    class Component
    {
    public:
		virtual ~Component() {;}
		virtual string getString(void) const {return string();}
		// Return true if key signal intercepted, false if not
		virtual bool onKey(SDLKey, S_TUI_VALUE) {return false;}
		virtual bool isEditable(void) const {return false;}
    protected:
    };

    class CallbackComponent : public Component
    {
    public:
        virtual void set_OnChangeCallback(const callback<void>& c) {onChangeCallback = c;}
    protected:
		callback<void> onChangeCallback;
	};

    class Container : public CallbackComponent
    {
    public:
        virtual ~Container();
		virtual string getString(void) const;
        virtual void addComponent(Component*);
		virtual bool onKey(SDLKey, S_TUI_VALUE);
    protected:
        list<Component*> childs;
    };

	class Bistate : public CallbackComponent
    {
    public:
		Bistate(bool init_state = false) : state(init_state) {;}
		virtual string getString(void) const {return state ? string_activated : string_disabled;}
		bool getState(void) const {return state;}
		void setState(bool s) {state = s;}
    protected:
		string string_activated;
		string string_disabled;
		bool state;
    };

	class Boolean_item : public Bistate
    {
    public:
		Boolean_item(bool init_state = false, const string& _label = string(),
			const string& _string_activated = string("ON"),
			const string& string_disabled  = string("OFF"));
		virtual bool onKey(SDLKey, S_TUI_VALUE);
		virtual string getString(void) const {return label + (state ? string_activated : string_disabled);}
		virtual bool isEditable(void) const {return true;}
    protected:
		string label;
    };

	class Decimal : public CallbackComponent
    {
    public:
		Decimal(double init_value = 0) : value(init_value) {;}
		virtual string getString(void) const {return string();}
		double getValue(void) const {return value;}
		void setValue(double v) {value = v;}
    protected:
		double value;
    };

    class Label : public Component
    {
    public:
        Label(const string& _label) : label(_label) {;}
		virtual string getString(void) const {return label;}
		void setLabel(const string& s) {label=s;}
    protected:
        string label;
    };

    class Branch : public Container
    {
    public:
		Branch();
		virtual string getString(void) const;
		virtual bool onKey(SDLKey, S_TUI_VALUE);
        virtual void addComponent(Component*);
    protected:
        list<Component*>::iterator current;
    };

    class MenuBranch : public Branch
    {
    public:
		MenuBranch(const string& s);
		virtual bool onKey(SDLKey, S_TUI_VALUE);
		virtual string getString(void) const;
    protected:
		string label;
		bool isNavigating;
		bool isEditing;
    };

}; // namespace s_tui

#endif // _TUI_H_
