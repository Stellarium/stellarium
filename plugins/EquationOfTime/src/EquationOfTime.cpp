/*
 * Equation Of Time plug-in for Stellarium
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
#include "Planet.hpp"
#include "EquationOfTime.hpp"
#include "EquationOfTimeWindow.hpp"

#include <QFontMetrics>
#include <QSettings>
#include <QPixmap>
#include <cmath>

StelModule* EquationOfTimeStelPluginInterface::getStelModule() const
{
	return new EquationOfTime();
}

StelPluginInfo EquationOfTimeStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(EquationOfTime);

	StelPluginInfo info;
	info.id = "EquationOfTime";
	info.displayedName = N_("Equation of Time");
	info.authors = "Alexander Wolf";
	info.contact = STELLARIUM_URL;
	info.description = N_("This plugin shows the solution of the equation of time.");
	info.version = EQUATIONOFTIME_PLUGIN_VERSION;
	info.license = EQUATIONOFTIME_PLUGIN_LICENSE;
	return info;
}

EquationOfTime::EquationOfTime()
	: flagShowSolutionEquationOfTime(false)
	, flagUseInvertedValue(false)
	, flagUseMsFormat(false)
	, flagEnableAtStartup(false)
	, flagShowEOTButton(false)
	, fontSize(20)
	, toolbarButton(Q_NULLPTR)
{
	setObjectName("EquationOfTime");
	mainWindow = new EquationOfTimeWindow();
	StelApp &app = StelApp::getInstance();
	conf = app.getSettings();
	gui = dynamic_cast<StelGui*>(app.getGui());
}

EquationOfTime::~EquationOfTime()
{
	delete mainWindow;
}

void EquationOfTime::init()
{
	StelApp &app = StelApp::getInstance();
	if (!conf->childGroups().contains("EquationOfTime"))
	{
		qDebug() << "[EquationOfTime] no EquationOfTime section exists in main config file - creating with defaults";
		restoreDefaultConfigIni();
	}

	// populate settings from main config file.
	readSettingsFromConfig();

	addAction("actionShow_EquationOfTime", N_("Equation of Time"), N_("Show solution for Equation of Time"), "showEOT", "Ctrl+Alt+T");

	enableEquationOfTime(getFlagEnableAtStartup());
	setFlagShowEOTButton(flagShowEOTButton);

	// Initialize the message strings and make sure they are translated when
	// the language changes.
	updateMessageText();
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateMessageText()));
	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveSettings()));
}

void EquationOfTime::deinit()
{
	//
}

void EquationOfTime::draw(StelCore *core)
{
	if (!isEnabled())
		return;
	if (core->getCurrentPlanet()->getEnglishName()!="Earth")
		return;

	StelPainter sPainter(core->getProjection2d());
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	float ppx = static_cast<float>(params.devicePixelsPerPixel);

	sPainter.setColor(textColor[0], textColor[1], textColor[2], 1.f);
	font.setPixelSize(getFontSize());
	sPainter.setFont(font);

	QString timeText;
	double eqTime = core->getSolutionEquationOfTime(core->getJDE());

	if (getFlagInvertedValue())
		eqTime *= -1;

	if (getFlagMsFormat())
	{
		int seconds = qRound((eqTime - static_cast<int>(eqTime))*60);
		QString messageSecondsValue = QString("%1").arg(qAbs(seconds), 2, 10, QLatin1Char('0'));

		timeText = QString("%1: %2%3%4%5%6").arg(messageEquation, (eqTime<0? QString(QLatin1Char('-')):QString()), QString::number(static_cast<int>(qAbs(eqTime))), messageEquationMinutes, messageSecondsValue, messageEquationSeconds);
	}
	else
		timeText = QString("%1: %2%3").arg(messageEquation, QString::number(eqTime, 'f', 2), messageEquationMinutes);


	QFontMetrics fm(font);
	QSize fs = fm.size(Qt::TextSingleLine, timeText);	

	sPainter.drawText(gui->getSkyGui()->getSkyGuiWidth()*ppx/2 - fs.width()*ppx/2, gui->getSkyGui()->getSkyGuiHeight()*ppx - fs.height()*ppx*1.5, timeText);

	//qDebug() << timeText;
}

double EquationOfTime::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LabelMgr")->getCallOrder(actionName)+110.;
	return 0;
}

bool EquationOfTime::configureGui(bool show)
{
	if (show)
	{
		mainWindow->setVisible(true);
	}

	return true;
}

void EquationOfTime::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	readSettingsFromConfig();
}

void EquationOfTime::restoreDefaultConfigIni(void)
{
	conf->beginGroup("EquationOfTime");

	// delete all existing Equation Of Time settings...
	conf->remove("");

	conf->setValue("enable_at_startup", false);
	conf->setValue("flag_use_ms_format", true);
	conf->setValue("flag_use_inverted_value", false);
	conf->setValue("flag_show_button", true);
	conf->setValue("text_color", "0.0,0.5,1.0");
	conf->setValue("font_size", 20);

	conf->endGroup();
}

void EquationOfTime::readSettingsFromConfig(void)
{
	conf->beginGroup("EquationOfTime");

	setFlagEnableAtStartup(conf->value("enable_at_startup", false).toBool());
	setFlagMsFormat(conf->value("flag_use_ms_format", true).toBool());
	setFlagInvertedValue(conf->value("flag_use_inverted_value", false).toBool());	
	setTextColor(Vec3f(conf->value("text_color", "0.0,0.5,1.0").toString()));
	setFontSize(conf->value("font_size", 20).toInt());
	flagShowEOTButton = conf->value("flag_show_button", true).toBool();

	conf->endGroup();
}

void EquationOfTime::saveSettingsToConfig(void) const
{
	conf->beginGroup("EquationOfTime");

	conf->setValue("enable_at_startup", getFlagEnableAtStartup());
	conf->setValue("flag_use_ms_format", getFlagMsFormat());
	conf->setValue("flag_use_inverted_value", getFlagInvertedValue());
	conf->setValue("flag_show_button", getFlagShowEOTButton());
	conf->setValue("text_color", getTextColor().toStr());
	conf->setValue("font_size", getFontSize());

	conf->endGroup();
}

void EquationOfTime::updateMessageText()
{
	messageEquation = q_("Equation of Time");
	// TRANSLATORS: minutes.
	messageEquationMinutes = qc_("m", "time");
	// TRANSLATORS: seconds.
	messageEquationSeconds = qc_("s", "time");
}

void EquationOfTime::setFlagShowEOTButton(bool b)
{
	if (b==true) {
		if (toolbarButton==Q_NULLPTR) {
			// Create the button
			toolbarButton = new StelButton(Q_NULLPTR,
						       QPixmap(":/EquationOfTime/bt_EquationOfTime_On.png"),
						       QPixmap(":/EquationOfTime/bt_EquationOfTime_Off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_EquationOfTime");
		}
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	} else {
		gui->getButtonBar()->hideButton("actionShow_EquationOfTime");
	}
	flagShowEOTButton = b;	
}

void EquationOfTime::enableEquationOfTime(bool b)
{
	flagShowSolutionEquationOfTime = b;
	emit equationOfTimeStateChanged(b);
}

void EquationOfTime::setFlagInvertedValue(bool b)
{
	flagUseInvertedValue=b;
}

void EquationOfTime::setFlagMsFormat(bool b)
{
	flagUseMsFormat=b;
}
//! Enable plugin usage at startup
void EquationOfTime::setFlagEnableAtStartup(bool b)
{
	flagEnableAtStartup=b;
}
//! Set font size for message
void EquationOfTime::setFontSize(int size)
{
	fontSize=size;
}

