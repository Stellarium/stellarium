/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

#include <string>
#include <iostream>
#include "stellarium.h"
#include "stel_core.h"
#include "stel_sdl.h"

using namespace std;

string DDIR;	// Data Directory
string TDIR;	// Textures Directory
string CDIR;	// Config Directory
string DATA_ROOT;	// Data Root Directory

// Print a beautiful console logo !!
void drawIntro(void)
{
    printf("\n");
    printf("    -----------------------------------------\n");
    printf("    | ## ### ### #   #   ### ###  #  # # # #|\n");
    printf("    | #   #  ##  #   #   ### ##   #  # # ###|\n");
    printf("    |##   #  ### ### ### # # # #  #  ### # #|\n");
    printf("    |                     %s|\n",APP_NAME);
    printf("    -----------------------------------------\n\n");
    printf("Copyright (C) 2003 Fabien Chereau\n\n");
    printf("Please check last version and send bug report & comments \n");
    printf("on stellarium web page : http://stellarium.free.fr\n\n");
};


// Display stellarium usage in the console
void usage(char **argv)
{
	printf("Usage: %s [OPTION] ...\n -v, --version          output version information and exit\n -h, --help             display this help and exit\n", argv[0]);
}


// Check command line arguments
void check_command_line(int argc, char **argv)
{
    if (argc == 2)
    {
        if (!(strcmp(argv[1],"--version") && strcmp(argv[1],"-v")))
        {
            printf("%s\n", APP_NAME);
            exit(0);
        }
        if (!(strcmp(argv[1],"--help") && strcmp(argv[1],"-h")))
        {
            usage(argv);
            exit(0);
        }
    }

    if (argc > 1)
    {
        printf("%s: Bad command line argument(s)\n", argv[0]);
        printf("Try `%s --help' for more information.\n", argv[0]);
        exit(1);
    }
}


// Set the data, textures, and config directories in core.global : test the default
// installation dir and try to find the files somewhere else if not found there
// This enable to launch stellarium from the local directory without installing it
void setDirectories(void)
{
	// The variable CONFIG_DATA_DIR must have been set by the configure script
	// Its value is the dataRoot directory, ie the one containing data/ and textures/

	// Check the presence of a file in possible data directories and set the
	// dataRoot string if the directory was found.
    FILE * tempFile = NULL;
	tempFile = fopen((string(CONFIG_DATA_DIR) + "/data/hipparcos.fab").c_str(),"r");
	if (tempFile)
	{
		DATA_ROOT = string(CONFIG_DATA_DIR);
	}
	else
	{
		tempFile = fopen("./data/hipparcos.fab","r");
		if (tempFile)
		{
			DATA_ROOT = ".";
		}
		else
		{
			tempFile = fopen("../data/hipparcos.fab","r");
			if (tempFile)
			{
				DATA_ROOT = "..";
			}
			else
			{
            	// Failure....
            	printf("ERROR : I can't find the datas directories in :\n");
            	printf("%s/ nor in ./ nor in ../\n",CONFIG_DATA_DIR);
                printf("You may fully install the software (on POSIX systems)\n");
                printf("or launch the application from the stellarium package directory.\n");
                exit(-1);
			}
		}
	}
    fclose(tempFile);
	tempFile = NULL;

	// We now have a valid dataRoot directory, we can then set the data and textures dir
	DDIR = DATA_ROOT + "/data/";
	TDIR = DATA_ROOT + "/textures/";

	// If the system is non unix (windows) or if it's macosx the config file is in the
	// config/ directory of the dataRoot directory
#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__) || defined(MACOSX)
	CDIR = DATA_ROOT + "/config/";

	if ((tempFile = fopen((CDIR + "config.txt").c_str(),"r")))
	{
		fclose(tempFile);
	}
	else
	{
		// First launch for that user : set default options by copying the default files
		system( (string("cp ") + CDIR + "default_config.txt " + CDIR + "config.txt").c_str() );
	}
#else

	// Just an indication if we are on unix/linux that we use local data files
	if (DATA_ROOT != string(CONFIG_DATA_DIR))
		cout << ">Found data files in " << DATA_ROOT << " : local version." << endl;

	// The problem is more complexe in the case of a unix/linux system
	// The config files are in the HOME/.stellarium/ directory and this directory
	// has to be created if it doesn't exist

	// Get the user home directory
	string homeDir = getenv("HOME");
	CDIR = homeDir + "/.stellarium/";

	// If unix system, check if the file $HOME/.stellarium/config.txt exists,
	// if not, try to create it.
	if ((tempFile = fopen((CDIR + "config.txt").c_str(),"r")))
	{
		fclose(tempFile);
	}
	else
	{
		cout << "Will create config files in " << CDIR << endl;
		if ((tempFile = fopen((CDIR + "config.txt").c_str(),"w")))
		{
			fclose(tempFile);
		}
		else
		{
			// Maybe the directory is not created so try to create it
			cout << "Try to create directory " << CDIR << endl;
			system(string("mkdir " + CDIR).c_str());

			if ((tempFile = fopen((CDIR + "config.txt").c_str(),"w")))
			{
				fclose(tempFile);
			}
			else
			{
				cout << "Can't create the file " << CDIR << "config.txt" << endl;
				cout << "If the directory " << CDIR << " is missing please create it by hand." << endl;
				cout << "If not check that you have access to " << CDIR << endl;
				exit(-1);
			}
		}

		// First launch for that user : set default options by copying the default files
		system( (string("cp ") + DATA_ROOT + "/config/default_config.txt " + CDIR + "config.txt").c_str() );
	}
#endif	// Unix system

}



// Main stellarium procedure
int main(int argc, char **argv)
{
	// Check the command line
	check_command_line(argc, argv);

	// Print the console logo..
    drawIntro();

	// Find what are the main Data, Textures and Config directories
    setDirectories();

	// Create the core of stellarium, it has to be initialized
	stel_core* core = new stel_core();

	core->set_directories(DDIR, TDIR, CDIR, DATA_ROOT);

	// Give the config file parameters which has to be given "hard coded"
	core->set_config_files("config.txt", "location.txt");

	// Load the configuration options from the given file names
	// This includes the video parameters
	core->load_config();

	// Create a stellarium sdl manager
	stel_sdl sdl_mgr(core);

	// Initialize video device and other sdl parameters
	sdl_mgr.init();

	core->init();

	// Start the main loop until the end of the execution
	sdl_mgr.start_main_loop();

	// Clean memory
	delete core;

	return 0;
}

