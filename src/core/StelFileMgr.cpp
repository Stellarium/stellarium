#include <config.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QDebug>

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
	// Set the userDir member.
	QFileInfo userDirFI;
#if defined(WIN32)
	QString homeString = QDir::homePath();
	if (homeString == QDir::rootPath() || homeString.toUpper() == "C:\\")
	{
		// This case happens in Win98 with no user profiles.  In this case
		// We don't want to bother with a separate user dir - we just 
		// return the install directory.
		userDirFI = getInstallationDir();
	}
	else
	{
		userDirFI = homeString + "/Stellarium";
	}

	// In 0.9.0 we forgot to check the %APPDATA% env var, which might
	// be set in XP or newer.  In this case, we want to use it, but
	// only if there is not already an existing data directory in the
	// "wrong place" (better to use an existing location which is slightly
	// wrong, than to lose all the users settings when thy upgrade).
	if (getenv("APPDATA")!=NULL && !userDirFI.isDir())
	{
		userDirFI = QFile::decodeName(getenv("APPDATA")) + "/Stellarium";
	}

#elif defined(MACOSX)
	userDirFI.setFile(QDir::homePath() + "/Library/Preferences/Stellarium");
#else 
	userDirFI.setFile(QDir::homePath() + "/.stellarium");
#endif
	if (!userDirFI.exists() || !userDirFI.isDir())
	{
		qWarning() << "WARNING StelFileMgr::StelFileMgr user dir does not exist: "
			<< userDirFI.filePath();
	}
	else if (!userDirFI.isWritable())
	{
		qWarning() << "WARNING StelFileMgr::StelFileMgr user dir is not writable: "
			<< userDirFI.filePath();
	}
	userDir = userDirFI.filePath();

	// OK, now we have the userDir set, we will add it and the installation 
	// dir to the search path.  The user directory is first.
	fileLocations.append(userDir);
	
	try
	{
		fileLocations.append(getInstallationDir());
	}
	catch(exception &e)
	{
		cerr << "WARNING: could not locate installation directory" << endl;
	}
}

StelFileMgr::~StelFileMgr()
{
}

QString StelFileMgr::findFile(const QString& path, const FLAGS& flags)
{
	if (path.isEmpty())
		throw (runtime_error(std::string("Empty file path")));
	// explicitly specified relative paths
	if (path[0] == '.')
		if (fileFlagsCheck(path, flags)) 
			return path;
		else
			throw (runtime_error(std::string("file does not match flags: ") + qPrintable(path)));

	// explicitly specified absolute paths
	QFileInfo thePath(path);
	if ( thePath.isAbsolute() )
		if (fileFlagsCheck(path, flags))
			return path;
		else
			throw (runtime_error(std::string("file does not match flags: ") + qPrintable(path)));
	
	foreach (QString i, fileLocations)
	{
		if (fileFlagsCheck(i + "/" + path, flags))
			return i + "/" + path;
	}
	
	throw (runtime_error(std::string("file not found: ") + qPrintable(path)));
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
	
	foreach (QString li, listPaths)
	{
		QFileInfo thisPath(li+"/"+path);
		if (thisPath.isDir()) 
		{
  			QDir thisDir(thisPath.absoluteFilePath());
			QStringList lsOut = thisDir.entryList();
			for (QStringList::const_iterator fileIt = lsOut.constBegin(); fileIt != lsOut.constEnd(); ++fileIt)
			{
				if ((*fileIt != "..") && (*fileIt != "."))
				{
					QFileInfo fullPath(li+"/"+path+"/"+*fileIt);
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

bool StelFileMgr::exists(const QString& path)
{
	return QFileInfo(path).exists();
}

bool StelFileMgr::isWritable(const QString& path)
{
	return QFileInfo(path).isWritable();
}

bool StelFileMgr::isDirectory(const QString& path)
{
	return QFileInfo(path).isDir();
}

bool StelFileMgr::mkDir(const QString& path)
{
	return QDir("/").mkpath(path);
}

QString StelFileMgr::dirName(const QString& path)
{
	return QFileInfo(path).dir().canonicalPath();
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
				cerr << "ERROR StelFileMgr::checkUserDir: user directory is not a writable directory: " << qPrintable(uDir.filePath()) << endl;
				qFatal("User directory is not a writable directory");
			}
		}
		else
		{
			// The user directory doesn't exist, lets create it.
			if (!QDir("/").mkpath(uDir.filePath()))
			{
				cerr << "ERROR: could not create user directory: " << qPrintable(uDir.filePath()) << endl;
				qFatal("Could not create user directory");
			}
		}
	}
	catch(exception& e)
	{
		// This should never happen  ;)
		cerr << "ERROR: cannot work out the user directory: " << e.what() << endl;
		qFatal("Cannot work out the user directory");
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
	foreach (QString i, fileLocations)
	{
		qDebug() << " " << ++count << ") " << i;
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
	result = QFile::decodeName(getenv("HOME"));
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
	return userDir;
}

void StelFileMgr::setUserDir(const QString& newDir)
{
	QFileInfo userDirFI(newDir);
	if (!userDirFI.exists() || !userDirFI.isDir())
	{
		qWarning() << "WARNING StelFileMgr::setUserDir user dir does not exist: "
			<< userDirFI.filePath();
		throw (runtime_error(std::string("NOT_VALID")));
	}
	else if (!userDirFI.isWritable())
	{
		qWarning() << "WARNING StelFileMgr::setUserDir user dir is not writable: "
			<< userDirFI.filePath();
		throw (runtime_error(std::string("NOT_VALID")));
	}
	userDir = userDirFI.filePath();
	fileLocations.replace(0, userDir);
}

QString StelFileMgr::getInstallationDir(void)
{
	// If we are running from the build tree, we use the files from there...
	if (QFileInfo(CHECK_FILE).exists())
		return ".";

#if defined(MACOSX)
	QFileInfo MacOSdir(QCoreApplication::applicationDirPath());
	QDir ResourcesDir = MacOSdir.dir();
	ResourcesDir.cd(QString("Resources"));
	QFileInfo installLocation(ResourcesDir.absolutePath());
	QFileInfo checkFile(installLocation.filePath() + QString("/") + QString(CHECK_FILE));
#else
	// Linux, BSD, Solaris etc.
	// We use the value from the config.h filesystem
	QFileInfo installLocation(QFile::decodeName(INSTALL_DATADIR));
	QFileInfo checkFile(QFile::decodeName(INSTALL_DATADIR "/" CHECK_FILE));
#endif

	if (checkFile.exists())
	{
		return installLocation.filePath();
	}
	else
	{
		qWarning() << "WARNING StelFileMgr::StelFileMgr: could not find install location:"
			<< installLocation.filePath() << " (we checked for " << checkFile.filePath() << ").";
		throw (runtime_error("NOT FOUND"));
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

QString StelFileMgr::getLocaleDir(void)
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
		localePath = QFileInfo(QFile::decodeName(INSTALL_LOCALEDIR));
	
#endif
	if (localePath.exists())
	{
		return localePath.filePath();
	}
	else
	{
		cerr << "WARNING in StelFileMgr::getLocaleDir() - could not determine locale directory, returning \"\"" << endl;
		return "";
	}
}

