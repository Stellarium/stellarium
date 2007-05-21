#include "config.h"
#include "StelFileMgr.hpp"
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/random/linear_congruential.hpp>
	
using namespace std;
namespace fs = boost::filesystem;

#define CHECK_FILE "data/ssystem.ini"

StelFileMgr::StelFileMgr()
{
	try
	{
		fileLocations.push_back(getUserDir());
	}
	catch(exception &e)
	{
		cerr << "WARNING: could not locate user directory" << endl;
	}
	
	try
	{
		fileLocations.push_back(getInstallationDir());
	}
	catch(exception &e)
	{
		cerr << "WARNING: could not locate installation directory" << endl;
	}

	cerr << "DEBUG: StelFileMgr::fileLocations path is (StelFileMgr::StelFileMgr()):" << endl;		   
	for(vector<fs::path>::iterator i = fileLocations.begin();
		   i != fileLocations.end();
		   i++)
	{
		cerr << "  + " << (*i).string() << endl;
	}
	cerr << "  [end search paths]" << endl;
}

StelFileMgr::~StelFileMgr()
{
}

const fs::path StelFileMgr::findFile(const string& path, const FLAGS& flags)
{
	if (path[0] == '.')
	{
		if (fileFlagsCheck(path, flags)) 
			return fs::path(path);
	}
	
	if ( fs::path(path).is_complete() )
		if ( fileFlagsCheck(path, flags) )
			return(path);
		else 
		{
			throw(runtime_error("file does not match flags: " + path));
		}
		
	fs::path result;
	for(vector<fs::path>::iterator i = fileLocations.begin();
		   i != fileLocations.end();
		   i++)
	{				
		if (fileFlagsCheck(*i / path, flags))
			return fs::path(*i / path);
	}
	
	throw(runtime_error("file not found: " + path));
}

const set<string> StelFileMgr::listContents(const string& path, const StelFileMgr::FLAGS& flags)
{
	set<string> result;
	vector<fs::path> listPaths;
	
	// If path is "complete" (a full path), we just look in there, else
	// we append relative paths to the search paths maintained by this class.
	if ( fs::path(path).is_complete() )
		listPaths.push_back("");
	else
		listPaths = fileLocations;
	
	for (vector<fs::path>::iterator li = listPaths.begin();
	     li != listPaths.end();
	     li++)
	{
		if ( fs::exists(*li / path) ) {
			if ( fs::is_directory(*li / path) )
			{ 
				fs::directory_iterator end_iter;
				for ( fs::directory_iterator dir_itr( *li / path );
						dir_itr != end_iter;
						++dir_itr )
				{
					// default is to return all objects in this directory
					bool returnThisOne = true;
				
					// but if we have flags set, that will filter the result
					fs::path fullPath(*li / path / dir_itr->leaf());
					if ( (flags & WRITABLE) && ! isWritable(fullPath) )
						returnThisOne = false;
				
					if ( (flags & DIRECTORY) && ! fs::is_directory(fullPath) )
						returnThisOne = false;
				
					// OK, add the ones we want to the result
					if ( returnThisOne )
						result.insert(dir_itr->leaf());
				}
			}
		}
	}

	return result;
}

void StelFileMgr::setSearchPaths(const vector<fs::path> paths)
{
	fileLocations = paths;
	cerr << "DEBUG: StelFileMgr::fileLocations path is (StelFileMgr::setSearchPaths(...)):" << endl;		   
	for(vector<fs::path>::iterator i = fileLocations.begin();
		   i != fileLocations.end();
		   i++)
	{
		cerr << "  + " << (*i).string() << endl;
	}
	cerr << "  [end search paths]" << endl;
}

bool StelFileMgr::exists(const fs::path& path)
{
	return fs::exists(path);
}

/**
 * Hopefully portable way to implement testing of write permission
 * to a file or directory.
 */
