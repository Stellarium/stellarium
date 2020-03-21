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
	info.description = N_("Provides a system printing sky");
	info.version = PRINTSKY_PLUGIN_VERSION;
	info.license = PRINTSKY_PLUGIN_LICENSE;
	return info;
}

PrintSky::PrintSky() :
	useInvertColors(false),
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
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);

	// FIXME: correct action syntax after fixing the properties.
	//gui->addGuiActions("actionInit_Printing_Sky", N_("Printing Sky"), "Ctrl+P", N_("Plugin Key Bindings"), true, false);
	//gui->getGuiActions("actionInit_Printing_Sky")->setChecked(true);
	//connect(gui->getGuiActions("actionInit_Printing_Sky"), SIGNAL(triggered()), this, SLOT(initPrintingSky()));
	QString section=N_("Print Sky");
	addAction("action_PrintSkyDialog", section, N_("Printing Sky"), this, SLOT(initPrintingSky), "Ctrl+P");

	try
	{
		//Make sure that "/modules/PrintSky" exists
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir() + "/modules/PrintSky/");

		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";

		QSettings settings(printskyIniPath, QSettings::IniFormat);
		useInvertColors = settings.value("use_invert_colors", 0.0).toBool();
		scaleToFit=settings.value("use_scale_to_fit", true).toBool();
		//orientation=settings.value("orientation", "Portrait").toString();
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

//! Determine which "layer" the plagin's drawing will happen on.
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

//! Show dialog printing enabling preview and print buttons
void PrintSky::initPrintingSky()
{
	printskyDialog->setVisible(true);
	printskyDialog->enableOutputOptions(true);
}

//void PrintSky::setStelStyle(const QString& section)
//{
//	printskyDialog->updateStyle();
//}

bool PrintSky::configureGui(bool show)
{
	if (show)
	{
		printskyDialog->setVisible(true);
		printskyDialog->enableOutputOptions(false);
	}

	return true;
}

/* ********************************************************************* */
void PrintSky::setInvertColorsState(bool state)
{
	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		//bool useInvertColors = settings.value("use_invert_colors", 0.0).toBool();
		if (state != useInvertColors)
		{
			useInvertColors=state;
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
void PrintSky::setScaleToFitState(bool state)
{
	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		//bool useScaleToFit = settings.value("use_scale_to_fit", 0.0).toBool();
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
void PrintSky::setOrientationPortraitState(bool state)
{
	//Q_UNUSED(state)
	//QString newOrientation=(ui->orientationPortraitRadioButton->isChecked()? "Portrait": "Landscape");

	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		//QString currentOrientation = settings.value("orientation", "Portrait").toString();
		//if (newOrientation != currentOrientation)
		{
			//settings.setValue("orientation", newOrientation);
			settings.setValue("orientation_portrait", state);
			emit(orientationChanged(true));
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate printsky.ini file or create a default one for PrintSky plugin: " << e.what();
	}

}

/* ********************************************************************* */
void PrintSky::setPrintDataState(bool state)
{
	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		//bool printData = settings.value("print_data", 0.0).toBool();
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
void PrintSky::setPrintSSEphemeridesState(bool state)
{
	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		//bool printSSEphemerides = settings.value("print_SS_ephemerides", 0.0).toBool();
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
