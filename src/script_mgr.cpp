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


#include <iostream>
#include <dirent.h>
#include <cstdio>

#include "script_mgr.h"
#include "script.h"
#include "stel_command_interface.h"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"

using namespace std;


ScriptMgr::ScriptMgr(StelCommandInterface *command_interface) : play_paused(false) {
	commander = command_interface;
	recording = 0;
	playing = 0;
	record_elapsed_time = 0;

	// used when scripts are on a CD that needs to be mounted manually
	RemoveableScriptDirectory = "";
	RemoveableDirectoryMounted = 0;
}

ScriptMgr::~ScriptMgr() {
}

// path is used for loading script assets 
bool ScriptMgr::play_script(string script_file, string script_path) {
	// load script...
	
	if(playing){
		// cancel current script and start next (one script can call another)
		cancel_script();
	}

	set_gui_debug(0);  // Default off until script sets otherwise

	//if (script) delete script;
	script = new Script();

	// if script is on mountable disk, mount that now
	if( RemoveableScriptDirectory != "" &&
		script_file.compare(0,RemoveableScriptDirectory.length(), RemoveableScriptDirectory) ==0) {
		try
		{
			system(StelApp::getInstance().getFileMgr().findFile("data/script_mount_script_disk" ).string().c_str());	  
			cout << "MOUNT DISK to read script" << endl;
			RemoveableDirectoryMounted = 1;
		}
		catch(exception& e)
		{
			cerr << "ERROR while trying to mount removable media: " << e.what() << endl;		
		}
	}

	if( script->load(script_file, script_path) ) {
		playing = 1;
		play_paused = 0;
		elapsed_time = wait_time = 0;
		return 1;

	} else {
		cancel_script();
		return 0;
	}
}

void ScriptMgr::cancel_script() {
	// delete script object...
	if(script) delete script;
	script = NULL;
	// images loaded are deleted from stel_command_interface directly
	playing = 0;
	play_paused = 0;
	
	if(RemoveableDirectoryMounted) {
		try
		{
			system(StelApp::getInstance().getFileMgr().findFile("data/script_unmount_script_disk" ).string().c_str());	  
			cout << "UNMOUNT DISK" << endl;
			RemoveableDirectoryMounted = 0;
		}
		catch(exception& e)
		{
			cerr << "ERROR while trying to mount removable media: " << e.what() << endl;		
		}
	}
}


void ScriptMgr::pause_script() {
	play_paused = 1;
	// need to pause time as well
	commander->execute_command("timerate action pause");
}

void ScriptMgr::resume_script() { 
	if(play_paused) {
		play_paused = 0;
		commander->execute_command("timerate action resume");
	}
}

void ScriptMgr::record_script(string script_filename) {

	// TODO: filename should be selected in a UI window, but until then this works
	if(recording) {
		cout << "Already recording script." << endl;
		return;
	}

	if(script_filename != "") {
		rec_file.open(script_filename.c_str(), fstream::out);
	} else {

		string sdir;
#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__)
		if(getenv("USERPROFILE")!=NULL){
			//for Win XP etc.
			sdir = string(getenv("USERPROFILE")) + "\\My Documents\\";
		}else{
			//for Win 98 etc.
			sdir = "C:\\My Documents\\";
		}
#else
		sdir = string(getenv("HOME")) + "/";
#endif
#ifdef MACOSX
		sdir += "/Desktop/";
#endif

		// add a number to be unique
		char c[3];
		FILE * fp;
		for(int j=0; j<=100; ++j)
		{
#ifndef _MSC_VER
			snprintf(c,3,"%d",j);
#else
			_snprintf(c,3,"%d",j);
#endif

			script_filename = sdir + "stellarium" + c + ".sts";
			fp = fopen(script_filename.c_str(), "r");
			if(fp == NULL)
				break;
			else
				fclose(fp);
		}
	
		rec_file.open(script_filename.c_str(), fstream::out);

	}
	

	if(rec_file.is_open()) {
		recording = 1;
		record_elapsed_time = 0;
		cout << "Now recording actions to file: " << script_filename << endl;
		rec_filename = script_filename;
	} else {
		cout << "Error opening script file for writing: " << script_filename << endl;
		rec_filename = "";
	} 
}

