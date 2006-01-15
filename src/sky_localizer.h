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

#include <cstdio>
#include <string>
#include "stel_utility.h"

using namespace std;

class SkyLocalizer
{

 public:
  SkyLocalizer(string _data_dir);
  virtual ~SkyLocalizer();

  wstring get_sky_culture_list(void);
  wstring convert_directory_to_sky_culture(string _directory);
  string convert_sky_culture_to_directory(wstring _name);
  bool test_sky_culture_directory(string _culture_dir);

 private:
  wstringHash_t dir_to_name;

};


#endif // _SKY_LOCALIZER_H_
