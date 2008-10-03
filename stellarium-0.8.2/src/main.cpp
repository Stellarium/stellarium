/*
* Stellarium
* Copyright (C) 2002 Fabien Chereau
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
#include <cstdlib>

#include "SDL.h"

#include "stellarium.h"
#include "stelapp.h"

#if defined(MACOSX) && defined(XCODE)
#include "StelConfig.h"
#endif

using namespace std;

string CDIR;	// Config Directory
string LDIR;	// Locale dir for PO translation files
string DATA_ROOT;	// Data Root Directory

// Print a beautiful console logo !!
void drawIntro(void)
{
  cout << " -------------------------------------------------------" << endl;
  cout << "[ This is "<< APP_NAME << " - http://www.stellarium.org ]" << endl;
  cout << "[ Copyright (C) 2000-2006 Fabien Chereau et al         ]" << endl;
  cout << " -------------------------------------------------------" << endl;
};


// Display stellarium usage in the console
void usage(char **argv)
{
  wcout << _("Usage: %s [OPTION] ...\n -v, --version          Output version information and exit.\n -h, --help             Display this help and exit.\n");
}



// Check command line arguments
int check_command_line(int argc, char **argv)
{
  if (argc == 2)
  {
    if (!(strcmp(argv[1],"--version") && strcmp(argv[1],"-v")))
    {
      wcout << APP_NAME << endl;
      exit(0);
    }
    if (!(strcmp(argv[1],"--help") && strcmp(argv[1],"-h")))
    {
      usage(argv);
      exit(0);
    }
  }

  if (argc == 3)
  {
    if (!(strcmp(argv[1],"--avi") && strcmp(argv[1],"-a")))
    {
      return 1;
    }	
  }

  if (argc == 5 || argc == 7 || argc == 9)
  {
    int arg;
    for (arg = 1; arg < argc; arg = arg + 2)
    {
      if (strcmp(argv[arg],"--avi") && 
        strcmp(argv[arg],"--resolution") &&
        strcmp(argv[arg],"--output") && 
        strcmp(argv[arg],"--codec"))
      {
        wcout << _("%s: Bad command line argument(s)\n");
        wcout << _("Try `%s --help' for more information.\n");
        exit(1);
      }
      else if (strcmp(argv[arg],"--codec"))
      {
        if (arg + 1 >= argc)
        {
          wcout << _("%s: Bad command line argument(s)\n");
          wcout << _("Try `%s --help' for more information.\n");
          exit(1);					
        }
      }
    }

    return 2;
  }

  if (argc == 6 || argc == 8 || argc == 10)
  {
    int arg;
    for (arg = 1; arg < argc; arg = arg + 2)
    {
      if (strcmp(argv[arg],"--avi") && 
        strcmp(argv[arg],"--resolution") &&
        strcmp(argv[arg],"--output") && 
        strcmp(argv[arg],"--codec") && 
        strcmp(argv[arg],"--onfly"))
      {
        wcout << _("%s: Bad command line argument(s)\n");
        wcout << _("Try `%s --help' for more information.\n");
        exit(1);
      }

      if (!strcmp(argv[arg],"--onfly")) arg--; // because this flag has no arguments
    }

    return 3;
  }

  return 0;
}


// Set the data, textures, and config directories in core.global : test the default
// installation dir and try to find the files somewhere else if not found there
// This enable to launch stellarium from the local directory without installing it
void setDirectories(const char* executableName)
{
#if !defined(MACOSX) && !defined(XCODE)
  // The variable CONFIG_DATA_DIR must have been set by the configure script
  // Its value is the dataRoot directory, ie the one containing data/ and textures/

  FILE * tempFile = NULL;
#if !defined(MACOSX) || ( defined(MACOSX) && !defined(XCODE) )
  // Check the presence of a file in possible data directories and set the
  // dataRoot string if the directory was found.
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
        cerr << "ERROR : I can't find the datas directories in :\n"  << CONFIG_DATA_DIR << 
          "/ nor in ./ nor in ../\nYou may fully install the software (type \"make install\" on POSIX systems)\nor launch the application from the stellarium package directory." << endl;
        MessageBox(NULL,"Can't find \"data//hipparcos.fab\" file.\nCheck your working directory.", "Stellarium Error", MB_OK | MB_ICONERROR);
        exit(-1);
      }
    }
  }
  fclose(tempFile);
  tempFile = NULL;

#else /* Compiling with XCode on MacOSX */
  char *lastSlash = rindex(executableName, '/');
  int len = lastSlash - executableName;
  char tempName[255];
  strncpy(tempName, executableName, len);
  *(tempName + len) = '\0';
  strcat(tempName, "/../Resources/");
  DATA_ROOT = string(tempName);
#endif

  // We now have a valid dataRoot directory, we can then set the data and textures dir
  LDIR = LOCALEDIR;

  // If the system is non unix (windows) or if it's macosx the config file is in the
  // config/ directory of the dataRoot directory
#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__) || defined(MINGW32) || defined(MACOSX)
  CDIR = DATA_ROOT + "/config/";
  LDIR = DATA_ROOT + "/data/locale";

  if ((tempFile = fopen((CDIR + "config.ini").c_str(),"r")))
  {
    fclose(tempFile);
  }
  else
  {
    // First launch for that user : set default options by copying the default files
    system( (string("cp ") + DATA_ROOT + "/data/default_config.ini " + CDIR + "config.ini").c_str() );
  }