void ScriptMgr::record_command(string commandline) {

	if(recording) {
		// write to file...

		if(record_elapsed_time) {
			rec_file << "wait duration " << record_elapsed_time/1000.f << endl;
			record_elapsed_time = 0;
		}
    
		rec_file << commandline << endl;

		// TEMPORARY for debugging
		cout << commandline << endl;
	}
}

void ScriptMgr::cancel_record_script() {
	// close file...
	rec_file.close();
	recording = 0;

	cout << "Script recording stopped." << endl;
}
	 

// runs maximum of one command per update 
// note that waits can drift by up to 1/fps seconds
void ScriptMgr::update(double delta_time) 
{

	if(recording) record_elapsed_time += (unsigned long int)delta_time;

	if(playing && !play_paused) 
	{

		elapsed_time += (unsigned long int) delta_time;  // time elapsed since last command (should have been) executed

		if(elapsed_time >= wait_time) 
		{
			// now time to run next command


			//      cout << "dt " << delta_time << " et: " << elapsed_time << endl;
			elapsed_time -= wait_time;
			string comd;
			unsigned long int wait;
			if(script->next_command(comd)) 
			{
				commander->execute_command(comd, wait, 0);  // untrusted commands
				wait_time = wait; 

			} else {
				// script done
				cout << "Script completed." << endl;
				commander->execute_command("script action end");
			}
		}
	}

}  

/****************************************************************************
 get a list of script files from directory.  If directory is equal to the
 removable media directory, the mount and unmount scripts are called
 before and after the file listing is done.  Note the removable media
 directory must be a complete path (in the boost::filesystem sense) - not a
 relative path.
 returns a string list of script file names, delimited with the '\n' 
 character
****************************************************************************/
string ScriptMgr::get_script_list(string directory) {
	string result="";
	// if directory is on mountable disk, mount that now
	if(RemoveableScriptDirectory!="" 
	   && directory.compare(0,RemoveableScriptDirectory.length(), RemoveableScriptDirectory)==0) 
	{
		try
		{
			system(StelApp::getInstance().getFileMgr().findFile("data/script_mount_script_disk").string().c_str());	  
			cout << "MOUNT DISK to read directory\n";
			directory += "/scripts";
			RemoveableDirectoryMounted = 1;
		}
		catch(exception& e)
		{
			cerr << "ERROR while trying to mount removable media: " << e.what() << endl;
		}
	}

	try
	{
		set<string> scriptList = StelApp::getInstance().getFileMgr().listContents(directory);
		for(set<string>::iterator i=scriptList.begin(); i!=scriptList.end(); i++)
		{
			// stl::sets are automagically sorted, so we just need to filter for .sts filenames
			if ( (*i).substr((*i).length() - 4, 4) == ".sts" )
			{
				result = result + *i + '\n';
			}
		}
	}
	catch(exception& e)
	{
		cerr << "ERROR while listing scripts: " << e.what() << endl;
	}
	
	if(RemoveableDirectoryMounted) {
		try
		{
			system(StelApp::getInstance().getFileMgr().findFile("data/script_unmount_script_disk").string().c_str());	  
			cout << "UNMOUNT DISK" << endl;
			RemoveableDirectoryMounted = 0;
		}
		catch(exception& e)
		{
			cerr << "ERROR while trying to unmount removable media: " << e.what() << endl;
		}
	}

	return result;
}

string ScriptMgr::get_script_path() { 
	if(script) return script->get_path(); 
	return ""; 
}


// look for a script called "startup.sts"
bool ScriptMgr::play_startup_script() {

	// first try on removeable directory
	if(RemoveableScriptDirectory !="" &&
	   play_script(RemoveableScriptDirectory + "/scripts/startup.sts", 
				   RemoveableScriptDirectory + "/scripts/")) {
	} 
	else {
		try {
			boost::filesystem::path fullPath(StelApp::getInstance().getFileMgr().findFile("data/scripts/startup.sts")); 
			boost::filesystem::path parentDir(fullPath / "..");
			play_script(fullPath.string(), parentDir.normalize().string() + "/");
		}
		catch(exception& e)
		{
			cerr << "ERROR running startup.sts script: " << e.what() << endl;
		}
	}
	return 1;
}
