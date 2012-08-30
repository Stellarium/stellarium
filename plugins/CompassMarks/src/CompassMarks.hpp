/*
 * Copyright (C) 2009 Matthew Gates
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
 
#ifndef COMPASSMARKS_HPP_
#define COMPASSMARKS_HPP_

#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelFader.hpp"
#include <QFont>

class QPixmap;
class StelButton;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
class CompassMarks : public StelModule
{
	Q_OBJECT
public:
	CompassMarks();
	virtual ~CompassMarks();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	virtual double getCallOrder(StelModuleActionName actionName) const;

public slots:
	void setCompassMarks(bool b);
	
private slots:
	void cardinalPointsChanged(bool b);

private:
	// Font used for displaying our text
	QFont font;
	Vec3f markColor;
	LinearFader markFader;
	QPixmap* pxmapGlow;
	QPixmap* pxmapOnIcon;
	QPixmap* pxmapOffIcon;
	StelButton* toolbarButton;
	bool cardinalPointsState;
};

#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class CompassMarksStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;

};

#endif /*COMPASSMARKS_HPP_*/
