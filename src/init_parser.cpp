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

init_parser::init_parser(const char * _file) : dico(NULL)
{
	// Check file presence
	FILE * fp = NULL;
	fp = fopen(_file,"rt");
	if (fp)
	{
		strncpy(file, _file, 255);
		fclose(fp);
	}
	else
	{
		printf("ERROR : Can't find config file %s\n",_file);
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
	dico = iniparser_load(file);
	if (!dico)
	{
		printf("ERROR : Couldn't read config file %s\n",file);
		exit(-1);
	}
}

void init_parser::save(void) const
{
	save_to(file);
}

void init_parser::save_to(const char * file_name) const
{
	// Check file presence
	FILE * fp = NULL;
	fp = fopen(file_name,"wt");
	if (fp)
	{
		iniparser_dump_ini(dico, fp);
	}
	else
	{
		printf("ERROR : Can't open file %s\n",file_name);
		exit(-1);
	}
}


const char * init_parser::get_str(const char * key) const
{
	if (iniparser_getstring(dico, key, NULL)) return iniparser_getstring(dico, key, NULL);
	else printf("WARNING : Can't find the configuration key %s, default NULL value returned\n", key);
	return NULL;
}

const char * init_parser::get_str(const char * section, const char * key) const
{
	char tmp[strlen(section) + strlen(key) + 2];
	strcpy(tmp,section);
	strcat(tmp,":");
	strcat(tmp,key);

	return get_str(tmp);
}

const char * init_parser::get_str(const char * section, const char * key, const char * def) const
{
	char tmp[strlen(section) + strlen(key) + 2];
	strcpy(tmp,section);
	strcat(tmp,":");
	strcat(tmp,key);

	return iniparser_getstring(dico, tmp, def);
}

int init_parser::get_int(const char * key) const
{
	int i = iniparser_getint(dico, key, -11111);
	// To be sure :) (bugfree)
	if (i==-11111)
	{
		i = iniparser_getint(dico, key, 0);
		if (i==0)
		{
			printf("WARNING : Can't find the configuration key %s, default 0 value returned\n", key);
		}
	}
	return i;
}

int init_parser::get_int(const char * section, const char * key) const
{
	char tmp[strlen(section) + strlen(key) + 2];
	strcpy(tmp,section);
	strcat(tmp,":");
	strcat(tmp,key);

	return get_int(tmp);
}

int init_parser::get_int(const char * section, const char * key, int def) const
{
	char tmp[strlen(section) + strlen(key) + 2];
	strcpy(tmp,section);
	strcat(tmp,":");
	strcat(tmp,key);
	return iniparser_getint(dico, tmp, def);
}

double init_parser::get_double(const char * key) const
{
	double d = iniparser_getdouble(dico, key, -11111.111111356);
	// To be sure :) (bugfree)
	if (d==-11111.111111356)
	{
		d = iniparser_getdouble(dico, key, 0.);
		if (d==0.)
		{
			printf("WARNING : Can't find the configuration key %s, default 0 value returned\n", key);
		}
	}
	return d;
}

double init_parser::get_double(const char * section, const char * key) const
{
	char tmp[strlen(section) + strlen(key) + 2];
	strcpy(tmp,section);
	strcat(tmp,":");
	strcat(tmp,key);

	return get_double(tmp);
}

double init_parser::get_double(const char * section, const char * key, double def) const
{
	char tmp[strlen(section) + strlen(key) + 2];
	strcpy(tmp,section);
	strcat(tmp,":");
	strcat(tmp,key);
	return iniparser_getdouble(dico, tmp, def);
}

int init_parser::get_boolean(const char * key) const
{
	int b = iniparser_getboolean(dico, key, -10);
	// To be sure :) (bugfree)
	if (b==-10)
	{
		b = iniparser_getboolean(dico, key, 0);
		if (b==0)
		{
			printf("WARNING : Can't find the configuration key %s, default 0 value returned\n", key);
		}
	}
	return b;
}

int init_parser::get_boolean(const char * section, const char * key) const
{
	char tmp[strlen(section) + strlen(key) + 2];
	strcpy(tmp,section);
	strcat(tmp,":");
	strcat(tmp,key);

	return get_boolean(tmp);
}

int init_parser::get_boolean(const char * section, const char * key, int def) const
{
	char tmp[strlen(section) + strlen(key) + 2];
	strcpy(tmp,section);
	strcat(tmp,":");
	strcat(tmp,key);

	return iniparser_getboolean(dico, tmp, def);
}

// Get number of sections
int init_parser::get_nsec(void) const
{
	return iniparser_getnsec(dico);
}

// Get name for section n.
const char * init_parser::get_secname(int n) const
{
	return iniparser_getsecname(dico, n);
}

// Return 1 if the entry exists, 0 otherwise
int init_parser::find_entry(const char * entry) const
{
	return iniparser_find_entry(dico, entry);
}

void init_parser::free_dico(void)
{
	iniparser_freedict(dico);
}
