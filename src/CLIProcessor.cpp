/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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

#include "CLIProcessor.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"

#include <QSettings>
#include <QDateTime>
#include <QDebug>
#include <iostream>

#include <QApplication>

void CLIProcessor::parseCLIArgsPreConfig(const QStringList& argList)
{
	if (argsGetOption(argList, "-v", "--version"))
	{
		std::cout << qPrintable(StelUtils::getApplicationName()) << std::endl;
		exit(0);
	}

	if (argsGetOption(argList, "-h", "--help"))
	{
		// Get the basename of binary
		QString binName = argList.at(0);
		binName.remove(QRegExp("^.*[/\\\\]"));

		std::cout << "Usage:\n"
		          << "  "
		          << qPrintable(binName) << " [options]\n\n"
		          << "Options:\n"
		          << "--version (or -v)       : Print program name and version and exit.\n"
		          << "--help (or -h)          : This cruft.\n"
		          << "--config-file (or -c)   : Use an alternative name for the config file\n"
		          << "--user-dir (or -u)      : Use an alternative user data directory\n"
		          << "--safe-mode (or -s)     : Disable shaders and use older GL renderer\n"
		          << "                          Try this is you have graphics problems\n"
		          << "--full-screen (or -f)   : With argument \"yes\" or \"no\" over-rides\n"
		          << "                          the full screen setting in the config file\n"
		          << "--screenshot-dir        : Specify directory to save screenshots\n"
		          << "--startup-script        : Specify name of startup script\n"
		          << "--home-planet           : Specify observer planet (English name)\n"
		          << "--altitude              : Specify observer altitude in meters\n"
		          << "--longitude             : Specify longitude, e.g. +53d58\\'16.65\\\"\n"
		          << "--latitude              : Specify latitude, e.g. -1d4\\'27.48\\\"\n"
		          << "--list-landscapes       : Print a list of value landscape IDs\n"
		          << "--landscape             : Start using landscape whose ID (dir name)\n"
		          << "                          is passed as parameter to option\n"
		          << "--sky-date              : Specify sky date in format yyyymmdd\n"
		          << "--sky-time              : Specify sky time in format hh:mm:ss\n"
		          << "--fov                   : Specify the field of view (degrees)\n"
		          << "--projection-type       : Specify projection type, e.g. stereographic\n"
		          << "--restore-defaults      : Delete existing config.ini and use defaults\n"
		          << "--multires-image        : With filename / URL argument, specify a\n"
		          << "                          multi-resolution image to load\n";
		exit(0);
	}

	if (argsGetOption(argList, "-s", "--safe-mode"))
	{
		qApp->setProperty("onetime_safe_mode", true);
	}

	if (argsGetOption(argList, "", "--list-landscapes"))
	{
		const QSet<QString>& landscapeIds = StelFileMgr::listContents("landscapes", StelFileMgr::Directory);
		foreach (const QString& i, landscapeIds)
		{
			try
			{
				// finding the file will throw an exception if it is not found
				// in that case we won't output the landscape ID as it cannot work
				StelFileMgr::findFile("landscapes/" + i + "/landscape.ini");
				std::cout << qPrintable(i) << std::endl;
			}
			catch (std::runtime_error& e){}
		}
		exit(0);
	}

	try
	{
		QString newUserDir;
		newUserDir = argsGetOptionWithArg(argList, "-u", "--user-dir", "").toString();
		if (newUserDir!="" && !newUserDir.isEmpty())
			StelFileMgr::setUserDir(newUserDir);
	}
	catch (std::runtime_error& e)
	{
		qCritical() << "ERROR: while processing --user-dir option: " << e.what();
		exit(1);
	}
}

