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

#ifndef _STEL_COMMAND_INTERFACE_H_
#define _STEL_COMMAND_INTERFACE_H_

#include "command_interface.h"
#include "stel_core.h"
#include "script_mgr.h"
#include "audio.h"

using namespace std;

// Predeclaration of the stel_core class
class stel_core;



class StelCommandInterface : CommandInterface
{

 public:
  StelCommandInterface(stel_core * core);
  virtual ~StelCommandInterface();
  virtual int StelCommandInterface::execute_command(string commandline);
  virtual int execute_command(string command, unsigned long int &wait);
  
 private:
  stel_core * stcore;
  Audio * audio;  // for audio track from script
};

// TODO: move elsewhere
int string_to_jday(string date, double &jd);

double str_to_double(string str);
int str_to_int(string str);
string double_to_str(double dbl);

#endif // _STEL_COMMAND_INTERFACE_H
