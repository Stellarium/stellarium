#include <QDebug>
#include <QKeyEvent>
#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelIniParser.hpp"
#include "StelVertexArray.hpp"
#include "PrintSky.hpp"
#include <QAction>
#include "StelMainView.hpp"
#include <QOpenGLWidget>

//! This method is the one called automatically by the StelModuleMgr just
//! after loading the dynamic library
StelModule* PrintSkyStelPluginInterface::getStelModule() const
{
	return new PrintSky();
}

StelPluginInfo PrintSkyStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	//Q_INIT_RESOURCE(PrintSky);

	StelPluginInfo info;
	info.id = "PrintSky";
	info.displayedName =  N_("Print Sky");
	info.authors = "Pep Pujols, Georg Zotti";
	info.contact = "maslarocaxica@gmail.com";
	info.description = N_("Provides a report printing option");
	info.version = PRINTSKY_PLUGIN_VERSION;
	info.license = PRINTSKY_PLUGIN_LICENSE;
	return info;
}

PrintSky::PrintSky() :
	invertColors(false),
	scaleToFit(false),
	printData(true),
	printSSEphemerides(true)
{
	setObjectName("PrintSky");
}

PrintSky::~PrintSky()
{
	delete printskyDialog;
	printskyDialog = Q_NULLPTR;
}

void PrintSky::init()
{
	printskyDialog = new PrintSkyDialog();

	// create action for enable/disable & hook up signals
	QString section=N_("Print Sky");
	addAction("action_PrintSkyDialog", section, N_("Printing Sky"), printskyDialog, "visible", "Ctrl+Alt+P");

	try
	{
		//Make sure that "/modules/PrintSky" exists
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir() + "/modules/PrintSky/");

		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";

		QSettings settings(printskyIniPath, QSettings::IniFormat);
		invertColors = settings.value("use_invert_colors", 0.0).toBool();
		scaleToFit=settings.value("use_scale_to_fit", true).toBool();
		orientationPortrait=settings.value("orientation_portrait", true).toBool();
		printData=settings.value("print_data", true).toBool();
		printSSEphemerides=settings.value("print_SS_ephemerides", true).toBool();
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate printsky.ini file or create a default one for PrintSky plugin: " << e.what();
	}	
}

void PrintSky::update(double deltaTime)
{
	Q_UNUSED(deltaTime)
}

//! Draw any parts on the screen which are for our module
void PrintSky::draw(StelCore* core)
{
	Q_UNUSED(core)
}

//! Determine which "layer" the plugin's drawing will happen on.
double PrintSky::getCallOrder(StelModuleActionName actionName) const
{
	Q_UNUSED(actionName)
	return 0;
}

void PrintSky::handleKeys(QKeyEvent* event)
{
	event->setAccepted(false);
}

void PrintSky::handleMouseClicks(class QMouseEvent* event)
{
	event->setAccepted(false);
}

bool PrintSky::handleMouseMoves(int x, int y, Qt::MouseButtons b)
{
	Q_UNUSED(x)
	Q_UNUSED(y)
	Q_UNUSED(b)
	return false;
}

////! Show dialog printing enabling preview and print buttons
//void PrintSky::initPrintingSky()
//{
//	printskyDialog->setVisible(true);
//	printskyDialog->enableOutputOptions(true);
//}

//void PrintSky::setStelStyle(const QString& section)
//{
//	printskyDialog->updateStyle();
//}

bool PrintSky::configureGui(bool show)
{
	if (show)
	{
		printskyDialog->setVisible(true);
		printskyDialog->enableOutputOptions(true); // was false, but there is no use for a dysfunctional GUI.
	}

	return true;
}

/* ********************************************************************* */
void PrintSky::setInvertColors(bool state)
{
	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		if (state != invertColors)
		{
			invertColors=state;
			settings.setValue("use_invert_colors", state);
			emit(invertColorsChanged(state));
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate printsky.ini file or create a default one for PrintSky plugin: " << e.what();
	}
}

/* ********************************************************************* */
void PrintSky::setScaleToFit(bool state)
{
	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		if (state != scaleToFit)
		{
			scaleToFit=state;
			settings.setValue("use_scale_to_fit", state);
			emit(scaleToFitChanged(state));
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate printsky.ini file or create a default one for PrintSky plugin: " << e.what();
	}
}

/* ********************************************************************* */
void PrintSky::setOrientationPortrait(bool state)
{
	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		if (state != orientationPortrait)
		{
			settings.setValue("orientation_portrait", state);
			emit(orientationChanged(state));
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate printsky.ini file or create a default one for PrintSky plugin: " << e.what();
	}
}

/* ********************************************************************* */
void PrintSky::setPrintData(bool state)
{
	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		if (state != printData)
		{
			printData=state;
			settings.setValue("print_data", state);
			emit(printDataChanged(state));
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate printsky.ini file or create a default one for PrintSky plugin: " << e.what();
	}
}

/* ********************************************************************* */
void PrintSky::setPrintSSEphemerides(bool state)
{
	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		if (state != printSSEphemerides)
		{
			printSSEphemerides=state;
			settings.setValue("print_SS_ephemerides", state);
			emit(printSSEphemeridesChanged(state));
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate printsky.ini file or create a default one for PrintSky plugin: " << e.what();
	}
}
