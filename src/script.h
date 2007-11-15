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

// This class handles loading and playing a script of recorded commands


#ifndef _SCRIPT_H_
#define _SCRIPT_H_

using namespace std;

#include <fstream>
#include <string>
#include <QString>
				 
class Script
{

 public:
  Script();
  ~Script();
  int load(QString script_file, QString script_path);         // open a script file
  int next_command(string &command);    // retreive next command to execute
  QString get_path() { return path; };

 private:
  ifstream * input_file;
  QString path;

};


#endif // _SCRIPT_H
