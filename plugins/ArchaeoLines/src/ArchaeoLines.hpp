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
#include <QColor>
#include <QKeyEvent>
#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelFader.hpp"
#include "StelCore.hpp"
#include "StelObjectMgr.hpp"

class QTimer;
class QPixmap;
class StelButton;
class ArchaeoLinesDialog;
class ArchaeoLine;

/*! @defgroup archaeoLines ArchaeoLines Plug-in
@{
A tool for archaeo-/ethnoastronomical alignment studies.

The ArchaeoLines plugin displays any combination of declination arcs most
relevant to archaeo- or ethnoastronomical studies. Of course, principles
used in this context are derived from natural observations, and many of
these declinations are still important in everyday astronomy.
 -# Declinations of equinoxes (i.e. equator) and the solstices
 -# Declinations of the crossquarter days (days right between solstices and equinoxes)
 -# Declinations of the Major Lunar Standstills
 -# Declinations of the Minor Lunar Standstills
 -# Declination of the Zenith passage
 -# Declination of the Nadir passage
 -# Declination of the currently selected object
 -# Current declination of the sun
 -# Current declination of the moon
 -# Current declination of a naked-eye planet

The lunar lines include horizon parallax effects. There are two lines each
drawn, for maximum and minimum distance of the moon. Note that declination
of the moon at the major standstill can exceed the indicated limits if it
is high in the sky due to parallax effects.

It may be very instructive to let the time run quite fast and observe the
line of "current moon" swinging between its north and south limits each month.
These limits grow and shrink between the Major and Minor Standstills.

The sun likewise swings between the solstices. Over centuries, the solstice
declinations very slightly move as well.
@}
*/

//! @class ArchaeoLine
//! Class which manages a line (small circle) to display around the sky like the solstices line.
//! Modelled after @class SkyLine found in GridLinesMgr.cpp at V0.13.2, but with small-circle drawing.
//! @author Georg Zotti
//! @ingroup archaeoLines
class ArchaeoLine : QObject
{
	Q_OBJECT
	Q_PROPERTY(Vec3f color READ getColor WRITE setColor)
	Q_PROPERTY(bool flagLabel READ isLabelVisible WRITE setLabelVisible)
public:
	enum Line {
		Equinox,
		Solstices,
		Crossquarters,
		MajorStandstill,
		MinorStandstill,
		ZenithPassage,
		NadirPassage,
		SelectedObject,
		CurrentSun,
		CurrentMoon,
		CurrentPlanetNone, // actually a placeholder for counting/testing. By itself it makes no sense, i.e. deactivates the planet line
		CurrentPlanetMercury,
		CurrentPlanetVenus,
		CurrentPlanetMars,
		CurrentPlanetJupiter,
		CurrentPlanetSaturn
	};

	ArchaeoLine(ArchaeoLine::Line lineType, double declination);
	virtual ~ArchaeoLine(){}
	void draw(StelCore* core, float intensity=1.0f) const;
	const Vec3f& getColor() const {return color;}
	bool isDisplayed(void) const {return fader;}

public slots:
	void setColor(const Vec3f& c) {color = c;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void setFadeDuration(float duration) {fader.setDuration((int)(duration*1000.f));}
	void setDisplayed(const bool displayed){fader = displayed;}
	void setFontSize(double newSize){font.setPixelSize(newSize);}
	//! Re-translates the label.
	void updateLabel();
	void setLabelVisible(bool b){flagLabel=b;}
	//! reset declination (degrees) of this small arc.
	void setDeclination(double decl){declination=decl;}
	bool isLabelVisible() const{return flagLabel;}
	void setLineType(ArchaeoLine::Line line) {lineType=line; updateLabel();} // Meaningful only for CurrentPlanet... types
	//! change label. Used only for selected-object line - the other labels should not be changed!
	void setLabel(const QString newLabel){label=newLabel;}

private:
	ArchaeoLine::Line lineType;
	double declination; // degrees
	Vec3f color;
	StelCore::FrameType frameType;
	bool flagLabel; //! show the label. (some should be permanently silent)
	QString label;
	LinearFader fader;
	QFont font;
};

//! Main class of the ArchaeoLines plug-in.
//! Provides an on-screen visualisation of several small circles relevant mainly to archaeoastronomy.
//! GZ 2014-12
class ArchaeoLines : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled
		   READ isEnabled
		   WRITE enableArchaeoLines)
//	Q_PROPERTY(bool dmsFormat
//		   READ isDmsFormat
//		   WRITE useDmsFormat)
	Q_PROPERTY(bool flagShowEquinox
				READ    isEquinoxDisplayed
				WRITE showEquinox)
	Q_PROPERTY(bool flagShowSolstices
				READ    isSolsticesDisplayed
				WRITE showSolstices)
	Q_PROPERTY(bool flagShowCrossquarters
				READ    isCrossquartersDisplayed
				WRITE showCrossquarters)
	Q_PROPERTY(bool flagShowMajorStandstills
				READ    isMajorStandstillsDisplayed
				WRITE showMajorStandstills)
	Q_PROPERTY(bool flagShowMinorStandstills
				READ    isMinorStandstillsDisplayed
				WRITE showMinorStandstills)
	Q_PROPERTY(bool flagShowZenithPassage
				READ    isZenithPassageDisplayed
				WRITE showZenithPassage)
	Q_PROPERTY(bool flagShowNadirPassage
				READ    isNadirPassageDisplayed
				WRITE showNadirPassage)
	Q_PROPERTY(bool flagShowSelectedObject
				READ    isSelectedObjectDisplayed
				WRITE showSelectedObject)
	Q_PROPERTY(bool flagShowCurrentSun
				READ    isCurrentSunDisplayed
				WRITE showCurrentSun)
	Q_PROPERTY(bool flagShowCurrentMoon
				READ    isCurrentMoonDisplayed
				WRITE showCurrentMoon)
	Q_PROPERTY(ArchaeoLine::Line enumShowCurrentPlanet
				READ    whichCurrentPlanetDisplayed
				WRITE showCurrentPlanet)