void CLIProcessor::parseCLIArgsPostConfig(const QStringList& argList, QSettings* confSettings)
{
	// Over-ride config file options with command line options
	// We should catch exceptions from argsGetOptionWithArg...
	int fullScreen, altitude;
	float fov;
	QString landscapeId, homePlanet, longitude, latitude, skyDate, skyTime;
	QString projectionType, screenshotDir, multiresImage, startupScript;
	try
	{
		fullScreen = argsGetYesNoOption(argList, "-f", "--full-screen", -1);
		landscapeId = argsGetOptionWithArg(argList, "", "--landscape", "").toString();
		homePlanet = argsGetOptionWithArg(argList, "", "--home-planet", "").toString();
		altitude = argsGetOptionWithArg(argList, "", "--altitude", -1).toInt();
		longitude = argsGetOptionWithArg(argList, "", "--longitude", "").toString();
		latitude = argsGetOptionWithArg(argList, "", "--latitude", "").toString();
		skyDate = argsGetOptionWithArg(argList, "", "--sky-date", "").toString();
		skyTime = argsGetOptionWithArg(argList, "", "--sky-time", "").toString();
		fov = argsGetOptionWithArg(argList, "", "--fov", -1.f).toFloat();
		projectionType = argsGetOptionWithArg(argList, "", "--projection-type", "").toString();
		screenshotDir = argsGetOptionWithArg(argList, "", "--screenshot-dir", "").toString();
		multiresImage = argsGetOptionWithArg(argList, "", "--multires-image", "").toString();
		startupScript = argsGetOptionWithArg(argList, "", "--startup-script", "").toString();
	}
	catch (std::runtime_error& e)
	{
		qCritical() << "ERROR while checking command line options: " << e.what();
		exit(0);
	}

	// Will be -1 if option is not found, in which case we don't change anything.
	if (fullScreen==1)
		confSettings->setValue("video/fullscreen", true);
	else if (fullScreen==0)
		confSettings->setValue("video/fullscreen", false);
	if (!landscapeId.isEmpty()) confSettings->setValue("init_location/landscape_name", landscapeId);
	if (!homePlanet.isEmpty()) confSettings->setValue("init_location/home_planet", homePlanet);
	if (altitude!=-1) confSettings->setValue("init_location/altitude", altitude);
	if (!longitude.isEmpty())
	{
		QRegExp longLatRx("[\\-+]?\\d+d\\d+\\'\\d+(\\.\\d+)?\"");
		if (longLatRx.exactMatch(longitude))
			confSettings->setValue("init_location/longitude", longitude);
		else
			qWarning() << "WARNING: --longitude argument has unrecognised format";
	}
	if (!latitude.isEmpty())
	{
		QRegExp longLatRx("[\\-+]?\\d+d\\d+\\'\\d+(\\.\\d+)?\"");
		if (longLatRx.exactMatch(latitude))
			confSettings->setValue("init_location/latitude", latitude);
		else
			qWarning() << "WARNING: --latitude argument has unrecognised format";
	}

	if (!skyDate.isEmpty() || !skyTime.isEmpty())
	{
		// Get the Julian date for the start of the current day
		// and the extra necessary for the time of day as separate
		// components.  Then if the --sky-date and/or --sky-time flags
		// are set we over-ride the component, and finally add them to
		// get the full julian date and set that.

		// First, lets determine the Julian day number and the part for the time of day
		QDateTime now = QDateTime::currentDateTime();
		double skyDatePart = now.date().toJulianDay();
		double skyTimePart = StelUtils::qTimeToJDFraction(now.time());

		// Over-ride the skyDatePart if the user specified the date using --sky-date
		if (!skyDate.isEmpty())
		{
			// validate the argument format, we will tolerate yyyy-mm-dd by removing all -'s
			QRegExp dateRx("\\d{8}");
			if (dateRx.exactMatch(skyDate.remove("-")))
				skyDatePart = QDate::fromString(skyDate, "yyyyMMdd").toJulianDay();
			else
				qWarning() << "WARNING: --sky-date argument has unrecognised format  (I want yyyymmdd)";
		}

		if (!skyTime.isEmpty())
		{
			QRegExp timeRx("\\d{1,2}:\\d{2}:\\d{2}");
			if (timeRx.exactMatch(skyTime))
				skyTimePart = StelUtils::qTimeToJDFraction(QTime::fromString(skyTime, "hh:mm:ss"));
			else
				qWarning() << "WARNING: --sky-time argument has unrecognised format (I want hh:mm:ss)";
		}

		confSettings->setValue("navigation/startup_time_mode", "preset");
		confSettings->setValue("navigation/preset_sky_time", skyDatePart + skyTimePart);
	}

	if (!multiresImage.isEmpty())
		confSettings->setValue("skylayers/clilayer", multiresImage);
	else
	{
		confSettings->remove("skylayers/clilayer");
	}

	if (!startupScript.isEmpty())
	{
		qApp->setProperty("onetime_startup_script", startupScript);
	}

	if (fov>0.0) confSettings->setValue("navigation/init_fov", fov);
	if (!projectionType.isEmpty()) confSettings->setValue("projection/type", projectionType);
	if (!screenshotDir.isEmpty())
	{
		try
		{
			QString newShotDir = argsGetOptionWithArg(argList, "", "--screenshot-dir", "").toString();
			if (!newShotDir.isEmpty())
				StelFileMgr::setScreenshotDir(newShotDir);
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "WARNING: problem while setting screenshot directory for --screenshot-dir option: " << e.what();
		}
	}
	else
	{
		const QString& confScreenshotDir = confSettings->value("main/screenshot_dir", "").toString();
		if (!confScreenshotDir.isEmpty())
		{
			try
			{
				StelFileMgr::setScreenshotDir(confScreenshotDir);
			}
			catch (std::runtime_error& e)
			{
				qWarning() << "WARNING: problem while setting screenshot from config file setting: " << e.what();
			}
		}
	}
}


