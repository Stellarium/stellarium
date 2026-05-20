/*
 * Visibility Map plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "VisibilityMap.hpp"
#include "gui/VisibilityMapDialog.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGuiItems.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QtMath>

namespace
{
QPixmap makeVisibilityMapIcon(bool on)
{
	QPixmap pix(160, 160);
	pix.fill(Qt::transparent);

	QPainter painter(&pix);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(QPen(on ? QColor(238, 238, 238) : QColor(155, 155, 155), 7.0));
	painter.setBrush(QColor(76, 76, 76));
	painter.drawEllipse(QPointF(80, 80), 66.0, 66.0);

	painter.setPen(QPen(QColor(210, 210, 210, on ? 215 : 145), 5.0));
	painter.drawArc(QRectF(22, 40, 116, 80), 0, 16 * 360);
	painter.drawLine(QPointF(14, 80), QPointF(146, 80));
	painter.drawLine(QPointF(80, 14), QPointF(80, 146));

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(18, 18, 18, on ? 165 : 205));
	painter.drawChord(QRectF(14, 14, 132, 132), -72 * 16, 152 * 16);

	painter.setBrush(on ? QColor(238, 238, 238) : QColor(178, 178, 178));
	painter.drawEllipse(QPointF(118, 43), 15.0, 15.0);
	painter.setPen(QPen(on ? QColor(238, 238, 238) : QColor(178, 178, 178), 5.0));
	for (int i = 0; i < 8; ++i)
	{
		const double a = i * M_PI / 4.0;
		const QPointF start(118 + qCos(a) * 22.0, 43 + qSin(a) * 22.0);
		const QPointF end(118 + qCos(a) * 33.0, 43 + qSin(a) * 33.0);
		painter.drawLine(start, end);
	}

	return pix;
}
}

StelModule* VisibilityMapStelPluginInterface::getStelModule() const
{
	return new DaylightMap();
}

StelPluginInfo VisibilityMapStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(DaylightMap);

	StelPluginInfo info;
	info.id = "DaylightMap";
	info.displayedName = N_("Visibility Map");
	info.authors = "Atque";
	info.contact = STELLARIUM_URL;
	info.description = N_("This plugin shows Earth daylight and twilight, including the sunrise-sunset "
	                       "terminator, civil, nautical, and astronomical twilight zones, Sun and Moon "
	                       "subpoint markers, rise/set isolines, object visibility maps, and daylight "
	                       "length isochrones.");
	info.version = VISIBILITYMAP_PLUGIN_VERSION;
	info.license = VISIBILITYMAP_PLUGIN_LICENSE;
	return info;
}

DaylightMap::DaylightMap()
	: mainWindow(new VisibilityMapDialog())
	, conf(StelApp::getInstance().getSettings())
	, gui(dynamic_cast<StelGui*>(StelApp::getInstance().getGui()))
	, flagShowButton(true)
	, toolbarButton(Q_NULLPTR)
{
	setObjectName("DaylightMap");
}

DaylightMap::~DaylightMap()
{
	delete mainWindow;
}

void DaylightMap::init()
{
	if (!conf->childGroups().contains("VisibilityMap"))
		restoreDefaultConfigIni();

	readSettingsFromConfig();

	addAction("actionShow_DaylightMap_dialog",
	          N_("Visibility Map"),
	          N_("Show visibility map"),
	          mainWindow, "visible", "");

	setFlagShowButton(flagShowButton);

	connect(StelApp::getInstance().getCore(),
	        SIGNAL(configurationDataSaved()),
	        this, SLOT(saveSettings()));
}

void DaylightMap::deinit()
{
}

double DaylightMap::getCallOrder(StelModuleActionName actionName) const
{
	Q_UNUSED(actionName)
	return 0.;
}

bool DaylightMap::configureGui(bool show)
{
	if (show)
		mainWindow->setVisible(true);
	return true;
}

void DaylightMap::restoreDefaults()
{
	restoreDefaultConfigIni();
	readSettingsFromConfig();
	setFlagShowButton(flagShowButton);
}

void DaylightMap::restoreDefaultConfigIni()
{
	conf->beginGroup("VisibilityMap");
	conf->remove("");
	conf->setValue("flag_show_button", true);
	conf->endGroup();
}

void DaylightMap::readSettingsFromConfig()
{
	conf->beginGroup("VisibilityMap");
	flagShowButton = conf->value("flag_show_button", true).toBool();
	conf->endGroup();
}

void DaylightMap::saveSettingsToConfig() const
{
	conf->beginGroup("VisibilityMap");
	conf->setValue("flag_show_button", getFlagShowButton());
	conf->endGroup();
}

void DaylightMap::setFlagShowButton(bool b)
{
	if (gui == Q_NULLPTR)
	{
		flagShowButton = b;
		emit flagShowButtonChanged(b);
		return;
	}

	if (b)
	{
		if (toolbarButton == Q_NULLPTR)
		{
			toolbarButton = new StelButton(Q_NULLPTR,
			                               makeVisibilityMapIcon(true),
			                               makeVisibilityMapIcon(false),
			                               QPixmap(":/graphicGui/miscGlow32x32.png"),
			                               "actionShow_DaylightMap_dialog",
			                               false);
		}
		gui->getButtonBar()->addButton(toolbarButton, "066-pluginsGroup");
	}
	else
	{
		gui->getButtonBar()->hideButton("actionShow_DaylightMap_dialog");
	}

	flagShowButton = b;
	emit flagShowButtonChanged(b);
}
