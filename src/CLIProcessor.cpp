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

#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>

#include <cstdio>
#include <iostream>

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
			  << "--verbose               : Even more diagnostic output in logfile \n"
			  << "                          (esp. multimedia handling)\n"
			  << "--compat33 (or -C)      : Request OpenGL 3.3 Compatibility Profile\n"
			  << "                          May help for certain driver configurations. Mac?\n"
			  << "--fix-text (or -t)      : May fix text rendering problems\n"
			  << "--scale-gui  <scale factor>  : Scaling the GUI according to scale factor\n"
			  << "--dump-opengl-details (or -d) : dump information about OpenGL support to logfile.\n"
			  << "                          Use this is you have graphics problems\n"
			  << "                          and want to send a bug report\n"
			  << "--full-screen (or -f)   : With argument \"yes\" or \"no\" over-rides\n"
			  << "                          the full screen setting in the config file\n"
			  << "--screenshot-dir        : Specify directory to save screenshots\n"
			  << "--startup-script        : Specify name of startup script\n"
			  << "--home-planet           : Specify observer planet (English name)\n"
			  << "--longitude             : Specify longitude, e.g. +53d58\\'16.65\\\"\n"
			  << "--latitude              : Specify latitude, e.g. -1d4\\'27.48\\\"\n"
			  << "--altitude              : Specify observer altitude in meters\n"
			  << "--list-landscapes       : Print a list of valid landscape IDs\n"
		          << "--landscape             : Start using landscape whose ID (dir name)\n"
		          << "                          is passed as parameter to option\n"
		          << "--sky-date              : Specify sky date in format yyyymmdd\n"
		          << "--sky-time              : Specify sky time in format hh:mm:ss\n"
		          << "--fov                   : Specify the field of view (degrees)\n"
		          << "--projection-type       : Specify projection type, e.g. stereographic\n"
		          << "--restore-defaults      : Delete existing config.ini and use defaults\n"
		          << "--multires-image        : With filename / URL argument, specify a\n"
			  << "                          multi-resolution image to load\n"
#ifdef Q_OS_WIN
			  << "--angle-mode (or -a)    : Use ANGLE as OpenGL ES2 rendering engine (autodetect driver)\n"
			  << "--angle-d3d9 (or -9)    : Force use Direct3D 9 for ANGLE OpenGL ES2 rendering engine\n"
			  << "--angle-d3d11           : Force use Direct3D 11 for ANGLE OpenGL ES2 rendering engine\n"
			  << "--angle-warp            : Force use the Direct3D 11 software rasterizer for ANGLE OpenGL ES2 rendering engine\n"
			  << "--mesa-mode (or -m)     : Use MESA as software OpenGL rendering engine\n"
			  << "--safe-mode (or -s)     : Synonymous to --mesa-mode \n"
			  #ifdef ENABLE_SPOUT
			  << "--spout (or -S) <sky|all> : Act as SPOUT sender (Sky only/including GUI)\n"
			  << "--spout-name <name>     : Set particular name for SPOUT sender.\n"
			  #endif
