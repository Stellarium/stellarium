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

#ifndef _SETTING_MGR_H_
#define _SETTING__MGR_H_

#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include "vecmath.h"

typedef std::map< std::string, std::string > stringHash_t;
typedef std::map< std::string, double > doubleHash_t;
typedef std::map< std::string, Vec3d > vectorHash_t;

typedef stringHash_t::const_iterator stringHashIter_t;

// manages the collection of configuration settings


class SettingMgr
{

 public:
  SettingMgr();
  //  SettingMgr(const SettingMgr& c);
  ~SettingMgr();
  void set(std::string name, std::string value);
  void set(std::string name, double value);
  void set(std::string name, Vec3d value);
  std::string get_string(std::string name) { return config_string[name]; };
  double get_double(std::string name) { return config_double[name]; };
  Vec3d get_vector(std::string name) { return config_vector[name]; };
  bool is_true(std::string name) { return config_double[name]!=0; };
  void print_values();
  void create_commands(std::string &out);
  // int read_config
  // int write_config

 private:
  stringHash_t config_string;
  doubleHash_t config_double;
  vectorHash_t config_vector;
};


#endif // _SETTING_MGR_H
