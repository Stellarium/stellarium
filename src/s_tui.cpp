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

#include "s_tui.h"

using namespace std;
using namespace s_tui;

// Same function as getString but cleaned of every color informations
string Component::getCleanString(void)
{
	string result, s(getString());
	for (unsigned int i=0;i<s.length();++i)
	{
		if (s[i]!=start_active[0] && s[i]!=stop_active[0]) result.push_back(s[i]);
	}
	return result;
}

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

string Container::getString(void)
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

Branch::Branch() : Container()
{
	current = childs.begin();
}

string Branch::getString(void)
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
				else current=--childs.end();
				return true;
			}
			if (k==SDLK_DOWN)
			{
				if (current!=--childs.end()) ++current;
				else current=childs.begin();
				return true;
			}
			return false;
		}
	}
	return false;
}

MenuBranch::MenuBranch(const string& s) : Branch(), label(s), isNavigating(false), isEditing(false)
{
}

bool MenuBranch::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (isNavigating)
	{
		if (isEditing)
		{
			if ((*Branch::current)->onKey(k, v)) return true;
			if (v==S_TUI_PRESSED && (k==SDLK_LEFT || k==SDLK_ESCAPE || k==SDLK_RETURN))
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
			if (v==S_TUI_PRESSED && (k==SDLK_RIGHT || k==SDLK_RETURN))
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
		if (v==S_TUI_PRESSED && (k==SDLK_RIGHT || k==SDLK_RETURN))
		{
			isNavigating = true;
			return true;
		}
		return false;
	}
	return false;
}

string MenuBranch::getString(void)
{
	if (!isNavigating) return label;
	if (isEditing) (*Branch::current)->setActive(true);
	string s(Branch::getString());
	if (isEditing) (*Branch::current)->setActive(false);
	return s;
}



MenuBranch_item::MenuBranch_item(const string& s) : Branch(), label(s), isEditing(false)
{
}

bool MenuBranch_item::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (isEditing)
	{
		if ((*Branch::current)->onKey(k, v)) return true;
		if (v==S_TUI_PRESSED && (k==SDLK_LEFT || k==SDLK_ESCAPE || k==SDLK_RETURN))
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
			if (!onChangeCallback.empty()) onChangeCallback();
			return true;
		}
		if (v==S_TUI_PRESSED && k==SDLK_DOWN)
		{
			if (Branch::current!=--Branch::childs.end()) ++Branch::current;
			if (!onChangeCallback.empty()) onChangeCallback();
			return true;
		}
		if (v==S_TUI_PRESSED && (k==SDLK_RIGHT || k==SDLK_RETURN))
		{
			if ((*Branch::current)->isEditable()) isEditing = true;
			return true;
		}
		if (v==S_TUI_PRESSED && (k==SDLK_LEFT || k==SDLK_ESCAPE))
		{
			return false;
		}
		return false;
	}
	return false;
}

string MenuBranch_item::getString(void)
{
	if (active) (*Branch::current)->setActive(true);
	string s(label + Branch::getString());
	if (active) (*Branch::current)->setActive(false);
	return s;
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

string Boolean_item::getString(void)
{
	return label + (active ? start_active : "") +
		(state ? string_activated : string_disabled) +
		(active ? stop_active : "");
}

string Integer::getString(void)
{
	ostringstream os;
	os << (active ? start_active : "") << value << (active ? stop_active : "");
	return os.str();
}

string Decimal::getString(void)
{
	ostringstream os;
	os << (active ? start_active : "") << value << (active ? stop_active : "");
	return os.str();
}

bool Integer_item::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (v==S_TUI_RELEASED) return false;
	if (!numInput)
	{
		if (k==SDLK_UP)
		{
			++value;
			if (value>max)
			{
				value = max;
				return true;
			}
			if (!onChangeCallback.empty()) onChangeCallback();
			return true;
		}
		if (k==SDLK_DOWN)
		{
			--value;
			if (value<min)
			{
				value = min;
				return true;
			}
			if (!onChangeCallback.empty()) onChangeCallback();
			return true;
		}

		if (k==SDLK_0 || k==SDLK_1 || k==SDLK_2 || k==SDLK_3 || k==SDLK_4 || k==SDLK_5 ||
			k==SDLK_6 || k==SDLK_7 || k==SDLK_8 || k==SDLK_9 || k==SDLK_MINUS)
		{
			// Start editing with numerical numbers
			numInput = true;
			strInput.clear();

			char c = k;
			strInput += c;

			return true;
		}
	}
	else	// numInput == true
	{
		if (k==SDLK_RETURN || k==SDLK_LEFT)
		{
			numInput=false;
			istringstream is(strInput);
			is >> value;
			if (value>max) value = max;
			if (value<min) value = min;
			if (!onChangeCallback.empty()) onChangeCallback();
			return false;
		}

		if (k==SDLK_UP)
		{
			istringstream is(strInput);
			is >> value;
			++value;
			if (value>max) value = max;
			if (value<min) value = min;
			ostringstream os;
			os << value;
			strInput = os.str();
			return true;
		}
		if (k==SDLK_DOWN)
		{
			istringstream is(strInput);
			is >> value;
			--value;
			if (value>max) value = max;
			if (value<min) value = min;
			ostringstream os;
			os << value;
			strInput = os.str();
			return true;
		}

		if (k==SDLK_0 || k==SDLK_1 || k==SDLK_2 || k==SDLK_3 || k==SDLK_4 || k==SDLK_5 ||
			k==SDLK_6 || k==SDLK_7 || k==SDLK_8 || k==SDLK_9 || k==SDLK_MINUS)
		{
			// The user was already editing
			char c = k;
			strInput += c;
			return true;
		}

		if (k==SDLK_ESCAPE)
		{
			numInput=false;
			return false;
		}
		return true; // Block every other characters
	}

	return false;
}

