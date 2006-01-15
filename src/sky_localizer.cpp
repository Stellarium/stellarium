/*
 * Stellarium
 * This file Copyright (C) 2004 Robert Spearman
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

#include <iostream>
#include <fstream>
#include "sky_localizer.h"
#include "stellarium.h"
#include "translator.h"

SkyLocalizer::SkyLocalizer(string _data_dir)
{

	// load list of sky cultures from disk
  char culture_name[100];
  char culture_directory[100];

  string fileName = _data_dir + "skycultures.fab";

  FILE * fic = fopen(fileName.c_str(),"r");
  if (!fic) {
    printf("Can't open %s\n",fileName.c_str());
    fclose(fic);
    return;
  }

  char line[256];
  char* s;
  while(!feof(fic)) {
    while (fgets(line, 255, fic)) {
      line[strlen(line)-1] = 0;     // strip LF
      s = strtok(line, "\t");
      strcpy(culture_name, s);

      s = strtok(NULL, "\t");
      strcpy(culture_directory, s);

      //      printf("Name: %s\tDir: %s\n", culture_name, culture_directory);
      dir_to_name[string(culture_directory)] = _(culture_name);
    }

  }
  fclose(fic);
}

SkyLocalizer::~SkyLocalizer(void) {
}


// is this a valid culture directory?
bool SkyLocalizer::test_sky_culture_directory(string _culture_dir) {
	return (dir_to_name[_culture_dir] != L"");
}

// returns newline delimited list of human readable culture names
wstring SkyLocalizer::get_sky_culture_list(void){

	wstring cultures;
	for ( wstringHashIter_t iter = dir_to_name.begin(); iter != dir_to_name.end(); ++iter ) {
		cultures += iter->second + L"\n";
	}

	return cultures;
}

wstring SkyLocalizer::convert_directory_to_sky_culture(string _directory)
{
	return dir_to_name[_directory];
}


string SkyLocalizer::convert_sky_culture_to_directory(wstring _name)
{
	for ( wstringHashIter_t iter = dir_to_name.begin(); iter != dir_to_name.end(); ++iter )
	{
		if (iter->second == _name) return iter->first;
	}
	return "";
}
