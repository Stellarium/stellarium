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


init_parser::init_parser(char * _file) : file(NULL), dico(NULL)
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
		printf(stderr, "ERROR : Can't find config file %s\n",_file);
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
		printf(stderr, "ERROR : Couldn't read config file %s\n",file);
		exit(-1);
	}
}

void init_parser::save(void)
{
	save_to(file);
}

void init_parser::save_to(char * file_name)
{
	// Check file presence
	FILE * fp = NULL;
	fp = fopen(file_name,"wt");
	if (fp)
	{
		iniparser_dump(dico, fp);
	}
	else
	{
		printf(stderr, "ERROR : Can't open file %s\n",_file);
		exit(-1);
	}
}

const char * init_parser::get_str(char * key)
{
	return iniparser_getstring(dico, key, NULL);
}

int init_parser::get_int(char * key)
{
	int i = getint(dico, key, -11111);
	// To be sure :) (bugfree)
	if (i==-11111)
	{
		i = getint(dico, key, 0);
		if (i==0)
		{
			printf("WARNING : Can't find the configuration key %s, default 0 value returned\n");
		}
	}
	return i;
}

double init_parser::get_double(char * key)
{
	double d = iniparser_getdouble(dico, key, -11111.111111356);
	// To be sure :) (bugfree)
	if (d==-11111.111111356)
	{
		d = getint(dico, key, 0.);
		if (d==0.)
		{
			printf("WARNING : Can't find the configuration key %s, default 0 value returned\n");
		}
	}
	return d;
}

int init_parser::get_boolean(char * key)
{
	int b = iniparser_getboolean(dico, key, -10);
	// To be sure :) (bugfree)
	if (b==-10)
	{
		b = getint(dico, key, 10);
		if (b==10)
		{
			printf("WARNING : Can't find the configuration key %s, default 0 value returned\n");
		}
	}
	return b;
}


void init_parser::free_dico(void)
{
	iniparser_freedict(dico);
}
