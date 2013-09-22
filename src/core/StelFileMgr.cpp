/*
 * Stellarium
 * Copyright (C) 2010 Fabien Chereau
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <cstdlib>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QDebug>
#include <QStandardPaths>

#include "StelUtils.hpp"

#ifdef Q_OS_WIN
# include <windows.h>
# ifndef _SHOBJ_H
# include <shlobj.h>
# include <QLibrary>
# endif
#endif

#include "StelFileMgr.hpp"

// Initialize static members.
QStringList StelFileMgr::fileLocations;
QString StelFileMgr::userDir;
QString StelFileMgr::screenshotDir;
QString StelFileMgr::installDir;

void StelFileMgr::init()
{
	// Set the userDir member.
#ifdef Q_OS_WIN
	QString winApiPath = getWin32SpecialDirPath(CSIDL_APPDATA);
	if (!winApiPath.isEmpty())
	{
		userDir = winApiPath + "\\Stellarium";
	}
#elif defined(Q_OS_MAC)
	userDir = QDir::homePath() + "/Library/Application Support/Stellarium";
#else
	userDir = QDir::homePath() + "/.stellarium";
#endif

	if (!QFile(userDir).exists())
	{
		qWarning() << "User config directory does not exist: " << QDir::toNativeSeparators(userDir);
	}
	try
	{
		makeSureDirExistsAndIsWritable(userDir);
	}
	catch (std::runtime_error &e)
	{
		qFatal("Error: cannot create user config directory: %s", e.what());
	}


	// OK, now we have the userDir set, add it to the search path
	fileLocations.append(userDir);

	
	// Determine install data directory location

	// If we are running from the build tree, we use the files from the current directory
	if (QFileInfo(CHECK_FILE).exists())
	{
		installDir = ".";
	}
	else
	{
#ifdef Q_OS_MAC
		QString relativePath = "/../Resources";
		if (QCoreApplication::applicationDirPath().contains("src")) {
			relativePath = "/../../../../..";
		}
		QFileInfo MacOSdir(QCoreApplication::applicationDirPath() + relativePath);
		
		QDir ResourcesDir = MacOSdir.dir();
		if (!QCoreApplication::applicationDirPath().contains("src")) {
			ResourcesDir.cd(QString("Resources"));
		}
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
			installDir = installLocation.filePath();
		}
		else
		{
			qWarning() << "WARNING StelFileMgr::StelFileMgr: could not find install location:" << 
				QDir::toNativeSeparators(installLocation.filePath()) << " (we checked for " << 
				QDir::toNativeSeparators(checkFile.filePath()) << ").";
			qFatal("Couldn't find install directory location.");
		}
	}
	
	// Then add the installation directory to the search path
	fileLocations.append(installDir);

	if (!QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).isEmpty())
		screenshotDir = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0];
}


QString StelFileMgr::findFile(const QString& path, Flags flags)
{
	qDebug() << path;
	if (path.isEmpty())
		return "";
	
	// explicitly specified relative paths
	if (path[0] == '.')
	{
		if (fileFlagsCheck(path, flags))
			return path;
		qWarning() << QString("file does not match flags: %1").arg(path);
		return "";
	}

	// Qt resource files
	if (path.startsWith(":/"))
		return path;

	foreach (const QString& i, fileLocations)
	{
		if (fileFlagsCheck(i + "/" + path, flags))
			return i + "/" + path;
	}

	// explicitly specified absolute paths
	if ( isAbsolute(path) )
	{
		if (fileFlagsCheck(path, flags))
			return path;
		qWarning() << QString("file does not match flags: %1").arg(path);
		return "";
	}
	
	qWarning() << QString("file not found: %1").arg(path);
	return "";
}

QStringList StelFileMgr::findFileInAllPaths(const QString &path, const Flags &flags)
{
	if (path.isEmpty())
		throw std::runtime_error("Empty file path");

	QStringList filePaths;

	// explicitly specified relative paths
	if (path[0] == '.')
	{
		if (fileFlagsCheck(path, flags))
		{
			filePaths.append(path);
			return filePaths;
		}
		else
			throw std::runtime_error(QString("file does not match flags: %1").arg(path).toLocal8Bit().constData());
	}

	// Qt resource files
	if (path.startsWith(":/"))
	{
		filePaths.append(path);
		return filePaths;
	}

	// explicitly specified absolute paths
	if ( isAbsolute(path) )
	{
		if (fileFlagsCheck(path, flags))
		{
			filePaths.append(path);
			return filePaths;
		}
		else
			throw std::runtime_error(QString("file does not match flags: %1").arg(path).toLocal8Bit().constData());
	}

	foreach (QString locationPath, fileLocations)
	{
		if (fileFlagsCheck(locationPath + "/" + path, flags))
		{
			filePaths.append(locationPath + "/" + path);
		}
	}

	if (filePaths.isEmpty())
		throw std::runtime_error(QString("file not found: %1").arg(path).toLocal8Bit().constData());
	else
		return filePaths;
}

QSet<QString> StelFileMgr::listContents(const QString& path, const StelFileMgr::Flags& flags, bool recursive)
{
	QSet<QString> result;
	QStringList listPaths;

	if (recursive)
	{
		QSet<QString> dirs = listContents(path, Directory, false);
		result = listContents(path, flags, false); // root
		// add results for each sub-directory
		foreach (const QString& d, dirs)
		{
			QSet<QString> subDirResult = listContents(path + "/" + d, flags, true);
			foreach (const QString& r, subDirResult)
			{
				result.insert(d + "/" + r);
			}
		}
		return result;
	}

	// If path is "complete" (a full path), we just look in there, else
	// we append relative paths to the search paths maintained by this class.
	if (QFileInfo(path).isAbsolute())
		listPaths.append("");
	else
		listPaths = fileLocations;

	foreach (const QString& li, listPaths)
	{
		QFileInfo thisPath;
		if (QFileInfo(path).isAbsolute())
			thisPath.setFile(path);
		else
			thisPath.setFile(li+"/"+path);

		if (thisPath.isDir())
		{
			QDir thisDir(thisPath.absoluteFilePath());
			foreach (const QString& fileIt, thisDir.entryList())
			{
				if (fileIt != ".." && fileIt != ".")
				{
					QFileInfo fullPath;
					if (QFileInfo(path).isAbsolute())
						fullPath.setFile(path+"/"+fileIt);
					else
						fullPath.setFile(li+"/"+path+"/"+fileIt);

					// default is to return all objects in this directory
					bool returnThisOne = true;

					// but if we have flags set, that will filter the result
					if ((flags & Writable) && !fullPath.isWritable())
						returnThisOne = false;

					if ((flags & Directory) && !fullPath.isDir())
						returnThisOne = false;

					if ((flags & File) && !fullPath.isFile())
						returnThisOne = false;

					// OK, add the ones we want to the result
					if (returnThisOne)
					{
						result.insert(fileIt);
					}
				}
			}
		}
	}

	return result;
}

bool StelFileMgr::exists(const QString& path)
{
	return QFileInfo(path).exists();
}

bool StelFileMgr::isAbsolute(const QString& path)
{
	return QFileInfo(path).isAbsolute();
}

bool StelFileMgr::isReadable(const QString& path)
{
	return QFileInfo(path).isReadable();
}

bool StelFileMgr::isWritable(const QString& path)
{
	return QFileInfo(path).isWritable();
}

bool StelFileMgr::isDirectory(const QString& path)
{
	return QFileInfo(path).isDir();
}

qint64 StelFileMgr::size(const QString& path)
{
	return QFileInfo(path).size();
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

bool StelFileMgr::fileFlagsCheck(const QString& path, const Flags& flags)
{
	QFileInfo thePath(path);
	if (flags & New)
	{
		// if the file already exists, it is not a new file
		if (thePath.exists())
			return false;

		// To be able to create a new file, we need to have a
		// parent directory which is writable.
		QFileInfo pInfo(thePath.dir().absolutePath());
		if (!pInfo.exists() || !pInfo.isWritable())
		{
			return false;
		}
	}
	else if (thePath.exists())
	{
		if ((flags & Writable) && !thePath.isWritable())
			return false;

		if ((flags & Directory) && !thePath.isDir())
			return false;

		if ((flags & File) && !thePath.isFile())
			return false;
	}
	else
	{
		// doesn't exist and New flag wasn't requested
		return false ;
	}

	return true;
}

QString StelFileMgr::getDesktopDir()
{

	if (QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).isEmpty())
		return "";

	QString result = QStandardPaths::standardLocations(QStandardPaths::DesktopLocation)[0];
	if (!QFileInfo(result).isDir())
		return "";
	
	return result;
}

QString StelFileMgr::getUserDir()
{
	return userDir;
}

void StelFileMgr::setUserDir(const QString& newDir)
{
	makeSureDirExistsAndIsWritable(newDir);
	QFileInfo userDirFI(newDir);
	userDir = userDirFI.filePath();
	fileLocations.replace(0, userDir);
}

QString StelFileMgr::getInstallationDir()
{
	return installDir;
}

QString StelFileMgr::getScreenshotDir()
{
	return screenshotDir;
}

void StelFileMgr::setScreenshotDir(const QString& newDir)
{
	QFileInfo userDirFI(newDir);
	if (!userDirFI.exists() || !userDirFI.isDir())
	{
		qWarning() << "WARNING StelFileMgr::setScreenshotDir dir does not exist: " << QDir::toNativeSeparators(userDirFI.filePath());
		throw std::runtime_error("NOT_VALID");
	}
	else if (!userDirFI.isWritable())
	{
		qWarning() << "WARNING StelFileMgr::setScreenshotDir dir is not writable: " << QDir::toNativeSeparators(userDirFI.filePath());
		throw std::runtime_error("NOT_VALID");
	}
	screenshotDir = userDirFI.filePath();
}

QString StelFileMgr::getLocaleDir()
{
#ifdef ENABLE_NLS
	QFileInfo localePath = QFileInfo(getInstallationDir() + "/translations");
	if (localePath.exists())
	{
		return localePath.filePath();
	}
	else
	{
		// If not found, try to look in the standard build directory (useful for developer)
		localePath = QCoreApplication::applicationDirPath() + "/../translations";
		if (localePath.exists())
		{
			return localePath.filePath();
		}
		else
		{
			qWarning() << "WARNING StelFileMgr::getLocaleDir() - could not determine locale directory";
			return "";
		}
	}
#else
	return QString();
#endif
}

// Returns the path to the cache directory. Note that subdirectories may need to be created for specific caches.
QString StelFileMgr::getCacheDir()
{
	return (QStandardPaths::standardLocations(QStandardPaths::CacheLocation) << getUserDir() + "/cache")[0];
}


void StelFileMgr::makeSureDirExistsAndIsWritable(const QString& dirFullPath)
{
	// Check that the dirFullPath directory exists
	QFileInfo uDir(dirFullPath);
	if (!uDir.exists())
	{
		// The modules directory doesn't exist, lets create it.
		qDebug() << "Creating directory " << QDir::toNativeSeparators(uDir.filePath());
		if (!QDir("/").mkpath(uDir.filePath()))
		{
			throw std::runtime_error(QString("Could not create directory: " +uDir.filePath()).toStdString());
		}
		QFileInfo uDir2(dirFullPath);
		if (!uDir2.isWritable())
		{
			throw std::runtime_error(QString("Directory is not writable: " +uDir2.filePath()).toStdString());
		}
	}
	else if (!uDir.isWritable())
	{
		throw std::runtime_error(QString("Directory is not writable: " +uDir.filePath()).toStdString());
	}
}

#ifdef Q_OS_WIN
QString StelFileMgr::getWin32SpecialDirPath(int csidlId)
{
	// This function is implemented using code from QSettings implementation in QT
	// (GPL edition, version 4.3).
	
	// Stellarium works only on wide-character versions of Windows anyway,
	// therefore it's using only the wide-char version of the code. --BM
	QLibrary library(QLatin1String("shell32"));
	typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, LPTSTR, int, BOOL);
	GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)library.resolve("SHGetSpecialFolderPathW");
	if (SHGetSpecialFolderPath)
	{
		TCHAR tpath[MAX_PATH];
		SHGetSpecialFolderPath(0, tpath, csidlId, FALSE);
		return QString::fromUtf16((ushort*)tpath);
	}

	return QString();
}
#endif