bool CLIProcessor::argsGetOption(const QStringList& args, QString shortOpt, QString longOpt)
{
	bool result=false;

		// Don't see anything after a -- as an option
	int lastOptIdx = args.indexOf("--");
	if (lastOptIdx == -1)
		lastOptIdx = args.size();

	for (int i=0;i<lastOptIdx;++i)
	{
		if ((!shortOpt.isEmpty() && args.at(i)==shortOpt) || args.at(i)==longOpt)
		{
			result = true;
			i=args.size();
		}
	}
	return result;
}

QVariant CLIProcessor::argsGetOptionWithArg(const QStringList& args, QString shortOpt, QString longOpt, QVariant defaultValue)
{
	// Don't see anything after a -- as an option
	int lastOptIdx = args.indexOf("--");
	if (lastOptIdx == -1)
		lastOptIdx = args.size();

	for (int i=0; i<lastOptIdx; i++)
	{
		bool match(false);
		QString argStr;

		// form -n=arg
		if ((shortOpt!="" && args.at(i).left(shortOpt.length()+1)==shortOpt+"="))
		{
			match=true;
			argStr=args.at(i).right(args.at(i).length() - shortOpt.length() - 1);
		}
		// form --number=arg
		else if (args.at(i).left(longOpt.length()+1)==longOpt+"=")
		{
			match=true;
			argStr=args.at(i).right(args.at(i).length() - longOpt.length() - 1);
		}
		// forms -n arg and --number arg
		else if ((shortOpt!="" && args.at(i)==shortOpt) || args.at(i)==longOpt)
		{
			if (i+1>=lastOptIdx)
			{
				throw (std::runtime_error(qPrintable("optarg_missing ("+longOpt+")")));
			}
			else
			{
				match=true;
				argStr=args.at(i+1);
				i++;  // skip option argument in next iteration
			}
		}

		if (match)
		{
			return QVariant(argStr);
		}
	}
	return defaultValue;
}

int CLIProcessor::argsGetYesNoOption(const QStringList& args, QString shortOpt, QString longOpt, int defaultValue)
{
	QString strArg = argsGetOptionWithArg(args, shortOpt, longOpt, "").toString();
	if (strArg.isEmpty())
	{
		return defaultValue;
	}
	if (strArg.compare("yes", Qt::CaseInsensitive)==0
		   || strArg.compare("y", Qt::CaseInsensitive)==0
		   || strArg.compare("true", Qt::CaseInsensitive)==0
		   || strArg.compare("t", Qt::CaseInsensitive)==0
		   || strArg.compare("on", Qt::CaseInsensitive)==0
		   || strArg=="1")
	{
		return 1;
	}
	else if (strArg.compare("no", Qt::CaseInsensitive)==0
			|| strArg.compare("n", Qt::CaseInsensitive)==0
			|| strArg.compare("false", Qt::CaseInsensitive)==0
			|| strArg.compare("f", Qt::CaseInsensitive)==0
			|| strArg.compare("off", Qt::CaseInsensitive)==0
			|| strArg=="0")
	{
		return 0;
	}
	else
	{
		throw (std::runtime_error("optarg_type"));
	}
}
