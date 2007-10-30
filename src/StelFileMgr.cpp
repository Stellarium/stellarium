#include <config.h>
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <QFileInfo>
#include <QDir>
#include <QString>

#ifdef WIN32
# include <windows.h>
# ifndef _SHOBJ_H
# include <shlobj.h>
# endif 
#endif

#ifdef MACOSX
#include "MacosxDirs.hpp"
#endif

#include "StelFileMgr.hpp"

using namespace std;

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

	//outputFileSearchPaths();
}

StelFileMgr::~StelFileMgr()
{
}

const string StelFileMgr::findFile(const string& path, const FLAGS& flags)
{
	// explicitly specified relative paths
	if (path[0] == '.')
		if (fileFlagsCheck(path, flags)) 
			return path;
		else
			throw(runtime_error("file does not match flags: " + path));

	// explicitly specified absolute paths
		QFileInfo thePath(QString::fromLocal8Bit (path.c_str()));
	if ( thePath.isAbsolute() )
		if (fileFlagsCheck(path, flags))
			return path;
		else
			throw(runtime_error("file does not match flags: " + path));
	
	for(vector<string>::iterator i = fileLocations.begin();
		   i != fileLocations.end();
		   i++)
	{
		if (fileFlagsCheck(*i + "/" + path, flags))
			return (*i + "/" + path);
	}
	
	throw(runtime_error("file not found: " + path));
}

const set<string> StelFileMgr::listContents(const string& path, const StelFileMgr::FLAGS& flags)
{
	set<string> result;
	vector<string> listPaths;
		
	// If path is "complete" (a full path), we just look in there, else
	// we append relative paths to the search paths maintained by this class.
	if (QFileInfo(QString::fromLocal8Bit (path.c_str())).isAbsolute())
		listPaths.push_back("");
	else
		listPaths = fileLocations;
	
	for (vector<string>::iterator li = listPaths.begin();
	     li != listPaths.end();
	     li++)
	{
		QFileInfo thisPath(QString::fromLocal8Bit ((*li+"/"+path).c_str()));
		if (thisPath.isDir()) 
		{
  QDir thisDir(thisPath.absoluteFilePath());
			QStringList lsOut = thisDir.entryList();
			for (QStringList::const_iterator fileIt = lsOut.constBegin(); 
				fileIt != lsOut.constEnd(); 
				++fileIt)
			{

				if ((*fileIt != "..") && (*fileIt != "."))
				{
					QFileInfo fullPath(QString::fromLocal8Bit ((*li+"/"+path+"/"+(*fileIt).toStdString()).c_str()));
					// default is to return all objects in this directory
					bool returnThisOne = true;
				
					// but if we have flags set, that will filter the result
					if ((flags & WRITABLE) && !fullPath.isWritable())
						returnThisOne = false;
				
					if ((flags & DIRECTORY) && !fullPath.isDir())
						returnThisOne = false;
					
					if ((flags & FILE) && !fullPath.isFile())
						returnThisOne = false;
					
					// we only want to return "hidden" results if the HIDDEN flag is set
					if (!(flags & HIDDEN))
						if ((*fileIt)[0] == '.') 
							returnThisOne = false;
				
					// OK, add the ones we want to the result
					if (returnThisOne)
					{
						result.insert((*fileIt).toStdString());
					}
				}
			}
		}
	}

	return result;
}

void StelFileMgr::setSearchPaths(const vector<string>& paths)
{
	fileLocations = paths;
	//outputFileSearchPaths();
}

bool StelFileMgr::exists(const string& path)
{
	return QFileInfo(QString::fromLocal8Bit (path.c_str())).exists();
}

bool StelFileMgr::isWritable(const string& path)
{
	return QFileInfo(QString::fromLocal8Bit (path.c_str())).isWritable();
}

bool StelFileMgr::isDirectory(const string& path)
{
	return QFileInfo(QString::fromLocal8Bit (path.c_str())).isDir();
}

bool StelFileMgr::mkDir(const string& path)
{
	return QDir("/").mkpath(QString::fromLocal8Bit (path.c_str()));
}

const string StelFileMgr::dirName(const string& path)
{
	return QFileInfo(QString::fromLocal8Bit (path.c_str())).dir().canonicalPath().toStdString();
}

const string StelFileMgr::baseName(const string& path)
{
	return QFileInfo(QString::fromLocal8Bit (path.c_str())).baseName().toStdString();
}

void StelFileMgr::checkUserDir()
{
	try {
		QFileInfo uDir(QString::fromLocal8Bit (getUserDir().c_str()));
		if (uDir.exists())
		{
			if (uDir.isDir() && uDir.isWritable())
			{
				// everything checks out fine.
				return;
			}
			else
			{
				cerr << "ERROR StelFileMgr::checkUserDir: user directory is not a writable directory: " << uDir.filePath().toStdString() << endl;
				exit(1);
			}
		}
		else
		{
			// The user directory doesn't exist, lets create it.
			if (!QDir("/").mkpath(uDir.filePath()))
			{
				cerr << "ERROR: could not create user directory: " << uDir.filePath().toStdString() << endl;
				exit(1);
			}
		}
	}
	catch(exception& e)
	{
		// This should never happen  ;)
		cerr << "ERROR: cannot work out the user directory: " << e.what() << endl;
		exit(1);
	}	
}

