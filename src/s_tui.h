/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chéreau
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

// Class which manages a Text User Interface "widgets"

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
#include <sstream>

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

	static const string start_active("\127");
	static const string stop_active("\126");

    class Component
    {
    public:
		Component() : active(false) {;}
		virtual ~Component() {;}
		virtual string getString(void) {return string();}
		// Return true if key signal intercepted, false if not
		virtual bool onKey(SDLKey, S_TUI_VALUE) {return false;}
		virtual bool isEditable(void) const {return false;}
		void setActive(bool a) {active = a;}
		bool getActive(void) const {return active;}
    protected:
		bool active;
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
		virtual string getString(void);
        virtual void addComponent(Component*);
		virtual bool onKey(SDLKey, S_TUI_VALUE);
    protected:
        list<Component*> childs;
    };

	class Bistate : public CallbackComponent
    {
    public:
		Bistate(bool init_state = false) : CallbackComponent(), state(init_state) {;}
		virtual string getString(void) {return state ? string_activated : string_disabled;}
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
		virtual string getString(void);
		virtual bool isEditable(void) const {return true;}
    protected:
		string label;
    };

	class Integer : public CallbackComponent
    {
    public:
		Integer(int init_value = 0) : CallbackComponent(), value(init_value) {;}
		virtual string getString(void);
		int getValue(void) const {return value;}
		void setValue(int v) {value = v;}
    protected:
		int value;
    };

	class Decimal : public CallbackComponent
    {
    public:
		Decimal(double init_value = 0.) : CallbackComponent(), value(init_value) {;}
		virtual string getString(void);
		double getValue(void) const {return value;}
		void setValue(double v) {value = v;}
    protected:
		double value;
    };

	class Integer_item : public Integer
    {
    public:
		Integer_item(int _min, int _max, int init_value, const string& _label = string()) :
			Integer(init_value), min(_min), max(_max), label(_label) {;}
		virtual string getString(void) {return label + Integer::getString();}
		virtual bool isEditable(void) const {return true;}
		virtual bool onKey(SDLKey, S_TUI_VALUE);
    protected:
		int min, max;
		string label;
    };

	class Decimal_item : public Decimal
    {
    public:
		Decimal_item(double _min, double _max, double init_value, const string& _label = string()) :
			Decimal(init_value), min(_min), max(_max), label(_label) {;}
		virtual string getString(void) {return label + Decimal::getString();}
		virtual bool isEditable(void) const {return true;}
		virtual bool onKey(SDLKey, S_TUI_VALUE);
    protected:
		double min, max;
		string label;
    };

    class Label : public Component
    {
    public:
        Label(const string& _label) : Component(), label(_label) {;}
		virtual string getString(void) {return label;}
		void setLabel(const string& s) {label=s;}
    protected:
        string label;
    };

    class Branch : public Container
    {
    public:
		Branch();
		virtual string getString(void);
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
		virtual string getString(void);
    protected:
		string label;
		bool isNavigating;
		bool isEditing;
    };

    class Time_item : public CallbackComponent
    {
    public:
		Time_item(const string& _label = string(), double _JD = 2451545.0) :
			CallbackComponent(), JD(_JD), current_edit(0), label(_label) {;}
		virtual bool onKey(SDLKey, S_TUI_VALUE);
		virtual string getString(void);
		virtual bool isEditable(void) const {return true;}
		double getJDay(void) const {return JD;}
		void setJDay(double jd) {JD = jd;}
    protected:
		void compute_ymdhms(void);
		void compute_JD(void);
		double JD;
		int current_edit;	// 0 to 5 year to second
		string label;
		int ymdhms[5];
		double second;
    };

}; // namespace s_tui

#endif // _TUI_H_
