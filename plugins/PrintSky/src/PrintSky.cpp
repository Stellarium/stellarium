#include <QDebug>
#include "PrintSky.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelIniParser.hpp"

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
	conf = StelApp::getInstance().getSettings();
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
	addAction("action_PrintSkyDialog", section, N_("Printing Sky"), printskyDialog, "visible", "Alt+Shift+P");

	conf->beginGroup("PrintSky");
	invertColors = conf->value("flag_invert_colors", false).toBool();
	scaleToFit=conf->value("flag_scale_to_fit", true).toBool();
	orientationPortrait=conf->value("flag_portrait_orientation", true).toBool();
	printData=conf->value("flag_print_data", true).toBool();
	printSSEphemerides=conf->value("flag_print_ephemerides", true).toBool();
	conf->endGroup();
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

////! Show dialog printing enabling preview and print buttons
//void PrintSky::initPrintingSky()
//{
//	printskyDialog->setVisible(true);
//	printskyDialog->enableOutputOptions(true);
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
	if (state != invertColors)
	{
		invertColors=state;
		conf->setValue("PrintSky/flag_invert_colors", state);
		emit(invertColorsChanged(state));
	}
}

/* ********************************************************************* */
void PrintSky::setScaleToFit(bool state)
{
	if (state != scaleToFit)
	{
		scaleToFit=state;
		conf->setValue("PrintSky/flag_scale_to_fit", state);
		emit(scaleToFitChanged(state));
	}
}

/* ********************************************************************* */
void PrintSky::setOrientationPortrait(bool state)
{
	if (state != orientationPortrait)
	{
		orientationPortrait = state;
		conf->setValue("PrintSky/flag_portrait_orientation", state);
		emit(orientationChanged(state));
	}
}

/* ********************************************************************* */
void PrintSky::setPrintData(bool state)
{
	if (state != printData)
	{
		printData=state;
		conf->setValue("PrintSky/flag_print_data", state);
		emit(printDataChanged(state));
	}
}

/* ********************************************************************* */
void PrintSky::setPrintSSEphemerides(bool state)
{
	if (state != printSSEphemerides)
	{
		printSSEphemerides=state;
		conf->setValue("PrintSky/flag_print_ephemerides", state);
		emit(printSSEphemeridesChanged(state));
	}
}
