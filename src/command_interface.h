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

/* This class handles parsing of a simple command syntax for scripting, 
   UI components, network commands, etc.
*/   

#ifndef _COMMAND_INTERFACE_H_
#define _COMMAND_INTERFACE_H_

#include <cstdio>
#include <string>
#include <map>
#include "StelUtils.hpp"

using namespace std;

class CommandInterface
{

 public:
  CommandInterface();
  virtual ~CommandInterface();
  virtual int execute_command(string command) = 0;
  
  protected:
  int parse_command(string command_line, string &command, stringHash_t &arguments);

};


#endif // _COMMAND_INTERFACE_H
