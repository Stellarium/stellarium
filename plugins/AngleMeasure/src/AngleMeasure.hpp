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
class AngleMeasureDialog;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
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
	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this
	//! plugin.
	virtual bool configureGui(bool show=true);
	bool isEnabled() const {return flagShowAngleMeasure;}
	bool isDmsFormat() const { return flagUseDmsFormat; }
	bool isPaDisplayed() const { return flagShowPA; }

	//! Set up the plugin with default values.  This means clearing out the AngleMeasure section in the
	//! main config.ini (if one already exists), and populating it with default values.
	void restoreDefaults(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void readSettingsFromConfig(void);

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig(void);

public slots:
	void enableAngleMeasure(bool b);
	void useDmsFormat(bool b);
	void showPositionAngle(bool b);

private slots:
	void updateMessageText();
	void clearMessage();

private:
	// if existing, delete AngleMeasure section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

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

