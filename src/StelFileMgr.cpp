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
#include <boost/random/linear_congruential.hpp>
	
using namespace std;
namespace fs = boost::filesystem;

StelFileMgr::StelFileMgr()
{
	// if the operating system supports user-level directories, the first place to look
	// for files is in there.
#if !defined(MACOSX) && !defined(MINGW32)
	// Linux & other POSIX-likes
	fs::path homeLocation(getenv("HOME"));
	if (!fs::is_directory(homeLocation))
	{
		// This will be the case if getenv failed or something else is weird
		cerr << "WARNING StelFileMgr::StelFileMgr: HOME env var refers to non-directory" << endl;
	}
	else 
	{
		homeLocation /= ".stellarium";
		fileLocations.push_back(homeLocation);
	}
#endif

	// TODO: the proper thing for Windows and OSX
	
	// OK, the main installation directory.  
#if !defined(MACOSX) && !defined(MINGW32)
	// For POSIX systems we use the value from the config.h filesystem
	fs::path installLocation(CONFIG_DATA_DIR);
	if (fs::exists(installLocation / CHECK_FILE))
	{
		fileLocations.push_back(installLocation);
		//configRootDir = installLocation;
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
			// cerr << "DEBUG StelFileMgr::StelFileMgr: found data files in current directory" << endl;
			fileLocations.push_back(installLocation);
			// configRootDir = installLocation;
		}		
		else 
		{
			cerr << "ERROR StelFileMgr::StelFileMgr: couldn't locate check file (" << CHECK_FILE << ") anywhere" << endl;	
		}
	}
#endif	
}

StelFileMgr::~StelFileMgr()
{
}

const fs::path StelFileMgr::findFile(const string& path, const FLAGS& flags)
{
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
}

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
	return result;
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

bool StelFileMgr::fileFlagsCheck(const fs::path& path, const FLAGS& flags)
{
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