bool StelFileMgr::fileFlagsCheck(const string& path, const FLAGS& flags)
{
	if ( ! (flags & HIDDEN) )
	{
		// Files are considered HIDDEN on POSIX systems if the file name begins with 
		// a "." character.  Unless we have the HIDDEN flag set, reject and path
		// where the basename starts with a .
		if (baseName(path)[0] == '.')
		{
			return(false);
		}
	}
	
	QFileInfo thePath(QString::fromLocal8Bit (path.c_str()));
	QDir parentDir = thePath.dir();

	if (flags & NEW)
	{
		// if the file already exists, it is not a new file
		if (thePath.exists())
			return false;				

		// To be able to create a new file, we need to have a 
		// parent directory which is writable.
		QFileInfo pInfo(parentDir.absolutePath());
		if (!pInfo.exists() || !pInfo.isWritable())
		{
			return(false);
		}
	}
	else if (thePath.exists())
	{
		if ((flags & WRITABLE) && !thePath.isWritable())
			return(false);
			
		if ((flags & DIRECTORY) && !thePath.isDir())
			return(false);
			
		if ((flags & FILE) && !thePath.isFile())
			return(false); 
	}
	else
	{
		// doesn't exist and NEW flag wasn't requested
		return(false);
	}
		
	return(true);
}

void StelFileMgr::outputFileSearchPaths(void)
{
	int count = 0;
	cout << "File search path set to:" << endl;		   
	for(vector<string>::iterator i = fileLocations.begin();
		   i != fileLocations.end();
		   i++)
	{
		cout << " " << ++count << ") " << *i << endl;
	}
}

const string StelFileMgr::getDesktopDir(void)
{
	// TODO: Test Windows and MAC builds.  I edited the code but have
	// not got a build platform -MNG
	string result;
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
	result += "/Desktop";
#endif
	if (!QFileInfo(QString::fromLocal8Bit (result.c_str())).isDir())
	{
		throw(runtime_error("NOT FOUND"));
	}
	return result;
}

const string StelFileMgr::getUserDir(void)
{
	QFileInfo userDir;
#if defined(WIN32)
	QString homeString = QDir::homePath();
	if (homeString == QDir::rootPath() || homeString.toUpper() == "C:\\")
	{
		// This case happens in Win98 with no user profiles.  In this case
		// We don't want to bother with a separate user dir - we just 
		// return the install directory.
		userDir = QString::fromStdString(getInstallationDir());
	}
	else
	{
		userDir = homeString + "/Stellarium";
	}

	// In 0.9.0 we forgot to check the %APPDATA% env var, which might
	// be set in XP or newer.  In this case, we want to use it, but
	// only if there is not already an existing data directory in the
	// "wrong place" (better to use an existing location which is slightly
	// wrong, than to lose all the users settings when thy upgrade).
	if (getenv("APPDATA")!=NULL && !userDir.isDir())
	{
		userDir = QString::fromLocal8Bit(getenv("APPDATA")) + "/Stellarium";
	}

#elif defined(MACOSX)
	userDir = QDir::homePath() + "/Library/Preferences/Stellarium";
#else 
	userDir = QDir::homePath() + "/.stellarium";
#endif
	if (!userDir.exists() || !userDir.isDir())
	{
		cerr << "WARNING StelFileMgr::getUserDir user dir does not exist: "
			<< userDir.filePath().toStdString() << endl;
	}
	else if (!userDir.isWritable())
	{
		cerr << "WARNING StelFileMgr::getUserDir user dir is not writable: "
			<< userDir.filePath().toStdString() << endl;
	}

	return userDir.filePath().toUtf8().data();
}

const string StelFileMgr::getInstallationDir(void)
{
	// If we are running from the build tree, we use the files from there...
	if (QFileInfo(CHECK_FILE).exists())
		return ".";

#if defined(MACOSX)
	QFileInfo installLocation( QString::fromLocal8Bit (MacosxDirs::getApplicationResourcesDirectory().c_str() ));
	QFileInfo checkFile(installLocation.filePath() + QString("/") + QString(CHECK_FILE));
#else
	// Linux, BSD, Solaris etc.
	// We use the value from the config.h filesystem
	QFileInfo installLocation(INSTALL_DATADIR);
	QFileInfo checkFile(INSTALL_DATADIR "/" CHECK_FILE);
#endif

	if (checkFile.exists())
	{
		return installLocation.filePath().toStdString();
	}
	else
	{
		cerr << "WARNING StelFileMgr::StelFileMgr: could not find install location:"
			<< installLocation.filePath().toStdString()
			<< " (we checked for "
			<< checkFile.filePath().toStdString()
			<< ")."
			<< endl;
		throw(runtime_error("NOT FOUND"));
	}
}
	
const string StelFileMgr::getScreenshotDir(void)
{
#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__) || defined(MINGW32) || defined(MACOSX)
	return getDesktopDir();
#else
	return QDir::homePath().toStdString();
#endif
}

const string StelFileMgr::getLocaleDir(void)
{
	QFileInfo localePath;
#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__) || defined(MINGW32) || defined(MACOSX)
	// Windows and MacOS X have the locale dir in the installation folder
	// TODO: check if this works with OSX
	localePath = QFileInfo(QString::fromLocal8Bit (getInstallationDir().c_str()) + "/locale");
#else
	// Linux, BSD etc, the locale dir is set in the config.h
	// but first, if we are in the development tree, don't rely on an 
	// install having been done.
	if (getInstallationDir() == ".")
		localePath = QFileInfo("./locale");
	else
		localePath = QFileInfo(INSTALL_LOCALEDIR);
	
#endif
	if (localePath.exists())
	{
		return localePath.filePath().toStdString();
	}
	else
	{
		cerr << "WARNING in StelFileMgr::getLocaleDir() - could not determine locale directory, returning \"\"" << endl;
		return "";
	}
}

