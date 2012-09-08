/*
 * Copyright (C) 2012 Ferdinand Majerech
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

#include "VecMath.hpp"
#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "RendererStatistics.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"

#include <QAction>
#include <QDebug>
#include <QPixmap>
#include <QSettings>

//! This method is the one called automatically by the StelModuleMgr just
//! after loading the dynamic library
StelModule* RendererStatisticsPluginInterface::getStelModule() const
{
	return new RendererStatistics();
}

StelPluginInfo RendererStatisticsPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(RendererStatistics);

	StelPluginInfo info;
	info.id = "RendererStatistics";
	info.displayedName = N_("Renderer Statistics");
	info.authors = "Ferdinand Majerech";
	info.contact = "kiithsacmp@gmail.com";
	info.description = N_("Displays statistics about the renderer");
	return info;
}

Q_EXPORT_PLUGIN2(RendererStatistics, RendererStatisticsPluginInterface)


RendererStatistics::RendererStatistics()
	: enabled(false), glowIcon(NULL), onIcon(NULL), offIcon(NULL), toolbarButton(NULL)
{
	setObjectName("RendererStatistics");

	QSettings* conf = StelApp::getInstance().getSettings();
	// Setup defaults if not present
	if (!conf->contains("RendererStatistics/font_size"))
		conf->setValue("RendererStatistics/font_size", 16);

	if (!conf->contains("RendererStatistics/enable_at_startup"))
		conf->setValue("RendererStatistics/enable_at_startup", false);

	// Load settings from main config file
	fontSize = conf->value("RendererStatistics/font_size", 16).toInt();
	font.setPixelSize(fontSize);
	font.setWeight(QFont::Black);
	lastUpdateTime.start();
}

RendererStatistics::~RendererStatistics()
{
	if(glowIcon != NULL) {delete glowIcon;}
	if(onIcon != NULL)   {delete onIcon;}
	if(offIcon != NULL)  {delete offIcon;}
}

//! Determine which "layer" the plugin's drawing will happen on.
double RendererStatistics::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	return 0;
}

void RendererStatistics::init()
{
	qDebug() << "RendererStatistics::init()";
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		glowIcon     = new QPixmap(":/graphicGui/glow32x32.png");
		onIcon       = new QPixmap(":/RendererStatistics/btRendererStatistics_on.png");
		offIcon      = new QPixmap(":/RendererStatistics/btRendererStatistics_off.png");

		toolbarButton = new StelButton(NULL, *onIcon, *offIcon, *glowIcon, gui->getGuiAction("actionShow_RendererStatistics"));
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		connect(gui->getGuiAction("actionShow_RendererStatistics"), SIGNAL(toggled(bool)), this, SLOT(setEnabled(bool)));

		QSettings* conf = StelApp::getInstance().getSettings();
		setEnabled(conf->value("RendererStatistics/enable_at_startup", false).toBool());
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable create toolbar button for the RendererStatistics plugin: " << e.what();
	}
}

//! Draw any parts on the screen which are for our module
void RendererStatistics::draw(class StelCore* core, StelRenderer* renderer)
{
	Q_UNUSED(core);

	if(!enabled){return;}
	renderer->setFont(font);
	renderer->setGlobalColor(1.0f, 0.0f, 0.0f);

	int row = 0;
	int rowHeight = static_cast<int>(fontSize * 1.25);
	int top = 160 + 24 * rowHeight;
	// We only update statistics displayed once per second to avoid 
	// fast-changing numbers.
	if(lastUpdateTime.elapsed() > 1000)
	{
		statistics = renderer->getStatistics();
		lastUpdateTime.start();
	}
	const char* key;
	double value;
	statistics.resetIteration();
	while (statistics.getNext(key, value)) 
	{
		renderer->drawText(TextParams(64, top - row * rowHeight, key));
		const QString valueStr = QString("%1").arg(value, 0, 'f', 3);
		renderer->drawText(TextParams(384, top - row * rowHeight, valueStr));
		++row;
	}
}

void RendererStatistics::update(double deltaTime)
{
	Q_UNUSED(deltaTime);
}

void RendererStatistics::setEnabled(bool b)
{
	enabled = b;
}
