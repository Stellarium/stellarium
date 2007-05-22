#ifndef STELFILEMGR_HPP
#define STELFILEMGR_HPP 1

#define CHECK_FILE "data/ssystem.ini"

#include <vector>
#include <set>
#include <string>
#include <boost/filesystem/path.hpp>

using namespace std;
namespace fs = boost::filesystem;

//! @brief Provides utilities for locating and handling files.
//!
//! StelFileMgr provides functions for locating files.  It maintains a list of 
//! directories in which to look for files called the search path. Typcially this
//! includes the Stellarium installation directory, and a per-user settings 
//! directory (on platforms which support it).
//! The concept is that the StelFileMgr will be asked for a named path, and it
//! will try to locate that path within each of the search directories.
//! @author Lippo Huhtala <lippo.huhtala@meridea.com>
//! @author Matthew Gates <matthew@porpoisehead.net>
class StelFileMgr
{
public:
	//! @enum FLAGS used as named bitfield flags as specifiers to filter results of StelFileMgr methods.
	enum FLAGS {
		REMOVABLE_MEDIA	= 0x00000001,	//!< Search on removable media if present (default is not to).
		WRITABLE	= 0x00000002,	//!< Only return writable paths. For directories this means
						//!< that it is possible to create files within the directory.
		DIRECTORY	= 0x00000004,	//!< Exclude non-directories.
		FILE		= 0x00000008,	//!< Exclude non-files.
		NEW 		= 0x00000010,	//!< Exclude existing paths.
  		HIDDEN		= 0x00000020	//!< Include "hidden" paths (starting with a . on POSIX systems).
	};
				
	//! Default Constructor.
	//! By default, StelFileMgr will be created with the Stellarium installation directory
	//! config_root in the search path.  On systems which provide a per-user data/settings
	//! directory (which we call the user_settings directory, this is also included in 
	//! the search path, before the <config_root> directory.
	StelFileMgr();
	
	~StelFileMgr();

	//! Search for a path within the search paths, for example "textures/fog.png".
	//! findFile looks through the search paths in order, returning the first instance
	//! of the specified path.  By specifying a flags parameter it is possible to constrain
	//! the results to those matching various criteria.
	//! If the path argument is a complete path (is a full path on single root OSes, or
	//! unanbigiously identifies one and only one file on multi-root OSes), it will 
	//! be tested for compliance with other conditions - the regular search path will
	//! not be tested.
	//! If you wish to serach for a non-exiting file which is not in the search path 
	//! you should explicitly prefix it with "./", or otherwise have a . at the start of
	//! the path parameter, e.g. path="./my_config_file_in_the_pwd.ini"
	//! @param path the name of the file to search for, for example "textures/fog.png".
	//! @param flags options which constrain the result.
	//! @return returns a full path of the file if found, else return an empty path.
	//! @exception [misc] boost filesystem exceptions on file path errors and such
	//! @exception std::exception what() -> "file not found: [filename]"
	//! @exception std::exception what() -> "file does not match flags: [fullpath]".
	//! 		This exception occurs if a full path (complete in the 
	//! 		boost::filesystem sense) is passes at the path argument, but 
	//!		that path does not match the flags specified.
	const fs::path findFile(const string& path, const FLAGS& flags=(FLAGS)0);
	
	//! Set a set of all possible files/directories in any Stellarium search directory
	//! @param path the path to search inside, e.g. "landscapes"
	//! @param flags options which constrain the result
	//! @return returns a std::set of file and directory names (just the basename, 
	//!         not the whole path), which are available in any of the search
	//!         paths + path.  Returns empty list if none were found or the path
	//!         is invalid (not a directory / not existing)
	const set<string> listContents(const string& path, const FLAGS& flags=(FLAGS)0);
		
	//! Get a vector of gs::path objects which describes the current search paths.
	//! @return returns a vector of boost::filesystem::path objects representing the
	//!         current search paths.
	const vector<fs::path>& getSearchPaths(void) { return fileLocations; }
	
	//! Set the search paths.
	//! @param paths is a vector of boost::filesystem::path objects which will become the new
	//!        search paths
	void setSearchPaths(const vector<fs::path> paths);
		
	//! Check if a path exists.  Note it might be a file or a directory.
	//! @param path to check
	bool exists(const fs::path& path);
	
	//! Check if a path is writable
	//! For files, true is returned if the file exists and is writable
	//! or if the file doesn't exist, but it's parent directory does,
	//! if the file can be created.
	//! In the case of directories, return true if the directory can
	//! have files created in it.
	//! @param path to check
	bool isWritable(const fs::path& path);
	
	//! Check if a path exists and is a directory.
	//! @param path to check
	bool isDirectory(const fs::path& path);
	
	//! Check if the user directory exists, is writable and a driectory
	//! Creates it if it does not exist.  Exits the program if any of this
	//! process fails.
	void checkUserDir();
	
	//! Get the user's Desktop directory
	//! This is a portable way to retrieve the directory for the user's desktop.
	//! On Linux and OSX this is $HOME/Desktop.  For Windows, the system is queried
	//! using SHGetSpecialFolderLocation.  If that doesn't work, the USERPROFILE
	//! environment variable is checked, and if set, \\Desktop is appended, else
	//! C:\\Windows\\Desktop is used.
	//! @return the path to the user's desktop directory
	//! @exception NOT_FOUND when the directory cannot be determined, or the
	//!            OS doesn't provide one.
	const fs::path getDesktopDir(void);
	
	//! Returns the path to the user directory
	//! This is the directory where we expect to find the [default] writable 
	//! configuration file, user versions of scripts, nebulae, stars, skycultures etc.
	//! It will be the first directory in the path which is used when
	//! trying to find most data files
	//! @return the path to the user private data directory	
	//! @exceptions NOT_FOUND if the directory could not be found
	const fs::path getUserDir(void);
	
	//! Returns the path to the installation directory
	//! This is the directory where we expect to find scripts, nebulae, stars, 
	//! skycultures etc, and will be added at the end of the search path
	//! @return the path to the installation data directory	
	//! @exceptions NOT_FOUND if the directory could not be found
	const fs::path getInstallationDir(void);
	
	//! This is the directory into which screenshots will be saved
	//! It is $HOME on Linux, BSD, Solaris etc.
	//! It is the user's Desktop on MacOS X (??? - someone please verify this)
	//! It is ??? on Windows
	//! @return the path to the directory where screenshots are saved
	//! @exceptions NOT_FOUND if the directory could not be found
	const fs::path getScreenshotDir(void);
	
	//! This is the directory into which saved scripts will be saved
	//! It is $HOME on Linux, BSD, Solaris etc.
	//! It is the user's desktop on MacOS X (??? - someone please verify this)
	//! It is ??? on Windows
	//! @return the path to the directory where recorded scripts are saved	
	//! @exceptions NOT_FOUND if the directory could not be found
	const fs::path getScriptSaveDir(void);
	
	//! get the directory for locate files (i18n)
	//! @return the path to the locale directory or "" if the locale directory could not be found.
	const string getLocaleDir(void);
	
private:
	//! Check if a (complete) path matches a set of flags
	//! @param path a complete (in the boost::fs sense) path
	//! @param flags a set of StelFileMgr::FLAGS to test against path
	//! @return true if path passes all flag tests, else false
	//! @exceptions [misc] can throw boost::filesystem exceptions if there are unexpected problems with IO
	bool fileFlagsCheck(const fs::path& path, const FLAGS& flags=(FLAGS)0);
	
	//! Used to print info to stdout on the current state of the file paths.
	void outputFileSearchPaths(void);
		
	vector<fs::path> fileLocations;
	
};

#endif
