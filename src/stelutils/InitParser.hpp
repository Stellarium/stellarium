/*
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

// Class which parses an init file
// C++ warper for the iniparser free library from N.Devillard

#ifndef _INIT_PARSER_H_
#define _INIT_PARSER_H_

#include <string>
#include <iostream>

using namespace std;

// Predeclaration
typedef struct _dictionary_ dictionary;

//! Class which parses .ini configuration files.
//! It is a C++ wrapper on the C library iniparser, by Nicolas Devillard
class InitParser
{
public:
	// Create the parser object from the given file
	// You need to call load() before using the get() functions
    InitParser();
    virtual ~InitParser();

	// Load the config file (the parsing operation)
	void load(const string& file_name);

	// Save the current config state in the original file
	void save(const string& file_name) const;

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

	// Get a boolean from the key.
	bool get_boolean(const string& key) const;
	bool get_boolean(const string& section, const string& key) const;
	bool get_boolean(const string& section, const string& key, bool def) const;

	// Set the given entry with the provided value. If the entry cannot be found
	// -1 is returned and the entry is created. Else 0 is returned.
	int set_str(const string& key, const string& val);
	int set_int(const string& key, int val);
	int set_double(const string& key, double val);
	int set_boolean(const string& key, bool val);

	int get_nsec(void) const;					// Get number of sections.
	string get_secname(int n) const;			// Get name for section n.
	int find_entry(const string& entry) const;	// Return 1 if the entry exists, 0 otherwise

private:
	// Check if the key is in the form section:key and if yes create the section in the dictionnary
	// if it doesn't exist.
	void make_section_from_key(const string& key);

	void free_dico(void);	// Unalloc memory
	dictionary * dico;		// The dictionnary containing the parsed data
};

#endif // _INIT_PARSER_H_
