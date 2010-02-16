/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <config.h>
#include "StelMainWindow.hpp"
#include "StelTranslator.hpp"
#include "StelLogger.hpp"
#include "StelFileMgr.hpp"
#include "CLIProcessor.hpp"
#include "StelIniParser.hpp"

#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QTranslator>
#include <QGLFormat>
#include <QPlastiqueStyle>
#include <QFileInfo>
#include <QtPlugin>

#ifdef MACOSX
#include "StelMacosxDirs.hpp"
#endif

Q_IMPORT_PLUGIN(StelGui)

#ifdef USE_STATIC_PLUGIN_VIRGO
Q_IMPORT_PLUGIN(VirGO)
#endif

#ifdef USE_STATIC_PLUGIN_SVMT
Q_IMPORT_PLUGIN(SVMT)
#endif

#ifdef USE_STATIC_PLUGIN_HELLOSTELMODULE
Q_IMPORT_PLUGIN(HelloStelModule)
#endif

#ifdef USE_STATIC_PLUGIN_ANGLEMEASURE
Q_IMPORT_PLUGIN(AngleMeasure)
#endif

#ifdef USE_STATIC_PLUGIN_COMPASSMARKS
Q_IMPORT_PLUGIN(CompassMarks)
#endif

#ifdef USE_STATIC_PLUGIN_SATELLITES
Q_IMPORT_PLUGIN(Satellites)
#endif

#ifdef USE_STATIC_PLUGIN_TEXTUSERINTERFACE
Q_IMPORT_PLUGIN(TextUserInterface)
#endif

#ifdef USE_STATIC_PLUGIN_LOGBOOK
Q_IMPORT_PLUGIN(LogBook)
#endif

#ifdef USE_STATIC_PLUGIN_OCULARS
Q_IMPORT_PLUGIN(Oculars)
#endif

#ifdef USE_STATIC_PLUGIN_TELESCOPECONTROL
Q_IMPORT_PLUGIN(TelescopeControl)
#endif

//! @class GettextStelTranslator
//! Provides i18n support through gettext.
class GettextStelTranslator : public QTranslator
{
public:
	virtual bool isEmpty() const { return false; }

	virtual QString translate(const char* context, const char* sourceText, const char* comment=0) const
	{
		return q_(sourceText);
	}
};


//! Copies the default configuration file.
//! This function copies the default_config.ini file to config.ini (or other
//! name specified on the command line located in the user data directory.
void copyDefaultConfigFile(const QString& newPath)
{
	QString defaultConfigFilePath;
	try
	{
		defaultConfigFilePath = StelFileMgr::findFile("data/default_config.ini");
	}
	catch (std::runtime_error& e)
	{
		qFatal("ERROR copyDefaultConfigFile failed to locate data/default_config.ini. Please check your installation.");
	}
	QFile::copy(defaultConfigFilePath, newPath);
	if (!StelFileMgr::exists(newPath))
	{
		qFatal("ERROR copyDefaultConfigFile failed to copy file %s  to %s. You could try to copy it by hand.",
			qPrintable(defaultConfigFilePath), qPrintable(newPath));
	}
}


