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

	try
	{
		//Make sure that "/modules/PrintSky" exists
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir() + "/modules/PrintSky/");

		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";

		QSettings settings(printskyIniPath, QSettings::IniFormat);
		useInvertColors = settings.value("use_invert_colors", 0.0).toBool();
		scaleToFit=settings.value("use_scale_to_fit", true).toBool();
		orientation=settings.value("orientation", "Portrait").toString();
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
