/*
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

// Class which parse an init file
// C++ warper for the iniparser free library from N.Devillard

#include "init_parser.h"

init_parser::init_parser(const string& _file) : dico(NULL)
{
	// Check file presence
	FILE * fp = NULL;
	fp = fopen(_file.c_str(),"rt");
	if (fp)
	{
		file = _file;
		fclose(fp);
	}
	else
	{
		cout << "ERROR : Can't find config file " << _file << endl;
		exit(-1);
	}
}

init_parser::~init_parser()
{
	free_dico();
}

void init_parser::load(void)
{
	if (dico) free_dico();
	dico = NULL;
	dico = iniparser_load(file.c_str());
	if (!dico)
	{
		cout << "ERROR : Couldn't read config file " << file << endl;
		exit(-1);
	}
}

void init_parser::save(void) const
{
	save_to(file.c_str());
}

void init_parser::save_to(const string& file_name) const
{
	// Check file presence
	FILE * fp = NULL;
	fp = fopen(file_name.c_str(),"wt");
	if (fp)
	{
		iniparser_dump_ini(dico, fp);
	}
	else
	{
		cout << "ERROR : Can't open file " << file_name << endl;
		exit(-1);
	}
}


string init_parser::get_str(const string& key) const
{
	if (iniparser_getstring(dico, key.c_str(), NULL)) return string(iniparser_getstring(dico, key.c_str(), NULL));
	else cout << "WARNING : Can't find the configuration key " << key << " default empty string returned" << endl;
	return string();
}

string init_parser::get_str(const string& section, const string& key) const
{
	return get_str((section+":"+key).c_str());
}

string init_parser::get_str(const string& section, const string& key, const string& def) const
{
	return iniparser_getstring(dico, (section+":"+key).c_str(), def.c_str());
}

int init_parser::get_int(const string& key) const
{
	int i = iniparser_getint(dico, key.c_str(), -11111);
	// To be sure :) (bugfree)
	if (i==-11111)
	{
		i = iniparser_getint(dico, key.c_str(), 0);
		if (i==0)
		{
			printf("WARNING : Can't find the configuration key %s, default 0 value returned\n", key.c_str());
		}
	}
	return i;
}

int init_parser::get_int(const string& section, const string& key) const
{
	return get_int((section+":"+key).c_str());
}

int init_parser::get_int(const string& section, const string& key, int def) const
{
	return iniparser_getint(dico, (section+":"+key).c_str(), def);
}

double init_parser::get_double(const string& key) const
{
	double d = iniparser_getdouble(dico, key.c_str(), -11111.111111356);
	// To be sure :) (bugfree)
	if (d==-11111.111111356)
	{
		d = iniparser_getdouble(dico, key.c_str(), 0.);
		if (d==0.)
		{
			printf("WARNING : Can't find the configuration key %s, default 0 value returned\n", key.c_str());
		}
	}
	return d;
}

double init_parser::get_double(const string& section, const string& key) const
{
	return get_double((section+":"+key).c_str());
}

double init_parser::get_double(const string& section, const string& key, double def) const
{
	return iniparser_getdouble(dico, (section+":"+key).c_str(), def);
}

int init_parser::get_boolean(const string& key) const
{
	int b = iniparser_getboolean(dico, key.c_str(), -10);
	// To be sure :) (bugfree)
	if (b==-10)
	{
		b = iniparser_getboolean(dico, key.c_str(), 0);
		if (b==0)
		{
			printf("WARNING : Can't find the configuration key %s, default 0 value returned\n", key.c_str());
		}
	}
	return b;
}

int init_parser::get_boolean(const string& section, const string& key) const
{
	return get_boolean((section+":"+key).c_str());
}

int init_parser::get_boolean(const string& section, const string& key, int def) const
{
	return iniparser_getboolean(dico, (section+":"+key).c_str(), def);
}

// Get number of sections
int init_parser::get_nsec(void) const
{
	return iniparser_getnsec(dico);
}

// Get name for section n.
string init_parser::get_secname(int n) const
{
	if (iniparser_getsecname(dico, n)) return string(iniparser_getsecname(dico, n));
	return string();
}

// Return 1 if the entry exists, 0 otherwise
int init_parser::find_entry(const string& entry) const
{
	return iniparser_find_entry(dico, entry.c_str());
}

void init_parser::free_dico(void)
{
	iniparser_freedict(dico);
}
