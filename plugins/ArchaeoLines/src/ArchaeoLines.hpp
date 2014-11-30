/*
 * Copyright (C) 2014 Georg Zotti
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
 
#ifndef ARCHAEOLINES_HPP_
#define ARCHAEOLINES_HPP_

#include <QFont>
#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelFader.hpp"
#include "StelCore.hpp"

class QTimer;
class QPixmap;
class StelButton;
class ArchaeoLinesDialog;

//! Main class of the Angle Measure plug-in.
//! Provides an on-screen angle measuring tool.
//! GZ extended in 2014-09, enough to call it V4.0
//! Equatorial Mode (original): mark start,end: distance/position angle in the sky, line rotates with sky, spherical angles influenced by refraction (numbers given on celestial sphere).
//! Horizontal Mode: mark start,end: distance/position angle in alt/azimuthal coordinates, line stays fixed in alt-az system. Angle may be different near to horizon because of refraction!
//! It is possible to link start and/or end to the sky. Distance/position angle still always in alt/azimuthal coordinates.
class ArchaeoLines : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled
		   READ isEnabled
		   WRITE enableArchaeoLines)
	Q_PROPERTY(bool dmsFormat
		   READ isDmsFormat
		   WRITE useDmsFormat)
//	Q_PROPERTY(bool paDisplayed
//		   READ isPaDisplayed
//		   WRITE showPositionAngle)
	  Q_PROPERTY(bool flagShowSolstices
				 READ       isSolsticesDisplayed
				 WRITE    showSolstices)
	  Q_PROPERTY(bool flagShowCrossquarters
				 READ       isCrossquartersDisplayed
				 WRITE    showCrossquarters)
	  Q_PROPERTY(bool flagShowMajorStandstills
				 READ       isMajorStandstillsDisplayed
				 WRITE    showMajorStandstills)
	  Q_PROPERTY(bool flagShowMinorStandstills
				 READ       isMinorStandstillsDisplayed
				 WRITE    showMinorStandstills)
	  Q_PROPERTY(bool flagShowSolarZenith
				 READ       isSolarZenithDisplayed
				 WRITE    showSolarZenith)
	  Q_PROPERTY(bool flagShowSolarNadir
				 READ       isSolarNadirDisplayed
				 WRITE    showSolarNadir)

public:
	ArchaeoLines();
	virtual ~ArchaeoLines();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(class QKeyEvent* event);
	//virtual void handleMouseClicks(class QMouseEvent* event);
	//virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	virtual bool configureGui(bool show=true);
	bool isEnabled() const {return flagShowArchaeoLines;}
	bool isDmsFormat() const { return flagUseDmsFormat; }
	bool isSolsticesDisplayed() const {return flagShowSolstices;}
	bool isCrossquartersDisplayed() const {return flagShowCrossquarters;}
	bool isMajorStandstillsDisplayed() const {return flagShowMajorStandstills;}
	bool isMinorStandstillsDisplayed() const {return flagShowMinorStandstills;}
	bool isSolarZenithDisplayed() const {return flagShowSolarZenith;}
	bool isSolarNadirDisplayed() const {return flagShowSolarNadir;}

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings() and saveSettings().
	void restoreDefaultSettings();

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "ArchaeoLines" section in Stellarium's
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
	void enableArchaeoLines(bool b);
	void useDmsFormat(bool b);

	void showSolstices(bool b){ flagShowSolstices=b; }
	void showCrossquarters(bool b){ flagShowCrossquarters=b;}
	void showMajorStandstills(bool b){ flagShowMajorStandstills=b;}
	void showMinorStandstills(bool b){flagShowMinorStandstills=b;}
	void showSolarZenith(bool b){flagShowSolarZenith=b;}
	void showSolarNadir(bool b){flagShowSolarNadir=b;}

private slots:
	void updateMessageText();
	void clearMessage();

private:
	QFont font;
	bool flagShowArchaeoLines;
	bool withDecimalDegree;
	bool flagUseDmsFormat;
	LinearFader lineFader;
	LinearFader messageFader;
	QTimer* messageTimer;
	QString messageEnabled;
  //	QString messageLeftButton;
  //	QString messageRightButton;
	QString messagePA;

	Vec3f textColor;
	Vec3f lineColor;


	bool flagShowSolstices;
	bool flagShowCrossquarters;
	bool flagShowMajorStandstills;
	bool flagShowMinorStandstills;
	bool flagShowSolarZenith;
	bool flagShowSolarNadir;
	double lastJD; // cache last-time-computed to 1/month or so?
	float lunarMajorNorth;
	float lunarMajorSouth;
	float lunarMinorNorth;
	float lunarMinorSouth;

	StelButton* toolbarButton;

	// draw one arc.
	//! @param core the StelCore object
	//! @param declination of the small circle
	//! @param frameType usually StelCore::FrameEquinoxEqu
	//! @param refractionMode usually StelCore::RefractionAuto
	//! @param txtColor color used for any text printed regarding this line
	//! @param lineColor color used for this line
	void drawOne(StelCore *core, const float declination, const StelCore::FrameType frameType, const StelCore::RefractionMode refractionMode, const Vec3f txtColor, const Vec3f lineColor);
  
	QSettings* conf;

	// GUI
	ArchaeoLinesDialog* configDialog;
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class ArchaeoLinesStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "stellarium.StelGuiPluginInterface/1.0")
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*ARCHAEOLINES_HPP_*/