bool StelFileMgr::isWritable(const fs::path& path)
{
	bool result = false;
		
	if ( fs::exists ( path ) )
	{
		// For directories, we consider them writable if it is possible to create a file
		// inside them.  We will try to create a file called "stmp[randomnumber]"
		// (first without appending a random number, but if that exist, we'll re-try
		// 10 times with different random suffixes).
		// If this is possible we will delete it and return true, else we return false.
		// We'll use the boost random number library for portability
		if ( fs::is_directory ( path ) )
		{
			// cerr << "DEBUG StelFileMgr::isWritable: exists and is directory: " << path.string() << endl;
			boost::rand48 rnd(boost::int32_t(3141));
			int triesLeft = 50;
			ostringstream outs;
			
			outs << "stmp";
			fs::path testPath(path / outs.str());
					
			while(fs::exists(testPath) && triesLeft-- > 0)
			{
				// cerr << "DEBUG StelFileMgr::isWritable: test path " << testPath.string() << " already exists, trying another" << endl;
				outs.str("");
				outs << "stmp" << rnd();
				testPath = path / outs.str();
			}
			
			if (fs::exists(testPath))
			{
				// cerr << "DEBUG StelFileMgr::isWritable: I reall tried, but I can't make a unique non-existing file name to test" << endl;
				result = false;
			}
			else
			{
				// OK, we have a non-exiting testPath now.  We just call isWritable for that path
				// cerr << "DEBUG StelFileMgr::isWritable: testing non-existing file in dir: " << testPath.string() << endl;
				result = isWritable(testPath);
			}
		}
		else {
			// try to open writable (without damaging file)
			// cerr << "DEBUG StelFileMgr::isWritable: exists and is file: " << path.string() << endl;
			ofstream f ( path.native_file_string().c_str(), ios_base::out | ios_base::app );	
			if ( f )
			{
				result = true;
				f.close();
			}
		}
	}
	else
	{
		// try to create file to see if it's possible.
		// cerr << "DEBUG StelFileMgr::isWritable: does not exist: " << path.string() << endl;
		ofstream f ( path.native_file_string().c_str(), ios_base::out | ios_base::app );
		if ( f )
		{
			f.close();
			fs::remove (path);
			result = true;
		}
	}
	
	return result;
}

bool StelFileMgr::isDirectory(const fs::path& path)
{
	return fs::is_directory(path);
}

void StelFileMgr::checkUserDir()
{
	try {
		if (fs::exists(getUserDir()))
		{
			if (isDirectory(getUserDir()) && isWritable(getUserDir()))
			{
				// everything checks out fine.
				return;
			}
			else
			{
				cerr << "ERROR: user directory is not a writable directory: " << getUserDir().string() << endl;
				exit(1);
			}
		}
		else
		{
			// The user directory doesn't exist, lets create it.
			system(string("mkdir " + getUserDir().string()).c_str());
			
			// And verify that it was created
			if (!fs::exists(getUserDir()))
			{
				cerr << "ERROR: could not create user directory: " << getUserDir().string() << endl;
				exit(1);
			}
		}
		
	}
	catch(exception& e)
	{
		// This should never happen  ;)
		cerr << "ERROR: cannot work out the user directory" << endl;
		exit(1);
	}	
}

bool StelFileMgr::fileFlagsCheck(const fs::path& path, const FLAGS& flags)
{
	if ( ! (flags & HIDDEN) )
	{
		// Files are considered HIDDEN on POSIX systems if the file name begins with 
		// a "." character.  Unless we have the HIDDEN flag set, reject and path
		// where the basename starts with a .
		if (basename(path)[0] == '.')
		{
			return(false);
		}
	}
	
	if ( flags & NEW )
	{
		// if the NEW flag is set, we check to see if the parent is an existing directory
		// which is writable
		fs::path parent(path / "..");
		if ( ! isWritable(parent.normalize()) || ! fs::is_directory(parent.normalize()) )
		{
			return(false);	
		}						
	}
	else if ( fs::exists(path) )
	{
		if ( (flags & WRITABLE) && ! isWritable(path) )
			return(false);
			
		if ( (flags & DIRECTORY) && ! fs::is_directory(path) )
			return(false);
			
		if ( (flags & FILE) && fs::is_directory(path) )
			return(false); 			
	}
	else
	{
		// doesn't exist and NEW flag wasn't requested
		return(false);
	}
		
	return(true);
}

// Platform dependent members with compiler directives in them.
const fs::path StelFileMgr::getDesktopDir(void)
{
	// TODO: Test Windows and MAC builds.  I edited the code but have
	// not got a build platform -MNG
	fs::path result;
#if defined(WIN32)
	char path[MAX_PATH];
	path[MAX_PATH-1] = '\0';
	// Previous version used SHGetFolderPath and made app crash on window 95/98..
	//if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path)))
	LPITEMIDLIST tmp;
	if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &tmp)))
	{
		SHGetPathFromIDList(tmp, path);                      
		result = path;
	}
	else
	{	
		if(getenv("USERPROFILE")!=NULL)
		{
			//for Win XP etc.
			result = string(getenv("USERPROFILE")) + "\\Desktop";
		}
		else
		{
			//for Win 98 etc.
			//note: will not work well for users who installed windows in a 
			//non-default location.  Ugly & a source of problems.
			result = "C:\\Windows\\Desktop";
		}
	}
#else
	result = getenv("HOME");
	result /= "Desktop";
#endif
	if (!fs::is_directory(result))
	{
		throw(runtime_error("NOT FOUND"));
	}
	return result;
}

