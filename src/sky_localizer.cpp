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

Sky_localizer::Sky_localizer(string _data_dir)
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

      name_to_dir[string(culture_name)] = string(culture_directory);
      dir_to_name[string(culture_directory)] = string(culture_name);
    }

  }
  fclose(fic);


  // Load list of languages from disk
  char buffer[1000];

  ifstream input_file(string(_data_dir + "skylanguages.fab").c_str());

  if (! input_file.is_open()) {
	  cout << "Error opening " + _data_dir + "skylanguages.fab" << endl;
	  return;
  }

  string lcode, lname;
  while (!input_file.getline(buffer,999).eof()) {
	  
	  if( buffer[0] != '#' && buffer[0] != '\n') {
		
		  istringstream fline( buffer );

		  if(fline >> lcode >> lname) {
			  master_locales[lcode] = lname;
			  // cout << lcode << " : " << lname << endl;
		  }
		  
	  }
  }


  // update language translation hash
  init_sky_locales();

}

Sky_localizer::~Sky_localizer(void) {
}


// call whenever need to initialize with current translations
void Sky_localizer::init_sky_locales() {

	locale_to_name.clear();
	name_to_locale.clear();

	// initialize sky locale lists by translating locale names from master list

	for ( stringHashIter_t iter = master_locales.begin(); iter != master_locales.end(); ++iter ) {
		locale_to_name[ iter->first ] = _(iter->second.c_str());
		name_to_locale[ _(iter->second.c_str()) ] = iter->first;
		// printf("name: %s\tlocale: %s\n", iter->second.c_str(), iter->first.c_str());
	}
	
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

	init_sky_locales();  // make sure up to date translations

	string locales;
	for ( stringHashIter_t iter = name_to_locale.begin(); iter != name_to_locale.end(); ++iter ) {
		locales += iter->first + "\n" + iter->second + "\n";
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

string Sky_localizer::clean_sky_locale_name(string _locale) {
#if !defined(MACOSX) && !defined(WIN32)
	// if locale is "system_default" try to use language from 
	// user's environment locale, otherwise default to English
	if( _locale == "system_default" ) {
		// read current ui locale
		char *tmp = setlocale(LC_MESSAGES, "");
		string ltmp(tmp);
		string language = ltmp.substr(0,ltmp.find('_'));
		//		printf("Language code is %s\n", language.c_str());

		// temporary - TODO: this hash should be created from a text file
		stringHash_t locale_to_lang;
		locale_to_lang["en"] = "eng";
		locale_to_lang["fr"] = "fra";
		locale_to_lang["de"] = "deu";
		locale_to_lang["es"] = "esl";
		locale_to_lang["pt"] = "por";
		locale_to_lang["nl"] = "dut";
		locale_to_lang["it"] = "ita";
		
		_locale = locale_to_lang[language];
		
		cout << _("Using sky language from environment locale: ") << _locale << endl;

		if( _locale == "" ) {
			cout << _("Did not recognize locale language code ") <<
				language << _(". Defaulting to english sky labels\n");
			_locale = "eng";  // default
		}
	}
#else
     _locale = "eng";  // default
#endif
	return _locale;

}
