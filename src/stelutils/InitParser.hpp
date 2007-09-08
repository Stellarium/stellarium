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

//! @class InitParser Parses .ini configuration files.
//! This class is used to parse and modify ini style configuration files.
//! Settings in an ini file are separated into sections, and in each section are
//! specified by a key.
//! It is a C++ wrapper on the C library iniparser, by Nicolas Devillard
class InitParser
{
public:
	//! Create the parser object. Note you will need to call
	//! load() before using the get() functions.
	InitParser();
	virtual ~InitParser();

	//! Load the config file (the parsing operation)
	//! @param file_name the path to the .ini file to be loaded.
	void load(const string& file_name);

	//! Save the current config state.
	//! @param file_name the path to the .ini file to be written
	void save(const string& file_name) const;

	//! Get a string for a setting in the InitParser object.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param key is a compound key consisting of the section and 
	//! section-key separated by a colon (:), e.g. "sectionname:keyname".
	//! @return the value for the specified key, or an empty string if the key is not found.
	string get_str(const string& key) const;
	
	//! Get a string for a setting in the InitParser object.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param section the section in which the required setting is to be found.
	//! @param key is the key which identifes the required setting with the specified section.
	//! @return the value for the specified key, or an empty string if the key is not found.
	string get_str(const string& section, const string& key) const;
	
	//! Get a string for a setting in the InitParser object, using a default value if the
	//! requested setting is not found in the ini file.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param section the section in which the required setting is to be found.
	//! @param key is the key which identifes the required setting with the specified section.
	//! @param def the default value which is used if the requested setting is not found.
	//! @return the value for the specified key, or the value of def if the key is not found.
	string get_str(const string& section, const string& key, const string& def) const;

	//! Get an integer value for a setting in the InitParser object.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param key is a compound key consisting of the section and 
	//! section-key separated by a colon (:), e.g. "sectionname:keyname".
	//! @return the value for the specified key, or 0 if the key is not found.
	int get_int(const string& key) const;
	
	//! Get an integer value for a setting in the InitParser object.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param section the section in which the required setting is to be found.
	//! @param key is the key which identifes the required setting with the specified section.
	//! @return the value for the specified key, or 0 if the key is not found.
	int get_int(const string& section, const string& key) const;
	
	//! Get an integer value for a setting in the InitParser object, using a default value if the
	//! requested setting is not found in the ini file.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param section the section in which the required setting is to be found.
	//! @param key is the key which identifes the required setting with the specified section.
	//! @param def the default value which is used if the requested setting is not found.
	//! @return the value for the specified key, or the value of def if the key is not found.
	int get_int(const string& section, const string& key, int def) const;

	//! Get a double value for a setting in the InitParser object.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param key is a compound key consisting of the section and 
	//! section-key separated by a colon (:), e.g. "sectionname:keyname".
	//! @return the value for the specified key, or 0.0 if the key is not found.
	double get_double(const string& key) const;
	
	//! Get a double value for a setting in the InitParser object.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param section the section in which the required setting is to be found.
	//! @param key is the key which identifes the required setting with the specified section.
	//! @return the value for the specified key, or 0.0 if the key is not found.
	double get_double(const string& section, const string& key) const;
	
	//! Get an double value for a setting in the InitParser object, using a default value if the
	//! requested setting is not found in the ini file.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param section the section in which the required setting is to be found.
	//! @param key is the key which identifes the required setting with the specified section.
	//! @param def the default value which is used if the requested setting is not found.
	//! @return the value for the specified key, or the value of def if the key is not found.
	double get_double(const string& section, const string& key, double def) const;

	//! Get a boolean value for a setting in the InitParser object.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param key is a compound key consisting of the section and 
	//! section-key separated by a colon (:), e.g. "sectionname:keyname".
	//! @return the value for the specified key, or 0 (false) if the key is not found.
	bool get_boolean(const string& key) const;
	
	//! Get a boolean value for a setting in the InitParser object.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param section the section in which the required setting is to be found.
	//! @param key is the key which identifes the required setting with the specified section.
	//! @return the value for the specified key, or 0 (false) if the key is not found.
	bool get_boolean(const string& section, const string& key) const;
	
	//! Get an boolean value for a setting in the InitParser object, using a default value if the
	//! requested setting is not found in the ini file.
	//! If the requested setting cannot be found a warning message will be printed on standard error.
	//! @param section the section in which the required setting is to be found.
	//! @param key is the key which identifes the required setting with the specified section.
	//! @param def the default value which is used if the requested setting is not found.
	//! @return the value for the specified key, or the value of def if the key is not found.
	bool get_boolean(const string& section, const string& key, bool def) const;
	
	//! Set the value of a specified setting with the provided value.
	//! @param key is a compound key consisting of the section and 
	//! section-key separated by a colon (:), e.g. "sectionname:keyname".
	//! @param val the new value to use for the specified key.
	//! @return if the entry cannot be found -1 is returned and the entry is created. Else 0 
	//! is returned.
	int set_str(const string& key, const string& val);
	
	//! Set the value of a specified setting with the provided value.
	//! @param key is a compound key consisting of the section and 
	//! section-key separated by a colon (:), e.g. "sectionname:keyname".
	//! @param val the new value to use for the specified key.
	//! @return if the entry cannot be found -1 is returned and the entry is created. Else 0 
	//! is returned.
	int set_int(const string& key, int val);
	
	//! Set the value of a specified setting with the provided value.
	//! @param key is a compound key consisting of the section and 
	//! section-key separated by a colon (:), e.g. "sectionname:keyname".
	//! @param val the new value to use for the specified key.
	//! @return if the entry cannot be found -1 is returned and the entry is created. Else 0 
	//! is returned.
	int set_double(const string& key, double val);
	
	//! Set the value of a specified setting with the provided value.
	//! @param key is a compound key consisting of the section and 
	//! section-key separated by a colon (:), e.g. "sectionname:keyname".
	//! @param val the new value to use for the specified key.
	//! @return if the entry cannot be found -1 is returned and the entry is created. Else 0 
	//! is returned.
	int set_boolean(const string& key, bool val);
	
	//! Get the number of sections in the InitParser object.
	//! @return the nuumber of sections found.
	int get_nsec(void) const;
	
	//! Get the name of the section according to it's position in the InitParser object.
	//! TODO: starting at 0 or 1?
	//! @return the name of the requested section.
	string get_secname(int n) const;
	
	//! Determine if a setting exists in an InitParser object.
	//! @return 1 if the entry exists, 0 otherwise
	int find_entry(const string& entry) const;

private:
	// Check if the key is in the form section:key and if yes create the section in the dictionnary
	// if it doesn't exist.
	void make_section_from_key(const string& key);

	void free_dico(void);	// Unalloc memory
	dictionary * dico;		// The dictionnary containing the parsed data
};

#endif // _INIT_PARSER_H_
