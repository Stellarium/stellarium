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

#include "config.h"

#include <QFont>
#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelFader.hpp"

class QTimer;
class QPixmap;
class StelButton;
class AngleMeasureDialog;

//! Main class of the Angle Measure plug-in.
//! Provides an on-screen angle measuring tool.
class AngleMeasure : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled
		   READ isEnabled
		   WRITE enableAngleMeasure)
	Q_PROPERTY(bool dmsFormat
		   READ isDmsFormat
		   WRITE useDmsFormat)
	Q_PROPERTY(bool paDisplayed
		   READ isPaDisplayed
		   WRITE showPositionAngle)
public:
	AngleMeasure();
	virtual ~AngleMeasure();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(class QKeyEvent* event);
	virtual void handleMouseClicks(class QMouseEvent* event);
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	virtual bool configureGui(bool show=true);
	bool isEnabled() const {return flagShowAngleMeasure;}
	bool isDmsFormat() const { return flagUseDmsFormat; }
	bool isPaDisplayed() const { return flagShowPA; }

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings() and saveSettings().
	void restoreDefaultSettings();

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "AngleMeasure" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see saveSettings(), restoreSettings()
	void loadSettings();

	//! Save the plug-in's settings to the configuration file.
	//! @warning textColor and lineColor are not saved, probably because
	//! they can't be changed by the user in-program.
	//! @todo find a way to save color values without "rounding drift"
	//! (this is especially important for restoring default color values).
	//! @see loadSettings(), restoreSettings()
	void saveSettings();

public slots:
	void enableAngleMeasure(bool b);
	void useDmsFormat(bool b);
	void showPositionAngle(bool b);

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
	QString messagePA;
	bool dragging;
	Vec3d startPoint;
	Vec3d endPoint;
	Vec3d perp1StartPoint;
	Vec3d perp1EndPoint;
	Vec3d perp2StartPoint;
	Vec3d perp2EndPoint;
	Vec3f textColor;
	Vec3f lineColor;
	double angle;
	bool flagUseDmsFormat;
	bool flagShowPA;
	StelButton* toolbarButton;

	void calculateEnds();
	QString calculateAngle(void) const;
	QString calculatePositionAngle(const Vec3d p1, const Vec3d p2) const;

	QSettings* conf;

	// GUI
	AngleMeasureDialog* configDialog;
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class AngleMeasureStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "stellarium.StelGuiPluginInterface/1.0")
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*ANGLEMEASURE_HPP_*/

