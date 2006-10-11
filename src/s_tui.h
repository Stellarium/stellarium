/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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

#include "stel_utility.h"
#include "translator.h"

// SDL is used only for the key codes, i'm lasy to redefine them
// This is TODO to make the s_ library independent
#include "SDL.h"

#include <set>
#include <list>
#include <map>
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
	static const wstring start_active(L"\22");  // white is hilight
	static const wstring stop_active(L"\21");

	// Base class. Note that the method bool isEditable(void) has to be overrided by returning true
	// for all the non passives components.
    class Component
    {
    public:
		Component() : active(false) {;}
		virtual ~Component() {;}
		virtual wstring getString(void) {return wstring();}
		virtual wstring getCleanString(void);
		// Return true if key signal intercepted, false if not
		virtual bool onKey(Uint16, S_TUI_VALUE) {return false;}
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
		virtual wstring getString(void);
        virtual void addComponent(Component*);
		virtual bool onKey(Uint16, S_TUI_VALUE);
    protected:
        list<Component*> childs;
    };

	// Component which manages 2 states
	class Bistate : public CallbackComponent
    {
    public:
		Bistate(bool init_state = false) : CallbackComponent(), state(init_state) {;}
		virtual wstring getString(void) {return state ? string_activated : string_disabled;}
		bool getValue(void) const {return state;}
		void setValue(bool s) {state = s;}
    protected:
		wstring string_activated;
		wstring string_disabled;
		bool state;
    };

	// Component which manages integer value
	class Integer : public CallbackComponent
    {
    public:
		Integer(int init_value = 0) : CallbackComponent(), value(init_value) {;}
		virtual wstring getString(void);
		int getValue(void) const {return value;}
		void setValue(int v) {value = v;}
    protected:
		int value;
    };

	// Boolean item widget. The callback function is called when the state is changed
	class Boolean_item : public Bistate
    {
    public:
		Boolean_item(bool init_state = false, const wstring& _label = wstring(),
			const wstring& _string_activated = wstring(L"ON"),
			const wstring& string_disabled  = wstring(L"OFF"));
		virtual bool onKey(Uint16, S_TUI_VALUE);
		virtual wstring getString(void);
		virtual bool isEditable(void) const {return true;}
		void setLabel(const wstring& _label, const wstring& _active, const wstring& _disabled) 
		{label = _label; string_activated=_active; string_disabled=_disabled;}
    protected:
		wstring label;
    };

	// Component which manages decimal (double) value
	class Decimal : public CallbackComponent
    {
    public:
		Decimal(double init_value = 0.) : CallbackComponent(), value(init_value) {;}
		virtual wstring getString(void);
		double getValue(void) const {return value;}
		void setValue(double v) {value = v;}
    protected:
		double value;
    };

	// Integer item widget. The callback function is called when the value is changed
	class Integer_item : public Integer
    {
    public:
		Integer_item(int _min, int _max, int init_value, const wstring& _label = wstring()) :
			Integer(init_value), numInput(false), mmin(_min), mmax(_max), label(_label) {;}
		virtual wstring getString(void);
		virtual bool isEditable(void) const {return true;}
		virtual bool onKey(Uint16, S_TUI_VALUE);
		void setLabel(const wstring& _label) {label = _label;}
    protected:
		bool numInput;
		wstring strInput;
		int mmin, mmax;
		wstring label;
    };

	// Decimal item widget. The callback function is called when the value is changed
	class Decimal_item : public Decimal
    {
    public:
		Decimal_item(double _min, double _max, double init_value, const wstring& _label = wstring(), double _delta = 1.0) :
			Decimal(init_value), numInput(false), mmin(_min), mmax(_max), label(_label), delta(_delta) {;}
		virtual wstring getString(void);
		virtual bool isEditable(void) const {return true;}
		virtual bool onKey(Uint16, S_TUI_VALUE);
		void setLabel(const wstring& _label) {label = _label;}
    protected:
		bool numInput;
		wstring strInput;
		double mmin, mmax;
		wstring label;
		double delta;
    };

	// Passive widget which only display text
    class Label_item : public Component
    {
    public:
        Label_item(const wstring& _label) : Component(), label(_label) {;}
		virtual wstring getString(void) {return label;}
		void setLabel(const wstring& s) {label=s;}
    protected:
        wstring label;
    };

	// Manage list of components with one of them selected.
	// Can navigate thru the components list with the arrow keys
    class Branch : public Container
    {
    public:
		Branch();
		virtual wstring getString(void);
		virtual bool onKey(Uint16, S_TUI_VALUE);
        virtual void addComponent(Component*);
		virtual Component* getCurrent(void) const {if (current==childs.end()) return NULL; else return *current;}
		virtual bool setValue(const wstring&);
		virtual bool setValue_Specialslash(const wstring&);
    protected:
        list<Component*>::const_iterator current;
    };

	// Base widget used for tree construction. Can navigate thru the components list with the arrow keys.
	// Activate the currently edited widget.
    class MenuBranch : public Branch
    {
    public:
		MenuBranch(const wstring& s);
		virtual bool onKey(Uint16, S_TUI_VALUE);
		virtual wstring getString(void);
		virtual bool isEditable(void) const {return true;}
		wstring getLabel(void) const {return label;}
		void setLabel(const wstring& _label) {label = _label;}
    protected:
		wstring label;
		bool isNavigating;
		bool isEditing;
    };

	// Widget quite like Menu Branch but always navigating, and always display the label
    class MenuBranch_item : public Branch
    {
    public:
		MenuBranch_item(const wstring& s);
		virtual bool onKey(Uint16, S_TUI_VALUE);
		virtual wstring getString(void);
		virtual bool isEditable(void) const {return true;}
		wstring getLabel(void) const {return label;}
    protected:
		wstring label;
		bool isEditing;
    };


	// Widget used to set time and date. The internal format is the julian day notation
    class Time_item : public CallbackComponent
    {
    public:
		Time_item(const wstring& _label = wstring(), double _JD = 2451545.0);
		~Time_item();
		virtual bool onKey(Uint16, S_TUI_VALUE);
		virtual wstring getString(void);
		virtual wstring getDateString(void);
		virtual bool isEditable(void) const {return true;}
		double getJDay(void) const {return JD;}
		void setJDay(double jd) {JD = jd;}
		void setLabel(const wstring& _label) {label = _label;}
    protected:
		void compute_ymdhms(void);
		void compute_JD(void);
		double JD;
		Integer_item* current_edit;	// 0 to 5 year to second
		wstring label;
		int ymdhms[5];
		double second;
		Integer_item *y, *m, *d, *h, *mn, *s;
    };


	// Widget which simply launch the callback when the user press enter
	class Action_item : public CallbackComponent
    {
    public:
		Action_item(const wstring& _label = L"", const wstring& sp1 = _("Do"), const wstring& sp2 = _("Done")) :
			CallbackComponent(), label(_label), string_prompt1(sp1), string_prompt2(sp2) {tempo = clock();}
		virtual bool onKey(Uint16, S_TUI_VALUE);
		virtual wstring getString(void);
		virtual bool isEditable(void) const {return true;}
		void setLabel(const wstring& _label) {label = _label;}
    protected:
		wstring label;
		wstring string_prompt1;
		wstring string_prompt2;
		int tempo;
    };

	// Same as before but ask for a confirmation
	class ActionConfirm_item : public Action_item
    {
    public:
		ActionConfirm_item(const wstring& _label = L"", const wstring& sp1 = _("Do"), const wstring& sp2 = _("Done"),	const wstring& sc = _("Are you sure ?")) :
				Action_item(_label, sp1, sp2), isConfirming(false), string_confirm(sc) {;}
		virtual bool onKey(Uint16, S_TUI_VALUE);
		virtual wstring getString(void);
    protected:
		bool isConfirming;
		wstring string_confirm;
    };

	// List item widget. The callback function is called when the selected item changes
	template <class T>
	class MultiSet_item : public CallbackComponent
    {
    public:
		MultiSet_item(const wstring& _label = wstring()) : CallbackComponent(), label(_label) {current = items.end();}
		MultiSet_item(const MultiSet_item& m) : CallbackComponent(), label(m.label)
		{
			setCurrent(m.getCurrent());
		}
		virtual wstring getString(void)
		{
			if (current==items.end()) return label;
			return label + (active ? start_active : L"") + *current + (active ? stop_active : L"");
		}
		virtual bool isEditable(void) const {return true;}
		virtual bool onKey(Uint16 k, S_TUI_VALUE v)
		{
		  if (current==items.end() || v==S_TUI_RELEASED) return false;
		        if (k==SDLK_RETURN)
			{
				if (!onTriggerCallback.empty()) onTriggerCallback();
				return false;
			}
			if (k==SDLK_UP)
			{
				if (current!=items.begin()) --current;
				else current = --items.end();
				if (!onChangeCallback.empty()) onChangeCallback();
				return true;
			}
			if (k==SDLK_DOWN)
			{
			        if (current!= --items.end()) ++current;
				else current = items.begin();
				if (!onChangeCallback.empty()) onChangeCallback();
				return true;
			}
			if (k==SDLK_LEFT || k==SDLK_ESCAPE) return false;
			return false;
		}
		void addItem(const T& newitem) {
		  items.insert(newitem); 
		  if(current==items.end()) current = items.begin();
		}
		void addItemList(const wstring& s)
		{
			wistringstream is(s);
			T elem;
			while(getline(is, elem))
			{
				addItem(elem);
			}
		}
		void replaceItemList(wstring s, int selection)
		{
		  items.clear();
		  addItemList(s);
		  current = items.begin();
		  for(int j=0; j<selection;j++) {
		    ++current;
		  }
		}
		const T& getCurrent(void) const {if((typename multiset<T>::const_iterator)current==items.end()) return emptyT; else return *current;}
		void setCurrent(const T& i) {
		  current = items.find(i);

		  // if not found, set to first item!
		  if(current==items.end()) {
		    current = items.begin();
		    if (!onChangeCallback.empty()) onChangeCallback();
		  }
		}
		bool setValue(const T& i)
		{
			if (items.find(i) == items.end()) return false;
			else current = items.find(i); return true;
		}
		wstring getLabel(void) const {return label;}
		virtual void set_OnTriggerCallback(const callback<void>& c) {onTriggerCallback = c;}
		void setLabel(const wstring& _label) {label = _label;}
    protected:
		T emptyT;
		multiset<T> items;
		typename multiset<T>::iterator current;
		wstring label;
		callback<void> onTriggerCallback;
    };

	// Specialization for strings because operator wstream << string does not exists..
	template<>
	wstring MultiSet_item<string>::getString(void);


	// List item widget with separation between UI keys (will be translated) and code value (never translated).
	// Assumes one-to-one mapping of keys to values
	// The callback function is called when the selected item changes
	template <class T>
	class MultiSet2_item : public CallbackComponent
    {
    public:
		MultiSet2_item(const wstring& _label = wstring()) : CallbackComponent(), label(_label) {current = items.end();}
		MultiSet2_item(const MultiSet2_item& m) : CallbackComponent(), label(m.label)
		{
			setCurrent(m.getCurrent());
		}
		virtual wstring getString(void)
		{
			if (current==items.end()) return label;
			return label + (active ? start_active : L"") + *current + (active ? stop_active : L"");
		}
		virtual bool isEditable(void) const {return true;}
		virtual bool onKey(Uint16 k, S_TUI_VALUE v)
		{
		  if (current==items.end() || v==S_TUI_RELEASED) return false;
		        if (k==SDLK_RETURN)
			{
				if (!onTriggerCallback.empty()) onTriggerCallback();
				return false;
			}
			if (k==SDLK_UP)
			{
				if (current!=items.begin()) --current;
				else current = --items.end();
				if (!onChangeCallback.empty()) onChangeCallback();
				return true;
			}
			if (k==SDLK_DOWN)
			{
			        if (current!= --items.end()) ++current;
				else current = items.begin();
				if (!onChangeCallback.empty()) onChangeCallback();
				return true;
			}
			if (k==SDLK_LEFT || k==SDLK_ESCAPE) return false;
			return false;
		}
		void addItem(const T& newkey, const T& newvalue) {
		  items.insert(newkey);
		  value[newkey] = newvalue;
		  if(current==items.end()) current = items.begin();
		}
		void addItemList(wstring s)  // newline delimited, key and value alternate
		{
			wistringstream is(s);
			T key, value;
			while(getline(is, key) && getline(is, value))
			{
				addItem(key, value);
			}
		}
		void replaceItemList(wstring s, int selection)
		{
		  items.clear();
		  value.clear();
		  addItemList(s);
		  current = items.begin();
		  for(int j=0; j<selection;j++) {
		    ++current;
		  }
		}
		const T& getCurrent(void) {if(current==items.end()) return emptyT; else return value[(*current)];}
		void setCurrent(const T& i) {  // set by value, not key

			typename multiset<T>::iterator iter;

			bool found =0;
			for(iter=items.begin(); iter!=items.end(); iter++ ) {
				if( i == value[(*iter)]) {
					current = iter;
					found = 1;
					break;
				}
			}

			if(!found) current = items.begin();
		    if (!onChangeCallback.empty()) onChangeCallback();
		}
	
		bool setValue(const T& i)
		{
			typename multiset<T>::iterator iter;

			bool found =0;
			for(iter=items.begin(); iter!=items.end(); iter++ ) {
				if( i == value[(*iter)]) {
					current = iter;
					found = 1;
					break;
				}
			}

			return found;
		}
		wstring getLabel(void) const {return label;}
		virtual void set_OnTriggerCallback(const callback<void>& c) {onTriggerCallback = c;}
		void setLabel(const wstring& _label) {label = _label;}
    protected:
		T emptyT;
		multiset<T> items;
		typename multiset<T>::iterator current;
		wstring label;
		callback<void> onTriggerCallback;
		map<T, T> value;  // hash of key, value pairs
    };


	// Widget used to set time zone. Initialized from a file of type /usr/share/zoneinfo/zone.tab
    class Time_zone_item : public CallbackComponent
    {
    public:
		Time_zone_item(const string& zonetab_file, const wstring& _label = wstring());
		virtual bool onKey(Uint16, S_TUI_VALUE);
		virtual wstring getString(void);
		virtual bool isEditable(void) const {return true;}
		string gettz(void); // should be const but gives a boring error...
		void settz(const string& tz);
		void setLabel(const wstring& _label) {label = _label;}
    protected:
		MultiSet_item<string> continents_names;
		map<string, MultiSet_item<string> > continents;
		wstring label;
		MultiSet_item<string>* current_edit;
    };


	// Widget used to edit a vector
    class Vector_item : public CallbackComponent
    {
    public:
		Vector_item(const wstring& _label = wstring(), Vec3d _init_vector = Vec3d(0,0,0));
		~Vector_item();
		virtual bool onKey(Uint16, S_TUI_VALUE);
		virtual wstring getString(void);
		virtual bool isEditable(void) const {return true;}
		Vec3d getVector(void) const { return Vec3d( a->getValue(), b->getValue(), c->getValue()); }
		void setVector(Vec3d _vector) { a->setValue(_vector[0]); b->setValue(_vector[1]); c->setValue(_vector[2]); }
		void setLabel(const wstring& _label) {label = _label;}
    protected:
		Decimal_item* current_edit;	// 0 to 2
		wstring label;
		Decimal_item *a, *b, *c;
    };



}; // namespace s_tui

#endif // _TUI_H_