string Integer_item::getString(void)
{
	ostringstream os;
	if (numInput) os << label << (active ? start_active : "") << strInput << (active ? stop_active : "");
	else os << label << (active ? start_active : "") << value << (active ? stop_active : "");
	return os.str();
}

bool Decimal_item::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (v==S_TUI_RELEASED) return false;
	if (!numInput)
	{
		if (k==SDLK_UP)
		{
			++value;
			if (value>max)
			{
				value = max;
				return true;
			}
			if (!onChangeCallback.empty()) onChangeCallback();
			return true;
		}
		if (k==SDLK_DOWN)
		{
			--value;
			if (value<min)
			{
				value = min;
				return true;
			}
			if (!onChangeCallback.empty()) onChangeCallback();
			return true;
		}

		if (k==SDLK_0 || k==SDLK_1 || k==SDLK_2 || k==SDLK_3 || k==SDLK_4 || k==SDLK_5 ||
			k==SDLK_6 || k==SDLK_7 || k==SDLK_8 || k==SDLK_9 || k==SDLK_PERIOD || k==SDLK_MINUS)
		{
			// Start editing with numerical numbers
			numInput = true;
			strInput.clear();

			char c = k;
			strInput += c;

			return true;
		}
	}
	else	// numInput == true
	{
		if (k==SDLK_RETURN || k==SDLK_LEFT)
		{
			numInput=false;
			istringstream is(strInput);
			is >> value;
			if (value>max) value = max;
			if (value<min) value = min;
			if (!onChangeCallback.empty()) onChangeCallback();
			return false;
		}

		if (k==SDLK_UP)
		{
			istringstream is(strInput);
			is >> value;
			++value;
			if (value>max) value = max;
			if (value<min) value = min;
			ostringstream os;
			os << value;
			strInput = os.str();
			return true;
		}
		if (k==SDLK_DOWN)
		{
			istringstream is(strInput);
			is >> value;
			--value;
			if (value>max) value = max;
			if (value<min) value = min;
			ostringstream os;
			os << value;
			strInput = os.str();
			return true;
		}

		if (k==SDLK_0 || k==SDLK_1 || k==SDLK_2 || k==SDLK_3 || k==SDLK_4 || k==SDLK_5 ||
			k==SDLK_6 || k==SDLK_7 || k==SDLK_8 || k==SDLK_9 || k==SDLK_PERIOD || k==SDLK_MINUS)
		{
			// The user was already editing
			char c = k;
			strInput += c;
			return true;
		}

		if (k==SDLK_ESCAPE)
		{
			numInput=false;
			return false;
		}
		return true; // Block every other characters
	}

	return false;
}

string Decimal_item::getString(void)
{
	ostringstream os;
	if (numInput) os << label << (active ? start_active : "") << strInput << (active ? stop_active : "");
	else os << label << (active ? start_active : "") << value << (active ? stop_active : "");
	return os.str();
}

bool Time_item::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (v==S_TUI_RELEASED) return false;
	if (k==SDLK_RIGHT)
	{
		++current_edit;
		if (current_edit>=6) current_edit=0;
		return true;
	}
	if (k==SDLK_LEFT)
	{
		--current_edit;
		if (current_edit<=-1)
		{
			current_edit=0;
			return false;
		}
		return true;
	}
	if (k==SDLK_UP || k==SDLK_DOWN)
	{
		compute_ymdhms();
		if (current_edit==5) second += (k==SDLK_UP ? 1 : -1);
		else ymdhms[current_edit] += (k==SDLK_UP ? 1 : -1);
		compute_JD();
		if (!onChangeCallback.empty()) onChangeCallback();
		return true;
	}
	return false;
}

