/*
 * Stellarium
 * This file Copyright (C) 2005 Robert Spearman
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


#include "script.h"


Script::Script()
{
	input_file = NULL;
	path = "";
}

Script::~Script()
{
	if(input_file)
		delete input_file;  // closes automatically
}

int Script::load(string script_file, string script_path)
{
	input_file = new ifstream(script_file.c_str());

	if (! input_file->is_open())
	{
		cout << _("Unable to open script ") << script_file << endl;
		return 0;
	}
	path = script_path;

	// TODO check first line of file for script identifier... ?

	return 1;
}

int Script::next_command(string &command)
{

	char buffer[1000];

	while (! input_file->eof() )
	{
		input_file->getline (buffer,999);

		if(input_file->eof())
			return 0;

		if( buffer[0] != '#' && buffer[0] != 0)
		{

			//      printf("Buffer is: %s\n", buffer);


			command = string(buffer);
			return 1;
		}
	}

	return 0;
}


