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


#include "sky_localizer.h"
#include "stellarium.h"

Sky_localizer::Sky_localizer(string _data_dir)
{
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

      name_to_dir[string(culture_name)] = string(culture_directory);
      dir_to_name[string(culture_directory)] = string(culture_name);
    }

  }
  fclose(fic);


  // initialize sky locale list (hardcoded right here)
  locale_to_name["eng"] = _("English");
  locale_to_name["esl"] = _("Spanish");
  locale_to_name["fra"]  = _("French");
  locale_to_name["haw"]  = _("Hawaiian");

  for ( stringHashIter_t iter = locale_to_name.begin(); iter != locale_to_name.end(); ++iter ) {
    name_to_locale[ iter->second ] = iter->first;
    //    printf("name: %s\tlocale: %s\n", iter->second.c_str(), iter->first.c_str());
  }


}

Sky_localizer::~Sky_localizer(void) {
}

// returns newline delimited list of human readable culture names
string Sky_localizer::get_sky_culture_list(void){

  string cultures;
  for ( stringHashIter_t iter = name_to_dir.begin(); iter != name_to_dir.end(); ++iter ) {
    cultures += iter->first + "\n";
  }

  return cultures;

}

string Sky_localizer::convert_directory_to_sky_culture(string _directory){

  return dir_to_name[_directory];

}


string Sky_localizer::convert_sky_culture_to_directory(string _name){

  return name_to_dir[_name];

}




// returns newline delimited list of human readable culture names
string Sky_localizer::get_sky_locale_list(void){

  string locales;
  for ( stringHashIter_t iter = name_to_locale.begin(); iter != name_to_locale.end(); ++iter ) {
    locales += iter->first + "\n";
  }

  return locales;

}

// locale is used by code, locale name is human readable
// e.g. fra = French
string Sky_localizer::convert_locale_to_name(string _locale){

  return locale_to_name[_locale];

}


string Sky_localizer::convert_name_to_locale(string _name){

  return name_to_locale[_name];

}


