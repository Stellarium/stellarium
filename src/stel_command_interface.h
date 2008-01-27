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
#include "StelUtils.hpp"
#include "StelModule.hpp"

using namespace std;

// Predeclaration of the StelCore class
class StelCore;
class StelApp;

class StelCommandInterface : CommandInterface, public StelModule
{

 public:
  StelCommandInterface(StelCore * core, StelApp * app);
  virtual ~StelCommandInterface();
  virtual void init() {return;}
  virtual double draw(StelCore* core) {return 0;}
  
  virtual int execute_command(string commandline);
  virtual int execute_command(string command, double arg);
  virtual int execute_command(string command, int arg);
  virtual int execute_command(string command, unsigned long int &wait, bool trusted);
  virtual int set_flag(string name, string value, bool &newval, bool trusted);
  virtual void update(double delta_time);

 private:
	int FlagTimePause;
	double temp_time_velocity;			// Used to store time speed while in pause
	
  StelCore * stcore;
  StelApp * stapp;
  class Audio * audio;  // for audio track from script
};


#endif // _STEL_COMMAND_INTERFACE_H
