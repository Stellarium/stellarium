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
	// Create the parser object from the given file
	// You need to call load() before using the get() functions
    init_parser(char * file);
    virtual ~init_parser();

	// Load the config file (the parsing operation)
	void load(void);

	// Save the current config state in the original file
	void save(void);

	// Save the current config state in the given file
	void save_to(char * file_name);

	// Get a string from the key. The returned char pointer is pointing
	// to a string allocated in the dictionary, do not free or modify it.
	const char * get_str(char * key);
	int get_int(char * key);		// Get a integer from the key.
	double get_double(char * key);	// Get a double from the key.
	int get_boolean(char * key);	// Get a boolean (int) from the key.

private:
	void free_dico(void);	// Unalloc memory
	dictionary * dico;		// The dictionnary containing the parsed data
	char file[255];			// The config file name.
};

#endif // _INIT_PARSER_H_
