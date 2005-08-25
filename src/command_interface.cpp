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

#include "command_interface.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

//#define PARSE_DEBUG 1

CommandInterface::CommandInterface() {
}

CommandInterface::~CommandInterface() {
}

  
int CommandInterface::parse_command(string command_line, string &command, stringHash_t &arguments) {

   istringstream commandstr( command_line );
   string key, value, temp;
   char nextc;

   commandstr >> command;
   transform(command.begin(), command.end(), command.begin(), ::tolower);

   while(commandstr >> key >> value ) {
	   if(value[0] == '"') {
		   // pull in all text inside quotes

		   if(value[value.length()-1] == '"') {
			   // one word in quotes
			   value = value.substr(1, value.length() -2 );
		   } else {
			   // multiple words in quotes
			   value = value.substr(1, value.length() -1 );

			   while(1){
				   nextc = commandstr.get();
				   if( nextc == '"' || !commandstr.good()) break;
				   value.push_back( nextc );
			   }
		   }
	   } 
	   
	   transform(key.begin(), key.end(), key.begin(), ::tolower);
	   arguments[key] = value;
	   
   }
   

#ifdef PARSE_DEBUG

  cout << "Command: " << command << endl << "Argument hash:" << endl;

   for ( stringHashIter_t iter = arguments.begin(); iter != arguments.end(); ++iter ) {
     cout << "\t" << iter->first << " : " << iter->second << endl;
  }

#endif

   return 1;  // no error checking yet
     
}
    

// for quick testing set to 1 and compile just this file
#if 0


main() {

  string command;
  stringHash_t args;
  int status;

  string commandline("flag art on atmosphere off location \"West Virginia\"");

  // if static...
  status = CommandInterface::parse_command(commandline, command, args);

}


#endif
