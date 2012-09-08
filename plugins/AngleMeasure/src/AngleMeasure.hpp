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
 
#ifndef ANGLEMEASURE_HPP_
#define ANGLEMEASURE_HPP_

#include <QFont>
#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelFader.hpp"

class QTimer;
class QPixmap;
class StelButton;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
class AngleMeasure : public StelModule
{
	Q_OBJECT
public:
	AngleMeasure();
	virtual ~AngleMeasure();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(class QKeyEvent* event);
	virtual void handleMouseClicks(class QMouseEvent* event);
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);

public slots:
	void enableAngleMeasure(bool b);

private slots:
	void updateMessageText();
	void clearMessage();

private:
	QFont font;
	bool flagShowAngleMeasure;
	LinearFader lineVisible;
	LinearFader messageFader;
	QTimer* messageTimer;
	QString messageEnabled;
	QString messageLeftButton;
	QString messageRightButton;
	bool dragging;
	Vec3d startPoint;
	Vec3d endPoint;
	Vec3d perp1StartPoint;
	Vec3d perp1EndPoint;
	Vec3d perp2StartPoint;
	Vec3d perp2EndPoint;
	Vec3f textColor;
	Vec3f lineColor;
	QString angleText;
	bool flagUseDmsFormat;
	StelButton* toolbarButton;

	void calculateEnds();
};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class AngleMeasureStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*ANGLEMEASURE_HPP_*/

