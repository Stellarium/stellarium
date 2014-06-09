/*
 * Pointer Coordinates plug-in for Stellarium
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
#include "PointerCoordinates.hpp"
#include "PointerCoordinatesWindow.hpp"

#include <QFontMetrics>
#include <QSettings>
#include <QPixmap>
#include <QPair>
#include <QMetaEnum>
#include <cmath>

StelModule* PointerCoordinatesStelPluginInterface::getStelModule() const
{
	return new PointerCoordinates();
}

StelPluginInfo PointerCoordinatesStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(PointerCoordinates);

	StelPluginInfo info;
	info.id = "PointerCoordinates";
	info.displayedName = N_("Pointer Coordinates");
	info.authors = "Alexander Wolf";
	info.contact = "http://stellarium.org";
	info.description = N_("This plugin shows the coordinates of the mouse pointer.");
	info.version = POINTERCOORDINATES_PLUGIN_VERSION;
	return info;
}

PointerCoordinates::PointerCoordinates()
	: currentPlace(TopRight)
	, flagShowCoordinates(false)
	, flagEnableAtStartup(false)
	, flagShowCoordinatesButton(false)
	, textColor(Vec3f(1,0.5,0))
	, coordinatesPoint(Vec3d(0,0,0))
	, fontSize(14)
	, toolbarButton(NULL)
{
	setObjectName("PointerCoordinates");
	mainWindow = new PointerCoordinatesWindow();
	StelApp &app = StelApp::getInstance();
	conf = app.getSettings();
	gui = dynamic_cast<StelGui*>(app.getGui());
}

PointerCoordinates::~PointerCoordinates()
{
	delete mainWindow;
}

void PointerCoordinates::init()
{
	if (!conf->childGroups().contains("PointerCoordinates"))
	{
		qDebug() << "PointerCoordinates: no coordinates section exists in main config file - creating with defaults";
		restoreDefaultConfiguration();
	}

	// populate settings from main config file.
	loadConfiguration();

	addAction("actionShow_MousePointer_Coordinates", N_("Pointer Coordinates"), N_("Show equatorial coordinates (J2000.0) of the mouse pointer"), "enabled", "");

	enableCoordinates(getFlagEnableAtStartup());
	setFlagShowCoordinatesButton(flagShowCoordinatesButton);
}

void PointerCoordinates::deinit()
{
	//
}

void PointerCoordinates::draw(StelCore *core)
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

	sPainter.drawText(getCoordinatesPlace(coordsText).first, getCoordinatesPlace(coordsText).second, coordsText);
}

void PointerCoordinates::enableCoordinates(bool b)
{
	flagShowCoordinates = b;
}

double PointerCoordinates::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	return 0;
}

bool PointerCoordinates::configureGui(bool show)
{
	if (show)
	{
		mainWindow->setVisible(true);
	}

	return true;
}

void PointerCoordinates::restoreDefaultConfiguration(void)
{
	// Remove the whole section from the configuration file
	conf->remove("PointerCoordinates");
	// Load the default values...
	loadConfiguration();
	// ... then save them.
	saveConfiguration();
	// But this doesn't save the color, so...
	conf->beginGroup("PointerCoordinates");
	conf->setValue("text_color", "1,0.5,0");
	conf->endGroup();
}

void PointerCoordinates::loadConfiguration(void)
{
	conf->beginGroup("PointerCoordinates");

	setFlagEnableAtStartup(conf->value("enable_at_startup", false).toBool());
	textColor = StelUtils::strToVec3f(conf->value("text_color", "1,0.5,0").toString());
	setFontSize(conf->value("font_size", 14).toInt());
	flagShowCoordinatesButton = conf->value("flag_show_button", true).toBool();
	setCurrentCoordinatesPlaceKey(conf->value("current_displaying_place", "TopRight").toString());

	conf->endGroup();
}

void PointerCoordinates::saveConfiguration(void)
{
	conf->beginGroup("PointerCoordinates");

	conf->setValue("enable_at_startup", getFlagEnableAtStartup());
	conf->setValue("flag_show_button", getFlagShowCoordinatesButton());
	conf->setValue("current_displaying_place", getCurrentCoordinatesPlaceKey());
	//conf->setValue("text_color", "1,0.5,0");
	conf->setValue("font_size", getFontSize());

	conf->endGroup();
}

void PointerCoordinates::setFlagShowCoordinatesButton(bool b)
{
	if (gui!=NULL)
	{
		if (b==true) {
			if (toolbarButton==NULL) {
				// Create the button
				toolbarButton = new StelButton(NULL,
							       QPixmap(":/PointerCoordinates/bt_PointerCoordinates_On.png"),
							       QPixmap(":/PointerCoordinates/bt_PointerCoordinates_Off.png"),
							       QPixmap(":/graphicGui/glow32x32.png"),
							       "actionShow_MousePointer_Coordinates");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		} else {
			gui->getButtonBar()->hideButton("actionShow_MousePointer_Coordinates");
		}
	}
	flagShowCoordinatesButton = b;
}

// Set the current place of the string with coordinates from its key
void PointerCoordinates::setCurrentCoordinatesPlaceKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("CoordinatesPlace"));
	CoordinatesPlace coordPlace = (CoordinatesPlace)en.keyToValue(key.toLatin1().data());
	if (coordPlace<0)
	{
		qWarning() << "Unknown coordinates place: " << key << "setting \"TopRight\" instead";
		coordPlace = TopRight;
	}
	setCurrentCoordinatesPlace(coordPlace);
}

//Get the current place of the string with coordinates
QString PointerCoordinates::getCurrentCoordinatesPlaceKey() const
{
	return metaObject()->enumerator(metaObject()->indexOfEnumerator("CoordinatesPlace")).key(currentPlace);
}

QPair<int, int> PointerCoordinates::getCoordinatesPlace(QString text)
{
	int x = 0, y = 0;
	float coeff = 1.5;
	QFontMetrics fm(font);
	QSize fs = fm.size(Qt::TextSingleLine, text);
	switch(getCurrentCoordinatesPlace())
	{
		case TopCenter:
			x = gui->getSkyGui()->getSkyGuiWidth()/2 - fs.width()/2;
			y = gui->getSkyGui()->getSkyGuiHeight() - fs.height()*coeff;
			break;
		case TopRight:
			x = 3*gui->getSkyGui()->getSkyGuiWidth()/4 - fs.width()/2;
			y = gui->getSkyGui()->getSkyGuiHeight() - fs.height()*coeff;
			break;
		case RightBottomCorner:
			x = gui->getSkyGui()->getSkyGuiWidth() - fs.width() - 10*coeff;
			y = fs.height();
			break;
	}
	return qMakePair(x, y);
}

