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
 
#ifndef _RENDERERSTATISTICS_HPP_
#define _RENDERERSTATISTICS_HPP_

#include "StelModule.hpp"

#include <QFont>
#include <QMap>
#include <QTime>

#include "renderer/StelRenderer.hpp"

//! Displays statistics about the running renderer (e.g. draws, texture memory usage, etc.).
class RendererStatistics : public StelModule
{
	Q_OBJECT
public:
	RendererStatistics();
	virtual ~RendererStatistics();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(class StelCore* core, class StelRenderer* renderer);
	virtual double getCallOrder(StelModuleActionName actionName) const;

public slots:
	void setEnabled(bool b);

private:
	//! Font used to draw the statistics.
	QFont font;
	//! Size of the font used to display the statistics.
	int fontSize;
	//! Is statistics display enabled?
	bool enabled;
	class QPixmap* glowIcon;
	class QPixmap* onIcon;
	class QPixmap* offIcon;
	class StelButton* toolbarButton;

	//! Renderer statistics from the last update.
	StelRendererStatistics statistics;

	//! Last time the statistics were updated.
	QTime lastUpdateTime;
};

#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class RendererStatisticsPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;

};

#endif /*_RENDERERSTATISTICS_HPP_*/
