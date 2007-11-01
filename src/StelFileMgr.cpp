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
		fileLocations.append(getUserDir());
	}
	catch(exception &e)
	{
		cerr << "WARNING: could not locate user directory" << endl;
	}
	
	try
	{
		fileLocations.append(getInstallationDir());
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

string StelFileMgr::findFile(const string& path, const FLAGS& flags)
{
	return qfindFile(QString::fromUtf8(path.c_str()), flags).toUtf8().data();
}

QString StelFileMgr::qfindFile(const QString& path, const FLAGS& flags)
{
	// explicitly specified relative paths
	if (path[0] == '.')
		if (fileFlagsCheck(path, flags)) 
			return path;
		else
			throw(runtime_error(std::string("file does not match flags: ") + path.toUtf8().data()));

	// explicitly specified absolute paths
	QFileInfo thePath(path);
	if ( thePath.isAbsolute() )
		if (fileFlagsCheck(path, flags))
			return path;
		else
			throw(runtime_error(std::string("file does not match flags: ") + path.toUtf8().data()));
	
	for (QStringList::iterator i=fileLocations.begin(); i!=fileLocations.end(); ++i)
	{
		if (fileFlagsCheck(*i + "/" + path, flags))
			return (*i + "/" + path);
	}
	
	throw(runtime_error(std::string("file not found: ") + path.toUtf8().data()));
}

QSet<QString> StelFileMgr::listContents(const QString& path, const StelFileMgr::FLAGS& flags)
{
	QSet<QString> result;
	QStringList listPaths;
			
	// If path is "complete" (a full path), we just look in there, else
	// we append relative paths to the search paths maintained by this class.
	if (QFileInfo(path).isAbsolute())
		listPaths.append("");
	else
		listPaths = fileLocations;
	
	for (QStringList::iterator li = listPaths.begin();li != listPaths.end();++li)
	{
		QFileInfo thisPath(*li+"/"+path);
		if (thisPath.isDir()) 
		{
  			QDir thisDir(thisPath.absoluteFilePath());
			QStringList lsOut = thisDir.entryList();
			for (QStringList::const_iterator fileIt = lsOut.constBegin(); fileIt != lsOut.constEnd(); ++fileIt)
			{
				if ((*fileIt != "..") && (*fileIt != "."))
				{
					QFileInfo fullPath(*li+"/"+path+"/"+*fileIt);
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
						result.insert(*fileIt);
					}
				}
			}
		}
	}

	return result;
}

void StelFileMgr::setSearchPaths(const QStringList& paths)
{
	fileLocations = paths;
	//outputFileSearchPaths();
}

bool StelFileMgr::exists(const string& path)
{
	return QFileInfo(QString::fromUtf8(path.c_str())).exists();
}
bool StelFileMgr::exists(const QString& path)
{
	return QFileInfo(path).exists();
}

bool StelFileMgr::isWritable(const string& path)
{
	return QFileInfo(QString::fromUtf8(path.c_str())).isWritable();
}
bool StelFileMgr::isWritable(const QString& path)
{
	return QFileInfo(path).isWritable();
}

bool StelFileMgr::isDirectory(const QString& path)
{
	return QFileInfo(path).isDir();
}

bool StelFileMgr::mkDir(const string& path)
{
	return QDir("/").mkpath(QString::fromUtf8(path.c_str()));
}

string StelFileMgr::dirName(const string& path)
{
	return QFileInfo(QString::fromUtf8(path.c_str())).dir().canonicalPath().toStdString();
}

QString StelFileMgr::baseName(const QString& path)
{
	return QFileInfo(path).baseName();
}

void StelFileMgr::checkUserDir()
{
	try {
		QFileInfo uDir(getUserDir());
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
				cerr << "ERROR: could not create user directory: " << uDir.filePath().toUtf8().data() << endl;
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

bool StelFileMgr::fileFlagsCheck(const QString& path, const FLAGS& flags)
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
	
	QFileInfo thePath(path);
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
	for(QStringList::iterator i=fileLocations.begin(); i!=fileLocations.end(); ++i)
	{
		cout << " " << ++count << ") " << qPrintable(*i) << endl;
	}
}

QString StelFileMgr::getDesktopDir(void)
{
	// TODO: Test Windows and MAC builds.  I edited the code but have
	// not got a build platform -MNG
	QString result;
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
			result = QString(getenv("USERPROFILE")) + "\\Desktop";
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
	if (!QFileInfo(result).isDir())
	{
		throw(runtime_error("NOT FOUND"));
	}
	return result;
}

QString StelFileMgr::getUserDir(void)
{
	QFileInfo userDir;
#if defined(WIN32)
	QString homeString = QDir::homePath();
	if (homeString == QDir::rootPath() || homeString.toUpper() == "C:\\")
	{
		// This case happens in Win98 with no user profiles.  In this case
		// We don't want to bother with a separate user dir - we just 
		// return the install directory.
		userDir = getInstallationDir();
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
	userDir.setFile(QDir::homePath() + "/Library/Preferences/Stellarium");
#else 
	userDir.setFile(QDir::homePath() + "/.stellarium");
#endif
	cerr << userDir.filePath().toStdString() << endl;
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
	return userDir.filePath();
}

QString StelFileMgr::getInstallationDir(void)
{
	// If we are running from the build tree, we use the files from there...
	if (QFileInfo(CHECK_FILE).exists())
		return ".";

#if defined(MACOSX)
	QFileInfo installLocation(MacosxDirs::getApplicationResourcesDirectory());
	QFileInfo checkFile(installLocation.filePath() + QString("/") + QString(CHECK_FILE));
#else
	// Linux, BSD, Solaris etc.
	// We use the value from the config.h filesystem
	QFileInfo installLocation(INSTALL_DATADIR);
	QFileInfo checkFile(INSTALL_DATADIR "/" CHECK_FILE);
#endif

	if (checkFile.exists())
	{
		return installLocation.filePath();
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
	
QString StelFileMgr::getScreenshotDir(void)
{
#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__) || defined(MINGW32) || defined(MACOSX)
	return getDesktopDir();
#else
	return QDir::homePath();
#endif
}

string StelFileMgr::getLocaleDir(void)
{
	QFileInfo localePath;
#if defined(WIN32) || defined(CYGWIN) || defined(__MINGW32__) || defined(MINGW32) || defined(MACOSX)
	// Windows and MacOS X have the locale dir in the installation folder
	// TODO: check if this works with OSX
	localePath = QFileInfo(getInstallationDir() + "/locale");
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