#endif
			  << " \n";
		exit(0);
	}

	if (argsGetOption(argList, "", "--verbose"))
		qApp->setProperty("verbose", true);

	if (argsGetOption(argList, "-C", "--compat33"))
		qApp->setProperty("onetime_compat33", true);

	if (argsGetOption(argList, "-t", "--fix-text"))
		qApp->setProperty("text_texture", true); // Will be observed in StelPainter::drawText()

	#ifdef Q_OS_WIN
	if (argsGetOption(argList, "-s", "--safe-mode"))
		qApp->setProperty("onetime_mesa_mode", true);

	if (argsGetOption(argList, "-a", "--angle-mode"))
		qApp->setProperty("onetime_angle_mode", true);

	if (argsGetOption(argList, "-9", "--angle-d3d9"))
	{
		qputenv("QT_ANGLE_PLATFORM", "d3d9");
		qApp->setProperty("onetime_angle_mode", true);
	}
	if (argsGetOption(argList, "", "--angle-d3d11"))
	{
		qputenv("QT_ANGLE_PLATFORM", "d3d11");
		qApp->setProperty("onetime_angle_mode", true);
	}
	if (argsGetOption(argList, "", "--angle-warp"))
	{
		qputenv("QT_ANGLE_PLATFORM", "warp");
		qApp->setProperty("onetime_angle_mode", true);
	}
	if (argsGetOption(argList, "-m", "--mesa-mode"))
		qApp->setProperty("onetime_mesa_mode", true);

	#endif
	if (argsGetOption(argList, "", "--list-landscapes"))
	{
		const QSet<QString>& landscapeIds = StelFileMgr::listContents("landscapes", StelFileMgr::Directory);
		for (const auto& i : landscapeIds)
		{
			// finding the file will throw an exception if it is not found
			// in that case we won't output the landscape ID as it cannot work
			if (!StelFileMgr::findFile("landscapes/" + i + "/landscape.ini").isEmpty())
				std::cout << qPrintable(i) << std::endl;
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
#ifdef ENABLE_SPOUT
	QString spoutStr, spoutName;
#endif
	try
	{
		bool dumpOpenGLDetails = argsGetOption(argList, "-d", "--dump-opengl-details");
		qApp->setProperty("dump_OpenGL_details", dumpOpenGLDetails);
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
#ifdef ENABLE_SPOUT
		// For now, we default to spout=sky when no extra option is given. Later, we should also accept "all".
		// Unfortunately, this still throws an exception when no optarg string is given.
		spoutStr  = argsGetOptionWithArg(argList, "-S", "--spout", "").toString();
		spoutName = argsGetOptionWithArg(argList, "", "--spout-name", "").toString();
#endif
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
	if (!landscapeId.isEmpty()) confSettings->setValue("location_run_once/landscape_name", landscapeId);
	if (!homePlanet.isEmpty()) confSettings->setValue("location_run_once/home_planet", homePlanet);
	if (altitude!=-1) confSettings->setValue("location_run_once/altitude", altitude);
	if (!longitude.isEmpty()) confSettings->setValue("location_run_once/longitude", StelUtils::getDecAngle(longitude)); // Store longitude in radian
	if (!latitude.isEmpty()) confSettings->setValue("location_run_once/latitude", StelUtils::getDecAngle(latitude)); // Store latitude in radian

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

	if (fov>0.0f) confSettings->setValue("navigation/init_fov", fov);
	if (!projectionType.isEmpty()) confSettings->setValue("projection/type", projectionType);
	if (!screenshotDir.isEmpty())
	{
		try
		{
			QString newShotDir = QDir::fromNativeSeparators(argsGetOptionWithArg(argList, "", "--screenshot-dir", "").toString());
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
		const QString& confScreenshotDir = QDir::fromNativeSeparators(confSettings->value("main/screenshot_dir", "").toString());
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

#ifdef ENABLE_SPOUT
	if (!spoutStr.isEmpty())
	{
		if (spoutStr=="all")
			qApp->setProperty("spout", "all");
		else
			qApp->setProperty("spout", "sky");
	}
	else
		qApp->setProperty("spout", "none");
	if (!spoutName.isEmpty())
		qApp->setProperty("spoutName", spoutName);
#else
	qApp->setProperty("spout", "none");
#endif
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
		if ((!shortOpt.isEmpty() && args.at(i).left(shortOpt.length()+1)==shortOpt+"="))
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
		else if ((!shortOpt.isEmpty() && args.at(i)==shortOpt) || args.at(i)==longOpt)
		{
			if (i+1>=lastOptIdx)
			{
				// i.e., option given as last option, but without arguments. Last chance: default value!
				if (defaultValue.isValid())
				{
					return defaultValue;
				}
				else
				{
					throw (std::runtime_error(qPrintable("optarg_missing ("+longOpt+")")));
				}
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