#else

  // Just an indication if we are on unix/linux that we use local data files
  if (DATA_ROOT != string(CONFIG_DATA_DIR))
    printf("> Found data files in %s : local version.\n", DATA_ROOT.c_str());

  // The problem is more complexe in the case of a unix/linux system
  // The config files are in the HOME/.stellarium/ directory and this directory
  // has to be created if it doesn't exist

  // Get the user home directory
  string homeDir = getenv("HOME");
  CDIR = homeDir + "/.stellarium/";

  // If unix system, check if the file $HOME/.stellarium/config.ini exists,
  // if not, try to create it.
  if ((tempFile = fopen((CDIR + "config.ini").c_str(),"r")))
  {
    fclose(tempFile);
  }
  else
  {
    printf("Will create config files in %s\n", CDIR.c_str());
    if ((tempFile = fopen((CDIR + "config.ini").c_str(),"w")))
    {
      fclose(tempFile);
    }
    else
    {
      // Maybe the directory is not created so try to create it
      printf("Try to create directory %s\n",CDIR.c_str());
      system(string("mkdir " + CDIR).c_str());

      if ((tempFile = fopen((CDIR + "config.ini").c_str(),"w")))
      {
        fclose(tempFile);
      }
      else
      {
        cerr << "Can't create the file "<< CDIR << "config.ini\nIf the directory " << 
          CDIR << " is missing please create it by hand.\nIf not, check that you have access to " <<
          CDIR << endl;
        exit(-1);
      }
    }

    // First launch for that user : set default options by copying the default files
    system( (string("cp ") + DATA_ROOT + "/data/default_config.ini " + CDIR + "config.ini").c_str() );
  }
#endif	// Unix system

#else   // Mac OS X
  const char *cdir = NULL;
  const char *data_root = NULL;

  if (!setDirectories(&cdir, &data_root))
    exit(-1);

  CDIR = cdir;
  DATA_ROOT = data_root;
  LDIR = DATA_ROOT + "/data/locale";
#endif
}



// Main stellarium procedure
//int _tmain(int argc, char **argv)
//{

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
  //int wmain( int argc, wchar_t *argv[ ], wchar_t *envp[ ] )
  //int _twmain( int argc, wchar_t *argv[ ], wchar_t *envp[ ] )
  //{
  // Used for getting system date formatting
  LPWSTR cmdLine = ::GetCommandLineW();
  int argc = 0;
  LPWSTR* pArgs = CommandLineToArgvW(cmdLine, &argc);
  char** argv = new char*[argc];

  for(int qq = 0; qq < argc; qq++)
  {
    char* pArg = new char[::wcslen(pArgs[qq])+1];
    for(int qj = 0; qj < ::wcslen(pArgs[qq]); qj++)
    {
      pArg[qj] = (char)pArgs[qq][qj];
    }
    pArg[::wcslen(pArgs[qq])] = 0;
    argv[qq] = pArg;
  }

  setlocale(LC_TIME, "");

  bool CreateAviAndExit = false;
  bool CreateAviOnFlyAndExit = false;
  string ScriptToRun = "startup.sts";
  int resultVideoWidth = -1;
  int resultVideoHeight = -1;
  string outputPath = "";
  string codec = "";

  // Check the command line
  int checkCommandLineResult = check_command_line(argc, argv);
  if (checkCommandLineResult == 1)
  {
    CreateAviAndExit = true;
    if (argc == 3)
    {
      ScriptToRun = argv[2];
    }
  }
  else if ((checkCommandLineResult == 2)||(checkCommandLineResult == 3))
  {
    if (checkCommandLineResult == 2)
      CreateAviAndExit = true;
    else 
      CreateAviOnFlyAndExit = true;

    int arg;
    for (arg = 1; arg < argc; arg = arg + 2)
    {
      if (arg + 1 <= argc)
      {
        if (!strcmp(argv[arg],"--avi"))
        {
          ScriptToRun = argv[arg + 1];
        }
        else if (!strcmp(argv[arg],"--resolution"))
        {
          string resolutionStr = argv[arg + 1];	
          string temp;
          int multIndex = resolutionStr.find("*");
          if (multIndex != std::string::npos)
          {
            temp = resolutionStr.substr(0, multIndex);
            resultVideoWidth = atoi(temp.c_str());
            temp = resolutionStr.substr(multIndex + 1);
            resultVideoHeight = atoi(temp.c_str());
          }
          else
          {
            wcout << _("%s: Bad command line argument(s)\n");
            wcout << _("Try `%s --help' for more information.\n");
            exit(1);
          }
        }
        else if (!strcmp(argv[arg],"--output"))
        {
          outputPath = argv[arg + 1];
        }
        else if (!strcmp(argv[arg],"--codec"))
        {
          codec = argv[arg + 1];
        }
        else if (!strcmp(argv[arg],"--onfly"))
        {
          // do nothing
        }
      }
      else
      {
        wcout << _("%s: Bad command line argument(s)\n");
        wcout << _("Try `%s --help' for more information.\n");
        exit(1);
      }
    }
  }

  // Print the console logo..
  drawIntro();

  // Find what are the main Data, Textures and Config directories
  setDirectories(argv[0]);

  StelApp* app = new StelApp(CDIR, LDIR, DATA_ROOT, CreateAviAndExit, CreateAviOnFlyAndExit,
    ScriptToRun, resultVideoWidth, resultVideoHeight, outputPath, codec);

  app->init();

  app->startMainLoop();

  // Clean memory
  delete app;

  return 0;
}

