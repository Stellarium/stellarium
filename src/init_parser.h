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

#include <string>
#include <iostream>
#include "iniparser.h"

using namespace std;

class init_parser
{
public:
	// Create the parser object from the given file
	// You need to call load() before using the get() functions
    init_parser(const string& file);
    virtual ~init_parser();

	// Load the config file (the parsing operation)
	void load(void);

	// Save the current config state in the original file
	void save(void) const;

	// Save the current config state in the given file
	void save_to(const string& file_name) const;

	// Get a string from the key.
	string get_str(const string& key) const;
	string get_str(const string& section, const string& key) const;
	string get_str(const string& section, const string& key, const string& def) const;

	// Get a integer from the key.
	int get_int(const string& key) const;
	int get_int(const string& section, const string& key) const;
	int get_int(const string& section, const string& key, int def) const;

	// Get a double from the key.
	double get_double(const string& key) const;
	double get_double(const string& section, const string& key) const;
	double get_double(const string& section, const string& key, double def) const;

	// Get a boolean (int) from the key.
	int get_boolean(const string& key) const;
	int get_boolean(const string& section, const string& key) const;
	int get_boolean(const string& section, const string& key, int def) const;

	int get_nsec(void) const;					// Get number of sections.
	string get_secname(int n) const;			// Get name for section n.
	int find_entry(const string& entry) const;	// Return 1 if the entry exists, 0 otherwise
	
private:
	void free_dico(void);	// Unalloc memory
	dictionary * dico;		// The dictionnary containing the parsed data
	string file;			// The config file name.
};

#endif // _INIT_PARSER_H_
