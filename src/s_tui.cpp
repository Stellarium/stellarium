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

#include <fstream>
#include <cmath>
#include "s_tui.h"

// A float to nearest int conversion routine for the systems which
// are not C99 conformant
inline int S_ROUND(float value)
{
	static double i_part;

	if (value >= 0)
	{
		if (modf((double) value, &i_part) >= 0.5)
			i_part += 1;
	}
	else
	{
		if (modf((double) value, &i_part) <= -0.5)
			i_part -= 1;
	}

	return ((int) i_part);
}

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

bool Branch::setValue(const string& s)
{
	list<Component*>::iterator c;
	for (c=childs.begin();c!=childs.end();c++)
	{
		cout << (*c)->getCleanString() << endl;
		if ((*c)->getCleanString()==s)
		{
			current = c;
			return true;
		}
	}
	return false;
}

bool Branch::setValue_Specialslash(const string& s)
{
	list<Component*>::iterator c;
	for (c=childs.begin();c!=childs.end();c++)
	{
		string cs = (*c)->getCleanString();
		int pos = cs.find('/');
		string ccs = cs.substr(0,pos);
		if (ccs==s)
		{
			current = c;
			return true;
		}
	}
	return false;
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
		return false;
	}
	else	// numInput == true
	{
		if (k==SDLK_RETURN)
		{
			numInput=false;
			istringstream is(strInput);
			is >> value;
			if (value>max) value = max;
			if (value<min) value = min;
			if (!onChangeCallback.empty()) onChangeCallback();
			return true;
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
			value+=delta;
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
			value-=delta;
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
			value+=delta;
			if (value>max) value = max;
			if (value<min) value = min;
			static char tempstr[16];
			snprintf(tempstr, 15, "%.2f", value);
			string os(tempstr);
			strInput = os.c_str();
			return true;
		}
		if (k==SDLK_DOWN)
		{
			value = atof(strInput.c_str());
			value-=delta;
			if (value>max) value = max;
			if (value<min) value = min;
			static char tempstr[16];
			snprintf(tempstr, 15, "%.2f", value);
			string os(tempstr);
			strInput = os.c_str();
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

	// Can't directly write value in os because there is a float precision limit bug..
	static char tempstr[16];
	snprintf(tempstr, 15, "%.2f", value);
	string vstr(tempstr);

	if (numInput) os << label << (active ? start_active : "") << strInput << (active ? stop_active : "");
	else os << label << (active ? start_active : "") << vstr.c_str() << (active ? stop_active : "");
	return os.str();
}


Time_item::Time_item(const string& _label, double _JD) :
	CallbackComponent(), JD(_JD), current_edit(NULL), label(_label),
	y(NULL), m(NULL), d(NULL), h(NULL), mn(NULL), s(NULL)
{
	y = new Integer_item(-100000, 100000, 2003);
	m = new Integer_item(1, 12, 1);
	d = new Integer_item(1, 31, 1);
	h = new Integer_item(1, 23, 1);
	mn = new Integer_item(1, 59, 1);
	s = new Integer_item(1, 59, 1);
	current_edit = y;
}

Time_item::~Time_item()
{
	if (y) delete y;
	if (m) delete y;
	if (d) delete y;
	if (h) delete y;
	if (mn) delete y;
	if (s) delete y;
}

bool Time_item::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (v==S_TUI_RELEASED) return false;

	if (current_edit->onKey(k,v))
	{
		compute_JD();
		compute_ymdhms();
		if (!onChangeCallback.empty()) onChangeCallback();
		return true;
	}
	else
	{
		if (k==SDLK_ESCAPE)
		{
			return false;
		}

		if (k==SDLK_RIGHT)
		{
			if (current_edit==y) current_edit=m;
			else if (current_edit==m) current_edit=d;
			else if (current_edit==d) current_edit=h;
			else if (current_edit==h) current_edit=mn;
			else if (current_edit==mn) current_edit=s;
			else if (current_edit==s) current_edit=y;
			return true;
		}
		if (k==SDLK_LEFT)
		{
			if (current_edit==y) current_edit=s;
			else if (current_edit==m) current_edit=y;
			else if (current_edit==d) current_edit=m;
			else if (current_edit==h) current_edit=d;
			else if (current_edit==mn) current_edit=h;
			else if (current_edit==s) current_edit=mn;
			return true;
		}
	}

	return false;
}

