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
#include <QMessageBox>

#include <cstdio>
#include <iostream>

#ifdef Q_OS_WIN
#include <Windows.h>
#ifndef _SHOBJ_H
	#include <ShlObj.h>
	#include <QLibrary>
#endif
#endif

#include "StelFileMgr.hpp"

// Initialize static members.
std::vector<QDir> StelFileMgr::searchDirectories;
QDir StelFileMgr::configDir;
QDir StelFileMgr::dataDir;
QDir StelFileMgr::screenshotDir;
QDir StelFileMgr::installDir;

void StelFileMgr:: init(const StelFileMgrOptions& options)
{
    configDir = resolveConfigDirectory(options.configDirPath);
    dataDir = resolveDataDirectory(options.dataDirPath);
    initInstallDirectory();

    // migrate legacy user directory into config and data directories
    migrateLegacyUserDirectory(options._test_legacyUserPath);

    if (!configDir.exists())
	{
        qWarning() << "User config directory does not exist: " << QDir::toNativeSeparators(configDir.absolutePath());
	}
	try
	{
        makeSureDirExistsAndIsWritable(configDir);
	}
	catch (std::runtime_error &e)
	{
		qFatal("Error: cannot create user config directory: %s", e.what());
	}

    if (!dataDir.exists())
	{
        qWarning() << "Application data directory does not exist: " << QDir::toNativeSeparators(dataDir.absolutePath());
	}
	try
	{
        makeSureDirExistsAndIsWritable(dataDir);
	}
	catch (std::runtime_error &e)
	{
		qFatal("Error: cannot create application data directory: %s", e.what());
	}

	// update file locations to include install, config, and data directories
	searchDirectories = std::vector<QDir>{configDir, dataDir, installDir};
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

	for (const auto& dir : searchDirectories)
	{
		const QFileInfo finfo(dir.filePath(path));
		if (fileFlagsCheck(finfo, flags))
			return dir.filePath(path);
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

	for (const auto& searchDir : searchDirectories)
	{
		const QFileInfo finfo(searchDir.filePath(path));
		if (fileFlagsCheck(finfo, flags))
			filePaths.append(searchDir.filePath(path));
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
	auto listPaths = QFileInfo(path).isAbsolute() ? std::vector<QDir>{QDir("/")} : searchDirectories;

	for (const auto& dir : listPaths)
	{
		QFileInfo thisPath(dir.filePath(path));
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

QString StelFileMgr::getLegacyUserDirPath()
{
    // Set the userDir member.
#ifdef Q_OS_WIN
    QString winApiPath = getWin32SpecialDirPath(CSIDL_APPDATA);
    if (winApiPath.isEmpty())
        return QString();
    }

    return QString(winApiPath + "\\Stellarium");
#elif defined(Q_OS_MAC)
    return QString(QDir::homePath() + "/Library/Application Support/Stellarium");
#else
    return QString(QDir::homePath() + "/.stellarium");
#endif
}

QDir StelFileMgr::resolveConfigDirectory(const QString& optionalConfigPath)
{
    if (optionalConfigPath.isEmpty()) {
        const auto appConfigLocation = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        qDebug() << "Using XDG App Config Directory: " << appConfigLocation.path();
        return appConfigLocation;
    }

    qDebug() << "Using ENV option config directory: " << optionalConfigPath;
    return QDir(optionalConfigPath);
}

//! Determine the directory to be used for application data storage
QDir StelFileMgr::resolveDataDirectory(const QString& optionalDataPath)
{
    if(optionalDataPath.isEmpty()) {
        const auto appConfigLocation = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        qDebug() << "Using XDG App Config Directory: " << appConfigLocation.path();
        return appConfigLocation;
    }

    qDebug() << "Using ENV defined data directory: " << optionalDataPath;
    return QDir(optionalDataPath);
}

void StelFileMgr::migrateLegacyUserDirectory(const QString &_test_legacyUserPath)
{
    const auto legacyPath = _test_legacyUserPath.isEmpty() ?
                getLegacyUserDirPath() :
                _test_legacyUserPath;
    const auto legacyUserDirectory = QDir(legacyPath);

//    if (legacyUserDirectory.exists() && (!configDir.exists() || !dataDir.exists())) {
//          QString question("Stellarium has changed it's configuration directory location.\n");
//          question.append("Would you like to migrate your existing config?");
//          auto reply =  QMessageBox::question(nullptr, "Migrate Legacy Config?", question, QMessageBox::Yes | QMessageBox::No);

//          // discarding removes old legacy directory
//          if (reply == QMessageBox::Yes) {
//              qDebug() << "Copying Legacy Directory (" << legacyUserDirectory.path() << ") to new location ("<< appConfigLocation.path() <<")";

//              if (!appConfigLocation.exists()) {
//                  QDir().mkpath(appConfigLocation.path());
//              }

//              // move all the files from the legacy directory ithe new directory
//              for (auto entry: legacyUserDirectory.entryInfoList()) {
//                  if (entry.fileName() != "." && entry.fileName() != "..") {
//                      auto destination = appConfigLocation.path() + "/" + entry.fileName();
//                      qDebug() << "Moving: " << entry.filePath() << " To: " << destination;
//                      QFile(entry.filePath()).rename(destination);
//                  }
//              }

//              // remove the old directory
//              legacyUserDirectory.removeRecursively();

//          } else {
//              auto reply =  QMessageBox::question(nullptr,
//                  "Remove Legacy Config?",
//                  "Would you like to remove your legacy config directory?",
//                  QMessageBox::Yes | QMessageBox::No);

//              if (reply == QMessageBox::Yes) {
//                  legacyUserDirectory.removeRecursively();
//              }
//          }
//      }
}

//! Set the installation directory
void StelFileMgr::initInstallDirectory()
{
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
}

void StelFileMgr::setSearchDirectories(const std::vector<QDir>& newSearchDirectories)
{
	searchDirectories = newSearchDirectories;
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

const QDir StelFileMgr::getDesktopDir()
{
    const auto standardDesktop = QDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

	if (!standardDesktop.exists()) {
		return QDir();
    }

	return standardDesktop;
}

const QDir& StelFileMgr::getConfigDir()
{
    return configDir;
}

void StelFileMgr::setConfigDir(const QString& newConfigDir)
{
    makeSureDirExistsAndIsWritable(newConfigDir);
    configDir = QDir(newConfigDir);
    searchDirectories[0] = configDir;

}

const QDir& StelFileMgr::getDataDir()
{
    return dataDir;
}

void StelFileMgr::setDataDir(const QString& newDataDir)
{
    makeSureDirExistsAndIsWritable(newDataDir);
    dataDir = QDir(newDataDir);
    searchDirectories[1] = dataDir;
}

const QDir& StelFileMgr::getInstallationDir()
{
	return installDir;
}

const QDir& StelFileMgr::getScreenshotDir()
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
	QFileInfo localePath = QFileInfo(getInstallationDir().filePath("/translations"));
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
const QDir StelFileMgr::getCacheDir()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
}

void StelFileMgr::makeSureDirExistsAndIsWritable(const QDir& dir)
{
    auto dirFileInfo = QFileInfo(dir.absolutePath());

    if (!dir.exists())
	{
		// The modules directory doesn't exist, lets create it.
        qDebug() << "Creating directory " << QDir::toNativeSeparators(dir.absolutePath());
		if (!QDir("/").mkpath(dir.absolutePath()))
		{
			throw std::runtime_error(QString("Could not create directory: " + dir.absolutePath()).toStdString());
		}

		if (!dirFileInfo.isWritable())
		{
			throw std::runtime_error(QString("Directory is not writable: " + dir.absolutePath()).toStdString());
		}
	}
	else if (!dirFileInfo.isWritable())
	{
		throw std::runtime_error(QString("Directory is not writable: " + dir.absolutePath()).toStdString());
    }

}

void StelFileMgr::makeSureDirExistsAndIsWritable(const QString& dirFullPath)
{
    makeSureDirExistsAndIsWritable(QDir(dirFullPath));
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
