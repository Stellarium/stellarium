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

// SDL is used only for the key codes, i'm lasy to redefine them
// This is TODO to make the s_ library independent
#include "SDL.h"

#include <set>
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

	// Caracters '\22' and '\21' correspond to the "change to color" blue and green special
	// stelarium ascii code, thus inserting them in a string will result in color change for the
	// next char until another color is set
	static const string start_active("\22");
	static const string stop_active("\21");

	// Base class. Note that the method bool isEditable(void) has to be overrided by returning true
	// for all the non passives components.
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

	// Store a callback on a function taking no parameters
    class CallbackComponent : public Component
    {
    public:
        virtual void set_OnChangeCallback(const callback<void>& c) {onChangeCallback = c;}
    protected:
		callback<void> onChangeCallback;
	};

	// Manage lists of components
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

	// Component which manages 2 states
	class Bistate : public CallbackComponent
    {
    public:
		Bistate(bool init_state = false) : CallbackComponent(), state(init_state) {;}
		virtual string getString(void) {return state ? string_activated : string_disabled;}
		bool getValue(void) const {return state;}
		void setValue(bool s) {state = s;}
    protected:
		string string_activated;
		string string_disabled;
		bool state;
    };

	// Component which manages integer value
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

	// Boolean item widget. The callback function is called when the state is changed
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

	// Component which manages decimal (double) value
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

	// Integer item widget. The callback function is called when the value is changed
	class Integer_item : public Integer
    {
    public:
		Integer_item(int _min, int _max, int init_value, const string& _label = string()) :
			Integer(init_value), numInput(false), min(_min), max(_max), label(_label) {;}
		virtual string getString(void);
		virtual bool isEditable(void) const {return true;}
		virtual bool onKey(SDLKey, S_TUI_VALUE);
    protected:
		bool numInput;
		string strInput;
		int min, max;
		string label;
    };

	// Decimal item widget. The callback function is called when the value is changed
	class Decimal_item : public Decimal
    {
    public:
		Decimal_item(double _min, double _max, double init_value, const string& _label = string()) :
			Decimal(init_value), numInput(false), min(_min), max(_max), label(_label) {;}
		virtual string getString(void);
		virtual bool isEditable(void) const {return true;}
		virtual bool onKey(SDLKey, S_TUI_VALUE);
    protected:
		bool numInput;
		string strInput;
		double min, max;
		string label;
    };

	// Passive widget which only display text
    class Label_item : public Component
    {
    public:
        Label_item(const string& _label) : Component(), label(_label) {;}
		virtual string getString(void) {return label;}
		void setLabel(const string& s) {label=s;}
    protected:
        string label;
    };

	// Manage list of components with one of them activated. Can navigate thru the components list
	// with the arrow keys
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

	// Base widget used for tree construction. Can navigate thru the components list with the arrow keys.
	// Activate the currently edited widget.
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

	// Widget used to set time and date. The internal format is the julian day notation
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

	// Widget which simply launch the callback when the user press enter
	class Action_item : public CallbackComponent
    {
    public:
		Action_item(const string& _label = "", const string& sp = "Enter/Escape") :
			CallbackComponent(), label(_label), string_prompt(sp) {;}
		virtual bool onKey(SDLKey, S_TUI_VALUE);
		virtual string getString(void);
		virtual bool isEditable(void) const {return true;}
    protected:
		string label;
		string string_prompt;
    };

	// Same as before but ask for a confirmation
	class ActionConfirm_item : public Action_item
    {
    public:
		ActionConfirm_item(const string& _label = "", const string& sp = "Enter/Escape",
			const string& sc = "Are you sure? (Enter/Escape)") :
				Action_item(_label, sp), isConfirming(false), string_confirm(sc) {;}
		virtual bool onKey(SDLKey, S_TUI_VALUE);
		virtual string getString(void);
    protected:
		bool isConfirming;
		string string_confirm;
    };

	// List item widget. The callback function is called when the selected item changes
	template <class T>
	class MultiSet_item : public CallbackComponent
    {
    public:
		MultiSet_item(const string& _label = string()) : CallbackComponent(), label(_label) {current = items.end();}
		virtual string getString(void)
		{
			if (current==items.end()) return label;
			ostringstream os;
			os << label << (active ? start_active : "") << *current << (active ? stop_active : "");
			return os.str();
		}
		virtual bool isEditable(void) const {return true;}
		virtual bool onKey(SDLKey k, S_TUI_VALUE v)
		{
			if (current==items.end() || v==S_TUI_RELEASED) return false;
			if (k==SDLK_UP)
			{
				if (current!=items.begin()) --current;
				else current = --items.end();
				return true;
			}
			if (k==SDLK_DOWN)
			{
				if (current!=--items.end()) ++current;
				else current = items.begin();
				return true;
			}
			return false;
		}
		void addItem(const T& newitem) {items.insert(newitem); if(current==items.end()) current = items.begin();}
		const T& getCurrent(void) const {if(current==items.end()) return emptyT; else return *current;}
		bool setCurrent(const T& i) {current = items.find(i);}
    protected:
		T emptyT;
		multiset<T> items;
		typename multiset<T>::iterator current;
		string label;
    };

}; // namespace s_tui

#endif // _TUI_H_
