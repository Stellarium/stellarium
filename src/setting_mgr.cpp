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

#include "setting_mgr.h"


SettingMgr::SettingMgr() {

}

SettingMgr::~SettingMgr() {

}

void SettingMgr::set(std::string name, std::string value) {
  config_string[name] = value;

  // attempt to parse into double and vector as well
  std::istringstream vstr( value );
  vstr >> config_double[name];

  char tmp;
  std::istringstream tstr( value );
  tstr >> config_vector[name][0] >> tmp 
       >> config_vector[name][1] >> tmp 
       >> config_vector[name][2];
}

void SettingMgr::set(std::string name, double value) {
  config_double[name] = value;

  std::ostringstream oss;
  oss << value;
  config_string[name] = oss.str();
}

void SettingMgr::set(std::string name, Vec3d value) {
  
  std::ostringstream oss;
  oss << value[0] << "," << value[1] << "," << value[2];
  config_string[name] = oss.str();

  config_vector[name] = value;
}


void SettingMgr::print_values() {
  
  for ( stringHashIter_t iter = config_string.begin(); iter != config_string.end(); ++iter ) {
    std::cout << iter->first << " : " << iter->second << std::endl;
  }

}

// int read_config
// int write_config

