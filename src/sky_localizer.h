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

#ifndef _SKY_LOCALIZER_H_
#define _SKY_LOCALIZER_H_

#include <stdio.h>
#include <string>
#include <map>

using namespace std;

typedef std::map< std::string, std::string > stringHash_t;
typedef stringHash_t::const_iterator stringHashIter_t;

class Sky_localizer
{

 public:
  Sky_localizer(string _data_dir);
  virtual ~Sky_localizer();

  string get_sky_culture_list(void);
  string convert_directory_to_sky_culture(string _directory);
  string convert_sky_culture_to_directory(string _name);

  string get_sky_locale_list(void);
  string convert_locale_to_name(string _locale);
  string convert_name_to_locale(string _name);

 private:
  stringHash_t name_to_dir;
  stringHash_t dir_to_name;

  stringHash_t name_to_locale;
  stringHash_t locale_to_name;

};


#endif // _SKY_LOCALIZER_H_