// Code originally from libnova which appeared to be totally wrong... New code from celestia
void Time_item::compute_ymdhms(void)
{
    int a = (int) (JD + 0.5);
    double c;
    if (a < 2299161)
    {
        c = a + 1524;
    }
    else
    {
        double b = (int) ((a - 1867216.25) / 36524.25);
        c = a + b - (int) (b / 4) + 1525;
    }

    int d = (int) ((c - 122.1) / 365.25);
    int e = (int) (365.25 * d);
    int f = (int) ((c - e) / 30.6001);

    double dday = c - e - (int) (30.6001 * f) + ((JD + 0.5) - (int) (JD + 0.5));

    ymdhms[1] = f - 1 - 12 * (int) (f / 14);
    ymdhms[0] = d - 4715 - (int) ((7.0 + ymdhms[1]) / 10.0);
    ymdhms[2] = (int) dday;

    double dhour = (dday - ymdhms[2]) * 24;
    ymdhms[3] = (int) dhour;

    double dminute = (dhour - ymdhms[3]) * 60;
    ymdhms[4] = (int) dminute;

    second = (dminute - ymdhms[4]) * 60;
}

// Code originally from libnova which appeared to be totally wrong... New code from celestia
void Time_item::compute_JD(void)
{
    int y = ymdhms[0], m = ymdhms[1];
    if (ymdhms[1] <= 2)
    {
        y = ymdhms[0] - 1;
        m = ymdhms[1] + 12;
    }

    // Correct for the lost days in Oct 1582 when the Gregorian calendar
    // replaced the Julian calendar.
    int B = -2;
    if (ymdhms[0] > 1582 || (ymdhms[0] == 1582 && (ymdhms[1] > 10 || (ymdhms[1] == 10 && ymdhms[2] >= 15))))
    {
        B = y / 400 - y / 100;
    }

    JD = (floor(365.25 * y) +
            floor(30.6001 * (m + 1)) + B + 1720996.5 +
            ymdhms[2] + ymdhms[3] / 24.0 + ymdhms[4] / 1440.0 + second / 86400.0);
}

// Convert Julian day to yyyy/mm/dd hh:mm:ss and return the string
string Time_item::getString(void)
{
	compute_ymdhms();

	ostringstream os;
	string s1[6];
	string s2[6];
	if (active)
	{
		s1[current_edit] = start_active;
		s2[current_edit] = stop_active;
	}

	os 	<< label << s1[0] << ymdhms[0] << s2[0] << "/" << s1[1] << ymdhms[1] << s2[1] << "/"
		<< s1[2] << ymdhms[2] << s2[2] << " " << s1[3] << ymdhms[3] << s2[3] << ":"
		<< s1[4] << ymdhms[4] << s2[4] << ":"<< s1[5] << (int)round(second) << s2[5];
	return os.str();
}

string Action_item::getString(void)
{
	if (clock() - tempo > CLOCKS_PER_SEC)
	{
		if (active)
		{
			return label + start_active + string_prompt1 + stop_active;
		}
		else return label + string_prompt1;
	}
	else
	{
		if (active)
		{
			return label + start_active + string_prompt2 + stop_active;
		}
		else return label + string_prompt2;
	}
}

bool Action_item::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (v==S_TUI_PRESSED && k==SDLK_RETURN)
	{
		// Call the callback if enter is pressed
		if (!onChangeCallback.empty()) onChangeCallback();
		tempo = clock();
		return true;
	}
	return false;
}

string ActionConfirm_item::getString(void)
{
	if (active)
	{
		if (isConfirming)
		{
			return label + start_active + string_confirm + stop_active;
		}
		else
		{
			return label + start_active + string_prompt1 + stop_active;
		}
	}
	else return label + string_prompt1;
}

bool ActionConfirm_item::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (v==S_TUI_PRESSED && k==SDLK_RETURN)
	{
		if (isConfirming)
		{
			// Call the callback if enter is pressed
			if (!onChangeCallback.empty()) onChangeCallback();
			isConfirming = false;
			return true;
		}
		else
		{
			isConfirming = true;
			return true;
		}
	}
	if (v==S_TUI_PRESSED && ( k==SDLK_ESCAPE || k==SDLK_LEFT) )
	{
		if (isConfirming)
		{
			isConfirming = false;
			return true;
		}
		return false;
	}
	return true;
}