// Convert Julian day to yyyy/mm/dd hh:mm:ss and return the string
string Time_item::getString(void)
{
	compute_ymdhms();

	string s1[6];
	string s2[6];
	if (current_edit==y && active){s1[0] = start_active; s2[0] = stop_active;}
	if (current_edit==m && active){s1[1] = start_active; s2[1] = stop_active;}
	if (current_edit==d && active){s1[2] = start_active; s2[2] = stop_active;}
	if (current_edit==h && active){s1[3] = start_active; s2[3] = stop_active;}
	if (current_edit==mn && active){s1[4] = start_active; s2[4] = stop_active;}
	if (current_edit==s && active){s1[5] = start_active; s2[5] = stop_active;}

	ostringstream os;
	os 	<< label <<
	s1[0] << y->getString() << s2[0] << "/" <<
	s1[1] << m->getString() << s2[1] << "/" <<
	s1[2] << d->getString() << s2[2] << " " <<
	s1[3] << h->getString() << s2[3] << ":" <<
	s1[4] << mn->getString() << s2[4] << ":" <<
	s1[5] << s->getString() << s2[5];
	return os.str();
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

    int dd = (int) ((c - 122.1) / 365.25);
    int e = (int) (365.25 * dd);
    int f = (int) ((c - e) / 30.6001);

    double dday = c - e - (int) (30.6001 * f) + ((JD + 0.5) - (int) (JD + 0.5));

    ymdhms[1] = f - 1 - 12 * (int) (f / 14);
    ymdhms[0] = dd - 4715 - (int) ((7.0 + ymdhms[1]) / 10.0);
    ymdhms[2] = (int) dday;

    double dhour = (dday - ymdhms[2]) * 24;
    ymdhms[3] = (int) dhour;

    double dminute = (dhour - ymdhms[3]) * 60;
    ymdhms[4] = (int) dminute;

    second = (dminute - ymdhms[4]) * 60;

	y->setValue(ymdhms[0]);
	m->setValue(ymdhms[1]);
	d->setValue(ymdhms[2]);
	h->setValue(ymdhms[3]);
	mn->setValue(ymdhms[4]);
	s->setValue(S_ROUND(second));
}

// Code originally from libnova which appeared to be totally wrong... New code from celestia
void Time_item::compute_JD(void)
{
	ymdhms[0] = y->getValue();
	ymdhms[1] = m->getValue();
	ymdhms[2] = d->getValue();
	ymdhms[3] = h->getValue();
	ymdhms[4] = mn->getValue();
	second = s->getValue();

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


// Widget used to set time zone. Initialized from a file of type /usr/share/zoneinfo/zone.tab
Time_zone_item::Time_zone_item(const string& zonetab_file, const string& _label) : label(_label)
{
	if (zonetab_file.empty())
	{
		cout << "Can't find file \"" << zonetab_file << "\"\n" ;
		exit(0);
	}

	ifstream is(zonetab_file.c_str());

	string unused, tzname;
	char zoneline[256];
	int i;

	while (is.getline(zoneline, 256))
	{
		if (zoneline[0]=='#') continue;
		istringstream istr(zoneline);
		istr >> unused >> unused >> tzname;
		i = tzname.find("/");
		if (continents.find(tzname.substr(0,i))==continents.end())
		{
			continents.insert(pair<string, MultiSet_item<string> >(tzname.substr(0,i),MultiSet_item<string>()));
			continents[tzname.substr(0,i)].addItem(tzname.substr(i+1,tzname.size()));
			continents_names.addItem(tzname.substr(0,i));
		}
		else
		{
			continents[tzname.substr(0,i)].addItem(tzname.substr(i+1,tzname.size()));
		}
	}

	is.close();
	current_edit=&continents_names;
}

bool Time_zone_item::onKey(SDLKey k, S_TUI_VALUE v)
{
	if (v==S_TUI_RELEASED) return false;

	if (current_edit->onKey(k,v))
	{
		if (!onChangeCallback.empty()) onChangeCallback();
		return true;
	}
	else
	{
		if (k==SDLK_ESCAPE)
		{
			return false;
		}

		if (k==SDLK_RIGHT)
		{
			if (current_edit==&continents_names) current_edit = &continents[continents_names.getCurrent()];
			else current_edit=&continents_names;
			return true;
		}
		if (k==SDLK_LEFT)
		{
			if (current_edit==&continents_names) return false;
			else current_edit = &continents_names;
			return true;
		}
	}

	return false;
}

string Time_zone_item::getString(void)
{
	string s1[2], s2[2];
	if (current_edit==&continents_names && active){s1[0] = start_active; s2[0] = stop_active;}
	if (current_edit!=&continents_names && active){s1[1] = start_active; s2[1] = stop_active;}

	return label + s1[0] + continents_names.getCurrent() + s2[0] + "/" + s1[1] +
		continents[continents_names.getCurrent()].getCurrent() + s2[1];
}

string Time_zone_item::gettz(void) // should be const but gives a boring error...
{
	if (continents.find(continents_names.getCurrent())!=continents.end())
		return continents_names.getCurrent() + "/" + continents[continents_names.getCurrent()].getCurrent();
	else return continents_names.getCurrent() + "/error" ;
}

void Time_zone_item::settz(string tz)
{
	int i = tz.find("/");
	continents_names.setCurrent(tz.substr(0,i));
	continents[continents_names.getCurrent()].setCurrent(tz.substr(i+1,tz.size()));
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
