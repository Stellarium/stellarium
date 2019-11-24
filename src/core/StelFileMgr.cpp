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

#include "StelUtils.hpp"

#include <cstdlib>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QDebug>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QtGlobal>

#include <cstdio>

#ifdef Q_OS_WIN
#include <Windows.h>
#ifndef _SHOBJ_H
	#include <ShlObj.h>
	#include <QLibrary>
#endif
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

#if QT_VERSION >= 0x050A00
	if (qEnvironmentVariableIsSet("STEL_USERDIR"))
	{
		userDir=qEnvironmentVariable("STEL_USERDIR");
	}
#else
	QByteArray userDirCand=qgetenv("STEL_USERDIR");
	if (userDirCand.length()>0)
	{
		userDir=QString::fromLocal8Bit(userDirCand);
	}
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
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	QString envRoot = env.value("STELLARIUM_DATA_ROOT", ".");

	if (QFileInfo(envRoot + QDir::separator() + QString(CHECK_FILE)).exists())
	{
		installDir = envRoot;
	}	
	else
	{
	#if defined(Q_OS_MAC)
		QString relativePath = "/../Resources";
		if (QCoreApplication::applicationDirPath().contains("src")) {
			relativePath = "/../..";
		}
		QFileInfo MacOSdir(QCoreApplication::applicationDirPath() + relativePath);
		// These two lines are used to see if the Qt bug still exists.
		// The output from C: should simply be the parent of what is show for B:
		// qDebug() << "B: " << MacOSdir.absolutePath();
		// qDebug() << "C: " << MacOSdir.dir().absolutePath();

		QDir ResourcesDir(MacOSdir.absolutePath());
		if (!QCoreApplication::applicationDirPath().contains("src")) {
			ResourcesDir.cd(QString("Resources"));
		}
		QFileInfo installLocation(ResourcesDir.absolutePath());
		QFileInfo checkFile(installLocation.filePath() + QDir::separator() + QString(CHECK_FILE));
	#elif defined(Q_OS_WIN)		
		QFileInfo installLocation(QCoreApplication::applicationDirPath());
		QFileInfo checkFile(installLocation.filePath() + QDir::separator() + QString(CHECK_FILE));
	#else
		// Linux, BSD, Solaris etc.
		// We use the value from the config.h filesystem
		QFileInfo installLocation(QFile::decodeName(INSTALL_DATADIR));
		QFileInfo checkFile(QFile::decodeName(INSTALL_DATADIR "/" CHECK_FILE));
	#endif

	#ifdef DEBUG
		if (!checkFile.exists())
		{	// for DEBUG use sources location 
			QString debugDataPath = INSTALL_DATADIR_FOR_DEBUG;
			checkFile = QFileInfo(debugDataPath + QDir::separator() + CHECK_FILE);
			installLocation = QFileInfo(debugDataPath);
		}
	#endif

		if (checkFile.exists())
		{
			installDir = installLocation.filePath();
		}
		else
		{
			qWarning() << "WARNING StelFileMgr::StelFileMgr: could not find install location:"
					 << QDir::toNativeSeparators(installLocation.filePath())
					 << " (we checked for "
					 << QDir::toNativeSeparators(checkFile.filePath()) << ").";

			qWarning() << "Maybe this is AppImage or something similar? Let's check relative path...";
			// This hook has been added after reverse-engineering an AppImage application
			QString relativePath =  QCoreApplication::applicationDirPath() + QString("/../share/stellarium");
			checkFile = QFileInfo(relativePath + QDir::separator() + CHECK_FILE);
			if (checkFile.exists())
			{
				installDir = relativePath;
			}
			else
			{
				qWarning() << "WARNING StelFileMgr::StelFileMgr: could not find install location:"
						 << QDir::toNativeSeparators(relativePath)
						 << " (we checked for "
						 << QDir::toNativeSeparators(checkFile.filePath()) << ").";

				qWarning() << "Maybe this is development environment? Let's check source directory path...";

				QString sourceDirPath = STELLARIUM_SOURCE_DIR; // The variable is defined in CMakeLists.txt file
				checkFile = QFileInfo(sourceDirPath + QDir::separator() + CHECK_FILE);
				if (checkFile.exists())
				{
					installDir = sourceDirPath;
				}
				else
				{
					qWarning() << "WARNING StelFileMgr::StelFileMgr: could not find install location:"
							 << QDir::toNativeSeparators(sourceDirPath)
							 << " (we checked for "
							 << QDir::toNativeSeparators(checkFile.filePath()) << ").";

					#ifndef UNIT_TEST
					// NOTE: Hook for buildbots (using within testEphemeris)
					qFatal("Couldn't find install directory location.");
					#endif
				}
			}
		}
	}

	// Then add the installation directory to the search path
	fileLocations.append(installDir);
}


