/*
* Stellarium
* Copyright (C) 2002 Fabien Chéreau
* Inspired by the gui.h by Chris Laurel <claurel@shatters.net>
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

#include "s_tui.h"

using namespace std;
using namespace s_tui;


//////////////////////////////// Container /////////////////////////////////////
// Manages hierarchical components : send signals ad actions  to childrens
////////////////////////////////////////////////////////////////////////////////

Container::~Container()
{
    list<Component*>::iterator iter = childs.begin();
    while (iter != childs.end())
    {
        if (*iter)
        {
            delete (*iter);
            (*iter)=NULL;
        }
        iter++;
    }
}

void Container::addComponent(Component* c)
{
	childs.push_back(c);
}

string Container::getString(void) const
{
	string s;
    list<Component*>::const_iterator iter = childs.begin();
	while (iter != childs.end())
	{
		s+=(*iter)->getString();
		iter++;
	}
	return s;
}


bool Container::onKey(SDLKey k, S_TUI_VALUE s)
{
	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		if ((*iter)->onKey(k, s)) return true;	// The signal has been intercepted
        iter++;
    }
    return false;
}

Branch::Branch()
{
	current = childs.begin();
}

string Branch::getString(void) const
{
	if (!*current) return string();
	else return (*current)->getString();
}

void Branch::addComponent(Component* c)
{
	childs.push_back(c);
	if (childs.size()==1) current = childs.begin();
}

bool Branch::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (!*current) return false;
	if (v==S_TUI_RELEASED) return (*current)->onKey(k,v);
	if (v==S_TUI_PRESSED)
	{
		if ((*current)->onKey(k,v)) return true;
		else
		{
			if (k==SDLK_UP)
			{
				if (current!=childs.begin()) --current;
				return true;
			}
			if (k==SDLK_DOWN)
			{
				if (current!=--childs.end()) ++current;
				return true;
			}
			return false;
		}
	}
	return false;
}

MenuBranch::MenuBranch(const string& s) : label(s), isNavigating(false), isEditing(false)
{
}

bool MenuBranch::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (isNavigating)
	{
		if (isEditing)
		{
			if ((*Branch::current)->onKey(k, v)) return true;
			if (v==S_TUI_PRESSED && (k==SDLK_LEFT || k==SDLK_ESCAPE))
			{
				isEditing = false;
				return true;
			}
			return true;
		}
		else
		{
			if (v==S_TUI_PRESSED && k==SDLK_UP)
			{
				if (Branch::current!=Branch::childs.begin()) --Branch::current;
				return true;
			}
			if (v==S_TUI_PRESSED && k==SDLK_DOWN)
			{
				if (Branch::current!=--Branch::childs.end()) ++Branch::current;
				return true;
			}
			if (v==S_TUI_PRESSED && k==SDLK_RIGHT)
			{
				if ((*Branch::current)->isEditable()) isEditing = true;
				return true;
			}
			if (v==S_TUI_PRESSED && (k==SDLK_LEFT || k==SDLK_ESCAPE))
			{
				isNavigating = false;
				return true;
			}
			return false;
		}
	}
	else
	{
		if (k==SDLK_RIGHT && v==S_TUI_PRESSED)
		{
			isNavigating = true;
			return true;
		}
		return false;
	}
	return false;
}

string MenuBranch::getString(void) const
{
	return label + (isNavigating ? string(" nav ") : string("") ) + (isEditing ? string(" edit ") : string("") ) + Branch::getString();
}

Boolean_item::Boolean_item(bool init_state, const string& _label, const string& _string_activated,
	const string& _string_disabled) :
		Bistate(init_state), label(_label)
{
	string_activated = _string_activated;
	string_disabled = _string_disabled;
}

bool Boolean_item::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (v==S_TUI_PRESSED && (k==SDLK_UP || k==SDLK_DOWN) )
	{
		state = !state;
		if (!onChangeCallback.empty()) onChangeCallback();
		return true;
	}
	return false;
}
