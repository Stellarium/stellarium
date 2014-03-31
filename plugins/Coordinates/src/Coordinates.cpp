/*
 * Coordinates plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "SkyGui.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "Coordinates.hpp"
#include "CoordinatesWindow.hpp"

#include <QFontMetrics>
#include <QSettings>
#include <QPixmap>
#include <cmath>

StelModule* CoordinatesStelPluginInterface::getStelModule() const
{
	return new Coordinates();
}

StelPluginInfo CoordinatesStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(Coordinates);

	StelPluginInfo info;
	info.id = "Coordinates";
	info.displayedName = N_("Coordinates");
	info.authors = "Alexander Wolf";
	info.contact = "http://stellarium.org";
	info.description = N_("This plugin shows the coordinates of the mouse pointer.");
	info.version = COORDINATES_PLUGIN_VERSION;
	return info;
}

Coordinates::Coordinates()
	: flagShowCoordinates(false),
	  toolbarButton(NULL)
{
	setObjectName("Coordinates");
	mainWindow = new CoordinatesWindow();	
}

Coordinates::~Coordinates()
{
	delete mainWindow;
}

void Coordinates::init()
{
	StelApp &app = StelApp::getInstance();
	conf = app.getSettings();
	gui = dynamic_cast<StelGui*>(app.getGui());

	if (!conf->childGroups().contains("coordinates"))
	{
		qDebug() << "Coordinates: no coordinates section exists in main config file - creating with defaults";
		restoreDefaultConfigIni();
	}

	// populate settings from main config file.
	readSettingsFromConfig();

	addAction("actionShow_MousePointer_Coordinates", N_("Coordinates"), N_("Show coordinates of the mouse pointer"), "enabled", "");

	enableCoordinates(getFlagEnableAtStartup());
	setFlagShowCoordinatesButton(flagShowCoordinatesButton);

	// Initialize the message strings and make sure they are translated when
	// the language changes.
	updateMessageText();
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateMessageText()));
}

void Coordinates::deinit()
{
	//
}

void Coordinates::draw(StelCore *core)
{
	if (!isEnabled())
		return;

	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000, StelCore::RefractionAuto);
	StelPainter sPainter(prj);
	sPainter.setColor(textColor[0], textColor[1], textColor[2], 1.f);
	font.setPixelSize(getFontSize());
	sPainter.setFont(font);

	QPoint p = QCursor::pos(); // get screen coordinates of mouse cursor
	Vec3d mousePosition;
	float wh = prj->getViewportWidth()/2; // get half of width of the screen
	float hh = prj->getViewportHeight()/2; // get half of height of the screen
	float mx = p.x()-wh; // point 0 in center of the screen, axis X directed to right
	float my = p.y()-hh; // point 0 in center of the screen, axis Y directed to bottom
	// calculate position of mouse cursor via position of center of the screen (and invert axis Y)
	prj->unProject(prj->getViewportPosX()+wh+mx, prj->getViewportPosY()+hh+1-my, mousePosition);

	double dec_j2000, ra_j2000;
	StelUtils::rectToSphe(&ra_j2000,&dec_j2000,mousePosition); // Calculate RA/DE (J2000.0) and show it...
	QString coordsText = QString("%1/%2").arg(StelUtils::radToHmsStr(ra_j2000, true)).arg(StelUtils::radToDmsStr(dec_j2000, true));

	QFontMetrics fm(font);
	QSize fs = fm.size(Qt::TextSingleLine, coordsText);
	sPainter.drawText(gui->getSkyGui()->getSkyGuiWidth()/2 - fs.width()/2, gui->getSkyGui()->getSkyGuiHeight() - fs.height()*1.5, coordsText);
}

void Coordinates::enableCoordinates(bool b)
{
	flagShowCoordinates = b;
}

double Coordinates::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}

bool Coordinates::configureGui(bool show)
{
	if (show)
	{
		mainWindow->setVisible(true);
	}

	return true;
}

void Coordinates::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	readSettingsFromConfig();
}

void Coordinates::restoreDefaultConfigIni(void)
{
	conf->beginGroup("coordinates");

	// delete all existing Coordinates settings...
	conf->remove("");

	conf->setValue("enable_at_startup", false);
	conf->setValue("flag_show_button", true);
	conf->setValue("text_color", "1,0.5,0");
	conf->setValue("font_size", 14);

	conf->endGroup();
}

void Coordinates::readSettingsFromConfig(void)
{
	conf->beginGroup("coordinates");

	setFlagEnableAtStartup(conf->value("enable_at_startup", false).toBool());
	textColor = StelUtils::strToVec3f(conf->value("text_color", "1,0.5,0").toString());
	setFontSize(conf->value("font_size", 14).toInt());
	flagShowCoordinatesButton = conf->value("flag_show_button", true).toBool();

	conf->endGroup();
}

void Coordinates::saveSettingsToConfig(void)
{
	conf->beginGroup("coordinates");

	conf->setValue("enable_at_startup", getFlagEnableAtStartup());
	conf->setValue("flag_show_button", getFlagShowCoordinatesButton());
	//conf->setValue("text_color", "1,0.5,0");
	conf->setValue("font_size", getFontSize());

	conf->endGroup();
}

void Coordinates::updateMessageText()
{
	messageCoordinates = q_("");
}

void Coordinates::setFlagShowCoordinatesButton(bool b)
{
	if (b==true) {
		if (toolbarButton==NULL) {
			// Create the button
			toolbarButton = new StelButton(NULL,
						       QPixmap(":/Coordinates/bt_Coordinates_On.png"),
						       QPixmap(":/Coordinates/bt_Coordinates_Off.png"),
						       QPixmap(":/graphicGui/glow32x32.png"),
						       "actionShow_MousePointer_Coordinates");
		}
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	} else {
		gui->getButtonBar()->hideButton("actionShow_MousePointer_Coordinates");
	}
	flagShowCoordinatesButton = b;
}