const fs::path StelFileMgr::getUserDir(void)
{
#if defined(MINGW32) || defined(WIN32)
	// Windows
#error "StelFileMgr::getUserDir not yet implemented for Windows"	
#elseif defined(MAXOSX)
	// OSX
#error "StelFileMgr::getUserDir not yet implemented for OSX"
#else 
	// Linux, BSD, Solaris etc.	       
	fs::path homeLocation(getenv("HOME"));
	if (!fs::is_directory(homeLocation))
	{
		// This will be the case if getenv failed or something else is weird
		cerr << "WARNING StelFileMgr::StelFileMgr: HOME env var refers to non-directory" << endl;
		throw(runtime_error("NOT FOUND"));
	}
	else 
	{
		homeLocation /= ".stellarium";
		return(homeLocation);
	}
#endif
	throw(runtime_error("NOT FOUND"));
}
	
const fs::path StelFileMgr::getInstallationDir(void)
{
	// If we are running from the build tree, we use the files from there...
	if (fs::exists(CHECK_FILE))
		return fs::path(".");

#if defined(MINGW32) || defined(WIN32)
	// Windows
#error "StelFileMgr::getInstallationDir not yet implemented for Windows"	
#elseif defined(MAXOSX)
	// OSX
#error "StelFileMgr::getInstallationDir not yet implemented for OSX"
#else
	// Linux, BSD, Solaris etc.
	// We use the value from the config.h filesystem
	fs::path installLocation(INSTALL_DATADIR);
	if (fs::exists(installLocation / CHECK_FILE))
	{
		return installLocation;
	}
	else
	{
		cerr << "WARNING StelFileMgr::StelFileMgr: could not find install location:"
			<< installLocation.string()
			<< " (we checked for "
			<< CHECK_FILE
			<< ")."
			<< endl;
		installLocation = fs::current_path();
		if (fs::exists(installLocation / CHECK_FILE))
		{
			return installLocation;
		}
		else
		{
			throw(runtime_error("NOT FOUND"));
		}
	}
#endif
	throw(runtime_error("NOT FOUND"));
}
	
const fs::path StelFileMgr::getScreenshotDir(void)
{
#if defined(MINGW32) || defined(WIN32)
	// Windows
#error "StelFileMgr::getScreenshotDir not yet implemented for Windows"	
	// return getDesktopDir();
#elseif defined(MAXOSX)
	// OSX
#error "StelFileMgr::getScreenshotDir not yet implemented for OSX"
	// return getDesktopDir();
#else 
	// Linux, BSD, Solaris etc.
	fs::path checkDir(getenv("HOME"));
	if (!fs::is_directory(checkDir))
	{
		cerr << "WARNING StelFileMgr::StelFileMgr: HOME env var refers to non-directory" << endl;
		throw(runtime_error("NOT FOUND"));
	}
	else
	{
		return(checkDir);
	}
#endif
	throw(runtime_error("NOT FOUND"));
}

const fs::path StelFileMgr::getScriptSaveDir(void)
{
#if defined(MINGW32) || defined(WIN32)
	// Windows
#error "StelFileMgr::getScreenshotDir not yet implemented for Windows"	
	// return getDesktopDir();
#elseif defined(MAXOSX)
	// OSX
#error "StelFileMgr::getScreenshotDir not yet implemented for OSX"
	// return getDesktopDir();
#else 
	// Linux, BSD, Solaris etc.
	fs::path checkDir(getenv("HOME"));
	if (!fs::is_directory(checkDir))
	{
		cerr << "WARNING StelFileMgr::StelFileMgr: HOME env var refers to non-directory" << endl;
		throw(runtime_error("NOT FOUND"));
	}
	else
	{
		return(checkDir);
	}
#endif
	cerr << "ERROR in StelFileMgr::getScriptSaveDir: could not determine installation directory" << endl;
	throw(runtime_error("NOT FOUND"));
}

const string StelFileMgr::getLocaleDir(void)
{
	fs::path localePath;
#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__) || defined(MINGW32) || defined(MACOSX)
	// Windows and MacOS X have the locale dir in the installation folder
	// TODO: check if this works with OSX
	localePath = getInstallationDir() / "data/locale";
#else
	// Linux, BSD etc, the locale dir is set in the config.h
	// but first, if we are in the development tree, don't rely on an 
	// install having been done.
	if (getInstallationDir() == ".")
		return "./data/locale";
	else
		localePath = fs::path(INSTALL_LOCALEDIR);
	
#endif
	if (fs::exists(localePath))
	{
		return localePath.string();
	}
	else
	{
		cerr << "WARNING in StelFileMgr::getLocaleDir() - could not determine locale directory, returning \"\"" << endl;
		return "";
	}
}