QString StelFileMgr::findFile(const QString& path, Flags flags)
{
	if (path.isEmpty())
	{
		qWarning() << "Empty file path";
		return "";
	}

	
	// Qt resource files
	if (path.startsWith(":/"))
		return path;

	// explicitly specified relative paths
	if (path[0] == '.')
	{
		if (fileFlagsCheck(path, flags))
			return path;
		else
		{
			qWarning() << QString("file does not match flags: %1").arg(path);
			return "";
		}
	}

	// explicitly specified absolute paths
	if (isAbsolute(path))
	{
		if (fileFlagsCheck(path, flags))
			return path;
		else
		{
			qWarning() << QString("file does not match flags: %1").arg(path);
			return "";
		}
	}
	
	for (const auto& i : fileLocations)
	{
		const QFileInfo finfo(i + "/" + path);
		if (fileFlagsCheck(finfo, flags))
			return i + "/" + path;
	}

	//FIXME: This line give false positive values for static plugins (trying search dynamic plugin first)
	//qWarning() << QString("file not found: %1").arg(path);
	return "";
}

QStringList StelFileMgr::findFileInAllPaths(const QString &path, const Flags &flags)
{
	QStringList filePaths;
	
	if (path.isEmpty())
		return filePaths;

	// Qt resource files
	if (path.startsWith(":/"))
	{
		filePaths.append(path);
		return filePaths;
	}

	// explicitly specified relative paths
	if (path[0] == '.')
	{
		if (fileFlagsCheck(path, flags))
			filePaths.append(path);
		return filePaths;
	}

	// explicitly specified absolute paths
	if ( isAbsolute(path) )
	{
		if (fileFlagsCheck(path, flags))
			filePaths.append(path);
		return filePaths;
	}

	for (const auto& locationPath : fileLocations)
	{
		const QFileInfo finfo(locationPath + "/" + path);
		if (fileFlagsCheck(finfo, flags))
			filePaths.append(locationPath + "/" + path);
	}

	return filePaths;
}

QSet<QString> StelFileMgr::listContents(const QString& path, const StelFileMgr::Flags& flags, bool recursive)
{
	QSet<QString> result;

	if (recursive)
	{
		QSet<QString> dirs = listContents(path, Directory, false);
		result = listContents(path, flags, false); // root
		// add results for each sub-directory
		for (const auto& d : dirs)
		{
			QSet<QString> subDirResult = listContents(path + "/" + d, flags, true);
			for (const auto& r : subDirResult)
			{
				result.insert(d + "/" + r);
			}
		}
		return result;
	}

	// If path is "complete" (a full path), we just look in there, else
	// we append relative paths to the search paths maintained by this class.
	QStringList listPaths = QFileInfo(path).isAbsolute() ? QStringList("/") : fileLocations;

	for (const auto& li : listPaths)
	{
		QFileInfo thisPath(QDir(li).filePath(path));
		if (!thisPath.isDir())
			continue;

		QDir thisDir(thisPath.absoluteFilePath());
		for (const auto& fileIt : thisDir.entryList())
		{
			if (fileIt == ".." || fileIt == ".")
				continue;
			QFileInfo fullPath(thisDir.filePath(fileIt));
			if (fileFlagsCheck(fullPath, flags))
				result.insert(fileIt);
		}
	}

	return result;
}

void StelFileMgr::setSearchPaths(const QStringList& paths)
{
	fileLocations = paths;
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

bool StelFileMgr::fileFlagsCheck(const QFileInfo& thePath, const Flags& flags)
{
	const bool exists = thePath.exists();
	
	if (flags & New)
	{
		// if the file already exists, it is not a new file
		if (exists)
			return false;

		// To be able to create a new file, we need to have a
		// parent directory which is writable.
		QFileInfo pInfo(thePath.dir().absolutePath());
		if (!pInfo.exists() || !pInfo.isWritable())
		{
			return false;
		}
	}
	else if (exists)
	{
		if (flags==0)
			return true;
		
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
	makeSureDirExistsAndIsWritable(newDir);
	QFileInfo userDirFI(newDir);
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
	GetSpecialFolderPath SHGetSpecialFolderPath = reinterpret_cast<GetSpecialFolderPath>(library.resolve("SHGetSpecialFolderPathW"));
	if (SHGetSpecialFolderPath)
	{
		TCHAR tpath[MAX_PATH];
		SHGetSpecialFolderPath(Q_NULLPTR, tpath, csidlId, FALSE);
		return QString::fromUtf16(reinterpret_cast<ushort*>(tpath));
	}

	return QString();
}
#endif