// Main stellarium procedure
int main(int argc, char **argv)
{
	QCoreApplication::setApplicationName("stellarium");
	QCoreApplication::setApplicationVersion(StelUtils::getApplicationVersion());
	QCoreApplication::setOrganizationDomain("stellarium.org");
	QCoreApplication::setOrganizationName("stellarium");

#ifndef USE_OPENGL_ES2
	QApplication::setStyle(new QPlastiqueStyle());
#endif

	// Used for getting system date formatting
	setlocale(LC_TIME, "");
	// We need scanf()/printf() and friends to always work in the C locale,
	// otherwise configuration/INI file parsing will be erroneous.
	setlocale(LC_NUMERIC, "C");

	// Handle command line options for alternative Qt graphics system types.
	// DEFAULT_GRAPHICS_SYSTEM is defined per platform in the main CMakeLists.txt file.
	// Avoid overriding if the user already specified the mode on the CLI.
	bool doSetDefaultGraphicsSystem=true;
	for (int i = 1; i < argc; ++i)
	{
		//QApplication accepts -graphicssystem only if
		//its value is a separate argument (the next value in argv)
		if (QString(argv[i]) == "-graphicssystem")
		{
			doSetDefaultGraphicsSystem = false;
		}
	}
	// If the user didn't set the graphics-system on the command line, use the one provided at compile time.
	if (doSetDefaultGraphicsSystem)
	{
		qDebug() << "Using default graphics system specified at build time: " << DEFAULT_GRAPHICS_SYSTEM;
		QApplication::setGraphicsSystem(DEFAULT_GRAPHICS_SYSTEM);
	}

	// The QApplication MUST be created before the StelFileMgr is initialized.
	QApplication app(argc, argv);

	// Init the file manager
	StelFileMgr::init();

	// Start logging.
	StelLogger::init(StelFileMgr::getUserDir()+"/log.txt");

	// Log command line arguments
	QString argStr;
	QStringList argList;
	for (int i=0; i<argc; ++i)
	{
		argList << argv[i];
		argStr += QString("%1 ").arg(argv[i]);
	}
	StelLogger::writeLog(argStr);

	// Parse for first set of CLI arguments - stuff we want to process before other
	// output, such as --help and --version
	CLIProcessor::parseCLIArgsPreConfig(argList);

	// OK we start the full program.
	// Print the console splash and get on with loading the program
	QString versionLine = QString("This is %1 - http://www.stellarium.org").arg(StelUtils::getApplicationName());
	QString copyrightLine = QString("Copyright (C) 2000-2010 Fabien Chereau et al");
	int maxLength = qMax(versionLine.size(), copyrightLine.size());
	qDebug() << qPrintable(QString(" %1").arg(QString().fill('-', maxLength+2)));
	qDebug() << qPrintable(QString("[ %1 ]").arg(versionLine.leftJustified(maxLength, ' ')));
	qDebug() << qPrintable(QString("[ %1 ]").arg(copyrightLine.leftJustified(maxLength, ' ')));
	qDebug() << qPrintable(QString(" %1").arg(QString().fill('-', maxLength+2)));
	qDebug() << "Writing log file to:" << StelLogger::getLogFileName();
	qDebug() << "File search paths:";
	int n=0;
	foreach (QString i, StelFileMgr::getSearchPaths())
	{
		qDebug() << " " << n << ". " << i;
		++n;
	}

	// Now manage the loading of the proper config file
	QString configName;
	try
	{
		configName = CLIProcessor::argsGetOptionWithArg(argList, "-c", "--config-file", "config.ini").toString();
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: while looking for --config-file option: " << e.what() << ". Using \"config.ini\"";
		configName = "config.ini";
	}

	QString configFileFullPath;
	try
	{
		configFileFullPath = StelFileMgr::findFile(configName, StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::File));
	}
	catch (std::runtime_error& e)
	{
		try
		{
			configFileFullPath = StelFileMgr::findFile(configName, StelFileMgr::New);
		}
		catch (std::runtime_error& e)
		{
			qFatal("Could not create configuration file %s.", qPrintable(configName));
		}
	}

	QSettings* confSettings = NULL;
	if (StelFileMgr::exists(configFileFullPath))
	{
		// Implement "restore default settings" feature.
		bool restoreDefaultConfigFile = false;
		if (CLIProcessor::argsGetOption(argList, "", "--restore-defaults"))
		{
			restoreDefaultConfigFile=true;
		}
		else
		{
			confSettings = new QSettings(configFileFullPath, StelIniFormat, NULL);
			restoreDefaultConfigFile = confSettings->value("main/restore_defaults", false).toBool();
		}
		if (!restoreDefaultConfigFile)
		{
			QString version = confSettings->value("main/version", "0.0.0").toString();
			if (version!=QString(PACKAGE_VERSION))
			{
				QTextStream istr(&version);
				char tmp;
				int v1=0;
				int v2=0;
				istr >> v1 >> tmp >> v2;
				// Config versions less than 0.6.0 are not supported, otherwise we will try to use it
				if (v1==0 && v2<6)
				{
					// The config file is too old to try an importation
					qDebug() << "The current config file is from a version too old for parameters to be imported ("
							 << (version.isEmpty() ? "<0.6.0" : version) << ").\n"
							 << "It will be replaced by the default config file.";
					restoreDefaultConfigFile = true;
				}
				else
				{
					qDebug() << "Attempting to use an existing older config file.";
				}
			}
		}
		if (restoreDefaultConfigFile)
		{
			if (confSettings)
				delete confSettings;
			QString backupFile(configFileFullPath.left(configFileFullPath.length()-3) + QString("old"));
			if (QFileInfo(backupFile).exists())
				QFile(backupFile).remove();
			QFile(configFileFullPath).rename(backupFile);
			copyDefaultConfigFile(configFileFullPath);
			confSettings = new QSettings(configFileFullPath, StelIniFormat);
			qWarning() << "Resetting defaults config file. Previous config file was backed up in " << backupFile;
		}
	}
	else
	{
		qDebug() << "Config file " << configFileFullPath << " does not exist. Copying the default file.";
		copyDefaultConfigFile(configFileFullPath);
		confSettings = new QSettings(configFileFullPath, StelIniFormat);
	}

	Q_ASSERT(confSettings);
	qDebug() << "Config file is: " << configFileFullPath;

	// Override config file values from CLI.
	CLIProcessor::parseCLIArgsPostConfig(argList, confSettings);

#ifdef MACOSX
	StelMacosxDirs::addApplicationPluginDirectory();
#endif

	if (confSettings->value("main/use_glshaders", false).toBool())
	{
		// The user explicitely wants to use GL shaders
		// Default Qt behaviour is alreay to use them if available (although it unstable),
		// so nothing to be done.
	}
	else
	{
		// The default behaviour on windows to is to use the OpenGL1 Qt paint engine because the OpenGL2 one causes slowdown.
#ifdef WIN32
		QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
#else
# ifdef Q_WS_MAC
		// On Leopard (10.5) + ppc architecture, text display is buggy if OpenGL2 Qt paint engine is used.
		if ((QSysInfo::MacintoshVersion == QSysInfo::MV_LEOPARD) && (QSysInfo::ByteOrder == QSysInfo::BigEndian))
			QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
# endif
#endif
	}

	// Initialize translator feature
	try
	{
		StelTranslator::init(StelFileMgr::findFile("data/iso639-1.utf8"));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while loading translations: " << e.what() << endl;
	}
	// Use our gettext translator for Qt translations as well
	GettextStelTranslator trans;
	app.installTranslator(&trans);

	if (!QGLFormat::hasOpenGL())
	{
		QMessageBox::warning(0, "Stellarium", q_("This system does not support OpenGL."));
	}

	StelMainWindow mainWin;
	mainWin.init(confSettings);
	app.exec();
	mainWin.deinit();

	delete confSettings;
	StelLogger::deinit();
	return 0;
}