public:
	ArchaeoLines();
	virtual ~ArchaeoLines();
	


	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(class QKeyEvent* event){event->setAccepted(false);}
	virtual bool configureGui(bool show=true);
	bool isEnabled() const {return flagShowArchaeoLines;}
	bool isDmsFormat() const { return flagUseDmsFormat; } // NOT SURE IF USEFUL
	bool isEquinoxDisplayed() const {return flagShowEquinox;}
	bool isSolsticesDisplayed() const {return flagShowSolstices;}
	bool isCrossquartersDisplayed() const {return flagShowCrossquarters;}
	bool isMajorStandstillsDisplayed() const {return flagShowMajorStandstills;}
	bool isMinorStandstillsDisplayed() const {return flagShowMinorStandstills;}
	bool isZenithPassageDisplayed() const {return flagShowZenithPassage;}
	bool isNadirPassageDisplayed() const {return flagShowNadirPassage;}
	bool isSelectedObjectDisplayed() const {return flagShowSelectedObject;}
	bool isCurrentSunDisplayed() const {return flagShowCurrentSun;}
	bool isCurrentMoonDisplayed() const {return flagShowCurrentMoon;}
	ArchaeoLine::Line whichCurrentPlanetDisplayed() const {return enumShowCurrentPlanet;}

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings().
	void restoreDefaultSettings();

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "ArchaeoLines" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see restoreDefaultSettings()
	void loadSettings();


public slots:
	void enableArchaeoLines(bool b);
	//void useDmsFormat(bool b);

	void showEquinox(bool b);
	void showSolstices(bool b);
	void showCrossquarters(bool b);
	void showMajorStandstills(bool b);
	void showMinorStandstills(bool b);
	void showZenithPassage(bool b);
	void showNadirPassage(bool b);
	void showSelectedObject(bool b);
	void showCurrentSun(bool b);
	void showCurrentMoon(bool b);
	void showCurrentPlanet(ArchaeoLine::Line l); // Allowed values for l: CurrentPlanetNone...CurrentPlanetSaturn.
	void showCurrentPlanet(QString planet); // Allowed values for planet: "none", "Mercury", "Venus", "Mars", "Jupiter", "Saturn".

	// called by the dialog GUI, converts GUI's QColor (0..255) to Stellarium's Vec3f float color.
	void setLineColor(ArchaeoLine::Line whichLine, QColor color);
	// called by the dialog UI, converts Stellarium's Vec3f float color to QColor (0..255).
	QColor getLineColor(ArchaeoLine::Line whichLine);

private:
	QFont font;
	bool flagShowArchaeoLines;
	bool withDecimalDegree;
	bool flagUseDmsFormat;
	LinearFader lineFader;

	Vec3f equinoxColor;
	Vec3f solsticesColor;
	Vec3f crossquartersColor;
	Vec3f majorStandstillColor;
	Vec3f minorStandstillColor;
	Vec3f zenithPassageColor;
	Vec3f nadirPassageColor;
	Vec3f selectedObjectColor;
	Vec3f currentSunColor;
	Vec3f currentMoonColor;
	Vec3f currentPlanetColor;



	bool flagShowEquinox;
	bool flagShowSolstices;
	bool flagShowCrossquarters;
	bool flagShowMajorStandstills;
	bool flagShowMinorStandstills;
	bool flagShowZenithPassage;
	bool flagShowNadirPassage;
	bool flagShowSelectedObject;
	bool flagShowCurrentSun;
	bool flagShowCurrentMoon;
	ArchaeoLine::Line enumShowCurrentPlanet;
	double lastJDE; // cache last-time-computed to every 10 days or so?

	ArchaeoLine * equinoxLine;
	ArchaeoLine * northernSolsticeLine;
	ArchaeoLine * southernSolsticeLine;
	ArchaeoLine * northernCrossquarterLine;
	ArchaeoLine * southernCrossquarterLine;
	ArchaeoLine * northernMajorStandstillLine0;
	ArchaeoLine * northernMajorStandstillLine1;
	ArchaeoLine * northernMinorStandstillLine2;
	ArchaeoLine * northernMinorStandstillLine3;
	ArchaeoLine * southernMinorStandstillLine4;
	ArchaeoLine * southernMinorStandstillLine5;
	ArchaeoLine * southernMajorStandstillLine6;
	ArchaeoLine * southernMajorStandstillLine7;
	ArchaeoLine * zenithPassageLine;
	ArchaeoLine * nadirPassageLine;
	ArchaeoLine * selectedObjectLine;
	ArchaeoLine * currentSunLine;
	ArchaeoLine * currentMoonLine;
	ArchaeoLine * currentPlanetLine;

	StelButton* toolbarButton;

	// draw one arc.
	//! @param core the StelCore object
	//! @param declination of the small circle
	//! @param frameType usually StelCore::FrameEquinoxEqu
	//! @param refractionMode usually StelCore::RefractionAuto
	//! @param txtColor color used for any text printed regarding this line
	//! @param lineColor color used for this line
//	void drawOne(StelCore *core, const float declination, const StelCore::FrameType frameType, const StelCore::RefractionMode refractionMode, const Vec3f txtColor, const Vec3f lineColor);
  
	QSettings* conf;

	// GUI
	ArchaeoLinesDialog* configDialog;
	StelCore* core; // used quite often, better keep a reference...
	StelObjectMgr* objMgr;
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class ArchaeoLinesStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*ARCHAEOLINES_HPP_*/

