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

#ifndef _INIT_PARSER_H_
#define _INIT_PARSER_H_

#include "iniparser.h"

class init_parser
{
public:
    init_parser(char * file);
    virtual ~init_parser();
	void load(void);
	void save(void);
	void save_to(char * file_name);
	const char * get_str(char * key);
	int get_int(char * key);
	double get_double(char * key);
	int get_boolean(char * key);
private:
	void free_dico(void);
	dictionary * dico;
	file char[255];
};

#endif // _INIT_PARSER_H_
