/*
 * Copyright (C) 2014 Georg Zotti
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
#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelVertexArray.hpp"
#include "ArchaeoLines.hpp"
#include "ArchaeoLinesDialog.hpp"

#include <QDebug>
#include <QTimer>
#include <QPixmap>
#include <QtNetwork>
#include <QSettings>
#include <QKeyEvent>
#include <QMouseEvent>
#include <cmath>

//! This method is the one called automatically by the StelModuleMgr just
//! after loading the dynamic library
StelModule* ArchaeoLinesStelPluginInterface::getStelModule() const
{
	return new ArchaeoLines();
}

StelPluginInfo ArchaeoLinesStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(ArchaeoLines);

	StelPluginInfo info;
	info.id = "ArchaeoLines";
	info.displayedName = N_("ArchaeoLines");
	info.authors = "Georg Zotti";
	info.contact = "http://homepage.univie.ac.at/Georg.Zotti";
	info.description = N_("Provides a tool for archaeoastronomical alignment studies");
	info.version = ARCHAEOLINES_VERSION;
	return info;
}

ArchaeoLines::ArchaeoLines()
	: flagShowArchaeoLines(false)
	, withDecimalDegree(false)
	, flagUseDmsFormat(false)
	, flagShowSolstices(false)
	, flagShowCrossquarters(false)
	, flagShowMajorStandstills(false)
	, flagShowMinorStandstills(false)
	, flagShowSolarZenith(false)
	, flagShowSolarNadir(false)
	, toolbarButton(NULL)
{
	setObjectName("ArchaeoLines");
	font.setPixelSize(16);

	configDialog = new ArchaeoLinesDialog();
	conf = StelApp::getInstance().getSettings();

	messageTimer = new QTimer(this);
	messageTimer->setInterval(7000);
	messageTimer->setSingleShot(true);

	connect(messageTimer, SIGNAL(timeout()), this, SLOT(clearMessage()));
}

ArchaeoLines::~ArchaeoLines()
{
	delete configDialog;
}

bool ArchaeoLines::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

//! Determine which "layer" the plugin's drawing will happen on.
double ArchaeoLines::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
	  return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)-10.; // negative: draw earlier than landscape!
	//if (actionName==StelModule::ActionHandleMouseClicks)
	//	return -11;
	return 0;
}

void ArchaeoLines::init()
{
	if (!conf->childGroups().contains("ArchaeoLines"))
		restoreDefaultSettings();

	loadSettings();

	StelApp& app = StelApp::getInstance();

	// Create action for enable/disable & hook up signals	
	addAction("actionShow_Archaeo_Lines", N_("ArchaeoLines"), N_("ArchaeoLines"), "enabled", "Ctrl+U");

	// Initialize the message strings and make sure they are translated when the language changes.
	updateMessageText();
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateMessageText()));

	// Add a toolbar button
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
		if (gui!=NULL)
		{
			toolbarButton = new StelButton(NULL,
						       QPixmap(":/archaeoLines/bt_archaeolines_on.png"),
						       QPixmap(":/archaeoLines/bt_archaeolines_off.png"),
						       QPixmap(":/graphicGui/glow32x32.png"),
						       "actionShow_Archaeo_Lines");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar button for ArchaeoLines plugin: " << e.what();
	}
}

void ArchaeoLines::update(double deltaTime)
{
	messageFader.update((int)(deltaTime*1000));
	lineFader.update((int)(deltaTime*1000));
	static StelCore *core=StelApp::getInstance().getCore();

	withDecimalDegree = dynamic_cast<StelGui*>(StelApp::getInstance().getGui())->getFlagShowDecimalDegrees();
}


void ArchaeoLines::drawOne(StelCore *core, const float declination, const StelCore::FrameType frameType, const StelCore::RefractionMode refractionMode, const Vec3f txtColor, const Vec3f lineColor)
{
	const StelProjectorP prj = core->getProjection(frameType, refractionMode);
	StelPainter painter(prj);
	painter.setFont(font);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	if (lineFader.getInterstate() > 0.000001f)
	{
		painter.setColor(lineColor[0], lineColor[1], lineColor[1], lineFader.getInterstate());
	// TODO: COPY&PASTE code from GrisLinesMgr

	}
	if (messageFader.getInterstate() > 0.000001f)
	{
		painter.setColor(txtColor[0], txtColor[1], txtColor[2], messageFader.getInterstate());
		int x = 83;
		int y = 120;
		painter.drawText(x, y, messageEnabled);
	}
	glDisable(GL_BLEND);
}

//! Draw any parts on the screen which are for our module
void ArchaeoLines::draw(StelCore* core)
{
	if (lineFader.getInterstate() < 0.000001f && messageFader.getInterstate() < 0.000001f)
		return;
	if (flagShowSolstices)
	{
		drawOne(core, 23.5, StelCore::FrameEquinoxEqu, StelCore::RefractionAuto, textColor, lineColor);
		drawOne(core, -23.5, StelCore::FrameEquinoxEqu, StelCore::RefractionAuto, textColor, lineColor);
	}
	if (flagShowSolarZenith)
	{
		drawOne(core, 48.5, StelCore::FrameEquinoxEqu, StelCore::RefractionAuto, textColor, lineColor);
	}
}



void ArchaeoLines::handleKeys(QKeyEvent* event)
{
	event->setAccepted(false);
}



void ArchaeoLines::enableArchaeoLines(bool b)
{
	flagShowArchaeoLines = b;
	lineFader = b;
	messageFader = b;
	if (b)
	{
		//qDebug() << "ArchaeoLines::enableArchaeoLines starting timer";
		messageTimer->start();
	}
}



void ArchaeoLines::useDmsFormat(bool b)
{
	flagUseDmsFormat=b;
}

void ArchaeoLines::updateMessageText()
{
	// TRANSLATORS: instructions for using the ArchaeoLines plugin.
	messageEnabled = q_("ArchaeoLines enabled.");
	//// TRANSLATORS: instructions for using the ArchaeoLines plugin.
	//messageLeftButton = q_("Drag with the left button to measure, left-click to clear.");
	//// TRANSLATORS: instructions for using the ArchaeoLines plugin.
	//messageRightButton = q_("Right-clicking changes the end point only.");
	// TRANSLATORS: PA is abbreviation for phrase "Position Angle"
	messagePA = q_("PA=");
}

void ArchaeoLines::clearMessage()
{
	//qDebug() << "ArchaeoLines::clearMessage";
	messageFader = false;
}

void ArchaeoLines::restoreDefaultSettings()
{
	Q_ASSERT(conf);
	// Remove the old values...
	conf->remove("ArchaeoLines");
	// ...load the default values...
	loadSettings();
	// ...and then save them.
	saveSettings();
	// But this doesn't save the colors, so:
	conf->beginGroup("ArchaeoLines");
	conf->setValue("text_color", "0,0.5,1");
	conf->setValue("line_color", "0,0.5,1");
	conf->endGroup();
}

void ArchaeoLines::loadSettings()
{
	conf->beginGroup("ArchaeoLines");

	useDmsFormat(conf->value("angle_format_dms", false).toBool());
	textColor = StelUtils::strToVec3f(conf->value("text_color", "0,0.5,1").toString());
	lineColor = StelUtils::strToVec3f(conf->value("line_color", "0,0.5,1").toString());
	// 4 solar limits
	flagShowSolstices = conf->value("show_solstices", true).toBool();
	flagShowCrossquarters = conf->value("show_crossquarters", false).toBool();
	// 4 lunar limits
	flagShowMajorStandstills = conf->value("show_major_standstills", false).toBool();
	flagShowMinorStandstills = conf->value("show_minor_standstills", false).toBool();
	// Mesoamerica
	flagShowSolarZenith = conf->value("show_solar_zenith", false).toBool();
	flagShowSolarNadir  = conf->value("show_solar_nadir",  false).toBool();

	conf->endGroup();
}

void ArchaeoLines::saveSettings()
{
	conf->beginGroup("ArchaeoLines");

	conf->setValue("angle_format_dms",       isDmsFormat());
	conf->setValue("show_solstices",         isSolsticesDisplayed());
	conf->setValue("show_crossquarters",     isCrossquartersDisplayed());
	conf->setValue("show_minor_standstills", isMinorStandstillsDisplayed());
	conf->setValue("show_major_standstills", isMajorStandstillsDisplayed());
	conf->setValue("show_solar_zenith",      isSolarZenithDisplayed());
	conf->setValue("show_solar_nadir",       isSolarNadirDisplayed());

	conf->endGroup();
}
